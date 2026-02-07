#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "util.hpp"
#include "crawler.hpp"

QSet<QString> Crawler::sHostnameBlacklist;

Crawler::Crawler(QObject *parent) : QObject(parent)
{
	uint32_t rngSeed=QDateTime::currentSecsSinceEpoch()+reinterpret_cast<uintptr_t>(this);
	mURLListActive=new QList<QUrl>;
	mURLListQueued=new QList<QUrl>;
	mRNG=new QRandomGenerator(rngSeed);
	mLoadingIntervalTimer=new QTimer(this);
	mWebPageProcessor=new WebPageProcessor(this);
	mIndexer=new Indexer(this);
	mLoadingIntervalTimer->setSingleShot(1);
	connect(mWebPageProcessor, &WebPageProcessor::pageProcessingFinished, this, &Crawler::onPageProcessingFinished);
	connect(mLoadingIntervalTimer, &QTimer::timeout, this, &Crawler::loadNextPage);
	connect(this, &Crawler::needToIndexNewPage, mIndexer, &Indexer::addPage);
}

Crawler::~Crawler()
{
	stop();
	const QList<QStringList*> crawlingZones(mCrawlingZones.values());
	mCrawlingZones.clear();
	for(QStringList *crawlingZone : crawlingZones)
	{
		delete crawlingZone;
	}
	delete mRNG;
	delete mURLListQueued;
	delete mURLListActive;
}

int Crawler::loadSettingsFromJSONFile(const QString &path_to_file)
{
	qDebug("Crawler::loadSettingsFromJSONFile");

	QFile crawlerConfigFile(path_to_file);

	if (!crawlerConfigFile.exists())
	{
		qDebug() << path_to_file << "doesn't exist";
		return 22;
	}

	if(!crawlerConfigFile.open(QIODevice::ReadOnly))
	{
		qDebug() << "Couldn't open" << path_to_file;
		return 33;
	}

	QByteArray crawlerConfigData=crawlerConfigFile.readAll();
	crawlerConfigFile.close();

	QJsonParseError err;
	QJsonDocument crawlerConfigJsonDoc = QJsonDocument::fromJson(crawlerConfigData, &err);

	if (err.error != QJsonParseError::NoError)
	{
		qDebug() << "Unable to parse" << path_to_file;
		qDebug() << err.errorString();
		return 44;
	}

	if(!crawlerConfigJsonDoc.isObject())
	{
		qDebug() << "Unable to get valid JSON object from" << path_to_file;
		return 55;
	}

	QJsonObject crawlerConfigJsonObject = crawlerConfigJsonDoc.object();

	if(crawlerConfigJsonObject.value("http_user_agent").isString())
	{
		this->setHttpUserAgent(crawlerConfigJsonObject.value("http_user_agent").toString());
	}

	if(crawlerConfigJsonObject.value("indexer_database_directory").isString())
	{
		this->setDatabaseDirectory(crawlerConfigJsonObject.value("indexer_database_directory").toString());
	}

	QSize crawlerWindowSize(0, 0);
	if(crawlerConfigJsonObject.value("window_width").isDouble())
	{
		crawlerWindowSize.setWidth(crawlerConfigJsonObject.value("window_width").toDouble());
	}
	if(crawlerConfigJsonObject.value("window_height").isDouble())
	{
		crawlerWindowSize.setHeight(crawlerConfigJsonObject.value("window_height").toDouble());
	}
	this->setWindowSize(crawlerWindowSize);

	if(crawlerConfigJsonObject.value("allowed_url_schemes").isArray())
	{
		const QJsonArray &allowedURLSchemes=crawlerConfigJsonObject.value("allowed_url_schemes").toArray();
		for (const QJsonValue &allowedURLScheme : allowedURLSchemes)
		{
			this->addAllowedURLScheme(allowedURLScheme.toString());
		}
	}

	if(crawlerConfigJsonObject.value("start_urls").isArray())
	{
		const QJsonArray &startURLs=crawlerConfigJsonObject.value("start_urls").toArray();
		for (const QJsonValue &startURL : startURLs)
		{
			this->addURLToQueue(startURL.toString());
		}
	}

	if(crawlerConfigJsonObject.value("crawling_zones").isArray())
	{
		const QJsonArray &crawlingZones=crawlerConfigJsonObject.value("crawling_zones").toArray();
		for (const QJsonValue &crawlingZone : crawlingZones)
		{
			this->addCrawlingZone(crawlingZone.toString());
		}
	}

	if(crawlerConfigJsonObject.value("black_list").isArray())
	{
		const QJsonArray &blHosts=crawlerConfigJsonObject.value("black_list").toArray();
		for (const QJsonValue &blHost : blHosts)
		{
			this->addHostnameToBlacklist(blHost.toString());
		}
	}

	return 0;
}

void Crawler::setPathToFirefoxProfile(const QString &path)
{
	mPathToFireFoxProfile=path;
}

void Crawler::setHttpUserAgent(const QString &user_agent)
{
	mWebPageProcessor->setHttpUserAgent(user_agent);
}

void Crawler::setWindowSize(const QSize &window_size)
{
	mWebPageProcessor->setWindowSize(window_size);
}

void Crawler::setDatabaseDirectory(const QString &database_directory)
{
	mIndexer->setDatabaseDirectory(database_directory);
}

QString Crawler::getDatabaseDirectory() const
{
	return mIndexer->getDatabaseDirectory();
}

void Crawler::addAllowedURLScheme(const QString &scheme)
{
	if(!scheme.isEmpty())
	{
		mAllowedURLSchemes.append(scheme);
	}
}

void Crawler::loadNextPage()
{
	qDebug("Crawler::loadNextPage");
	if (mURLListActive->isEmpty())
	{
		qSwap(mURLListActive, mURLListQueued);
		if (mURLListActive->isEmpty())
		{
			emit finished();
			return;
		}
	}
	QUrl nextURL = mURLListActive->takeAt(mRNG->bounded(0, mURLListActive->count()));
	qDebug() << nextURL.toString();
	qDebug() << mURLListActive->count()+mURLListQueued->count() << "URLs pending on the list";
	mWebPageProcessor->loadPage(nextURL);
}

#ifndef NDEBUG
static int visited_n=0;
#endif

void Crawler::onPageProcessingFinished()
{
	qDebug("Crawler::onPageProcessingFinished");

	const QString &pageContentText = mWebPageProcessor->getPageContentAsTEXT();
	const QString &pageContentHtml = mWebPageProcessor->getPageContentAsHTML();
	const QList<QUrl> &pageLinksList = mWebPageProcessor->getPageLinks();
	PageMetadata pageMetadata;

	pageMetadata.timeStamp = QDateTime::currentDateTime();
	pageMetadata.contentHash = xorshift_hash_64(pageContentHtml.toUtf8());
	pageMetadata.url = mWebPageProcessor->getPageURLEncoded(QUrl::RemoveFragment);
	pageMetadata.urlHash = xorshift_hash_64(pageMetadata.url);
	pageMetadata.title = mWebPageProcessor->getPageTitle();
	pageMetadata.words = ExtractWordsAndFrequencies(pageContentText);
	pageMetadata.wordsTotal = 0;
	QMap<QString, quint64>::ConstIterator pageWordsIt;
	for(pageWordsIt=pageMetadata.words.constBegin(); pageWordsIt!=pageMetadata.words.constEnd(); pageWordsIt++)
	{
		pageMetadata.wordsTotal += pageWordsIt.value();
	}

	mVisitedURLsHashes.insert(pageMetadata.urlHash);

	qDebug() << pageMetadata.url;

	emit needToIndexNewPage(pageMetadata);

	addURLsToQueue(pageLinksList);

#ifndef NDEBUG
	QFile pageHTMLFile(QString("page_")+QString::number(pageMetadata.contentHash&UINT32_MAX, 16).toUpper() + QString(".html"));
	if (pageHTMLFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		pageHTMLFile.write(pageContentHtml.toUtf8());
		pageHTMLFile.close();
	}
	else
	{
		qWarning() << "Failed to open page.html";
	}

	QFile pageTXTFile(QString("page_")+QString::number(pageMetadata.contentHash&UINT32_MAX, 16).toUpper() + QString(".txt"));
	if (pageTXTFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		pageTXTFile.write(pageContentText.toUtf8());
		pageTXTFile.close();
	}
	else
	{
		qWarning() << "Failed to open page.txt";
	}

	QFile pageLinksFile(QString("page_")+QString::number(pageMetadata.contentHash&UINT32_MAX, 16).toUpper() + QString("_links.txt"));
	if (pageLinksFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		for(const QUrl &link : pageLinksList)
		{
			pageLinksFile.write(link.toString().toUtf8()+QByteArray("\n"));
		}
		pageLinksFile.close();
	}
	else
	{
		qWarning() << "Failed to open page_links.txt";
	}

	QFile pageWordsFile(QString("page_")+QString::number(pageMetadata.contentHash&UINT32_MAX, 16).toUpper() + QString("_words.txt"));
	if (pageWordsFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		for(const QString &word : pageMetadata.words.keys())
		{
			pageWordsFile.write(word.toUtf8()+" "+QString::number(pageMetadata.words[word]).toUtf8()+QByteArray("\n"));
		}
		pageWordsFile.close();
	}
	else
	{
		qWarning() << "Failed to open page_words.txt";
	}

	if(++visited_n>=100)
	{
		stop();
	}
	else
#endif
	{
		mLoadingIntervalTimer->setInterval(mRNG->bounded(PAGE_LOADING_INTERVAL_MIN, PAGE_LOADING_INTERVAL_MAX));
		mLoadingIntervalTimer->start();
	}
}

void Crawler::addURLsToQueue(const QList<QUrl> &urls)
{
	qDebug("Crawler::addURLsToQueue");
	for (const QUrl &url : urls)
	{
		addURLToQueue(url);
	}
}

void Crawler::addURLToQueue(const QUrl &url)
{
	qDebug("Crawler::addURLToQueue");
	QUrl urlAdjusted=url.adjusted(QUrl::RemoveFragment);
	uint64_t urlHash = xorshift_hash_64(urlAdjusted.toEncoded());
	QString URLString=urlAdjusted.toString();
	qDebug() << URLString;
	bool skipThisURL=false;
	if (sHostnameBlacklist.contains(urlAdjusted.host()))
	{
		skipThisURL=true;
		qDebug() << "Skipping blacklisted host";
	}
	else if (mVisitedURLsHashes.contains(urlHash))
	{
		skipThisURL=true;
		qDebug() << "Skipping visited page";
	}
	bool urlAllowedByZonePrefix=1;
	const QStringList* const zonePrefixList=mCrawlingZones.value(urlAdjusted.host(), nullptr);
	if(nullptr!=zonePrefixList)
	{
		urlAllowedByZonePrefix=0;
		for(const QString &zonePrefix : *zonePrefixList)
		{
			if(!zonePrefix.isEmpty())
			{
				if(URLString.startsWith(zonePrefix))
				{
					urlAllowedByZonePrefix=1;
					break;
				}
			}
		}
	}
	if(!urlAllowedByZonePrefix)
	{
		skipThisURL=true;
		qDebug() << "Skipping page outside crawling zone";
	}
	if(!mAllowedURLSchemes.isEmpty())
	{
		if(!mAllowedURLSchemes.contains(urlAdjusted.scheme()))
		{
			skipThisURL=true;
			qDebug() << "Skipping URL due to inacceptable scheme:" << urlAdjusted.scheme();
		}
	}
	if(mURLListQueued->contains(urlAdjusted) || mURLListActive->contains(urlAdjusted))
	{
		skipThisURL=true;
		qDebug() << "Skipping duplicate URL";
	}
	if(skipThisURL!=true)
	{
		mURLListQueued->append(urlAdjusted);
		qDebug() << "Adding URL to the processing list";
	}
}

void Crawler::addHostnameToBlacklist(const QString &hostname)
{
	qDebug("Crawler::addHostnameToBlacklist");
	if (!sHostnameBlacklist.contains(hostname))
	{
		sHostnameBlacklist.insert(hostname);
		qDebug() << "Host name has been added to the blacklist:" << hostname;
	}
}

void Crawler::addCrawlingZone(const QUrl &zone_prefix)
{
	qDebug("Crawler::addCrawlingZone");
	if(zone_prefix.scheme().isEmpty())
	{
		return;
	}
	if(zone_prefix.host().isEmpty())
	{
		return;
	}
	QString zonePrefixString=zone_prefix.toString();
	qDebug() << zonePrefixString;
	QStringList *zonePrefixList=mCrawlingZones.value(zone_prefix.host(), nullptr);
	if(nullptr!=zonePrefixList)
	{
		if(!zonePrefixList->contains(zonePrefixString))
		{
			zonePrefixList->append(zonePrefixString);
		}
	}
	else
	{
		zonePrefixList=new QStringList;
		zonePrefixList->append(zonePrefixString);
		mCrawlingZones.insert(zone_prefix.host(), zonePrefixList);
	}
}

void Crawler::saveIndex()
{
	qDebug("Crawler::saveIndex");
	mIndexer->save(QStringLiteral(""));
}

void Crawler::loadIndex()
{
	qDebug("Crawler::loadIndex");
	mIndexer->load(QStringLiteral(""));
}

void Crawler::start()
{
	qDebug("Crawler::start");
	mWebPageProcessor->loadCookiesFromFirefoxProfile(mPathToFireFoxProfile);
	if(!mLoadingIntervalTimer->isActive())
	{
		mLoadingIntervalTimer->setInterval(mRNG->bounded(PAGE_LOADING_INTERVAL_MIN, PAGE_LOADING_INTERVAL_MAX));
		mLoadingIntervalTimer->start();
		emit started();
	}
}

void Crawler::stop()
{
	qDebug("Crawler::stop");
	mLoadingIntervalTimer->stop();
#ifndef NDEBUG
	qDebug() << "unvisited pages:";
	qDebug() << *mURLListActive;
	qDebug() << *mURLListQueued;
	qDebug() << "visited pages:";
	QSet<uint64_t>::const_iterator visitedURLsHashesIt;
	for(visitedURLsHashesIt=mVisitedURLsHashes.constBegin(); visitedURLsHashesIt!=mVisitedURLsHashes.constEnd(); visitedURLsHashesIt++)
	{
		const PageMetadata *pageMDPtr=mIndexer->getPageMetadataByUrlHash(*visitedURLsHashesIt);
		if(nullptr!=pageMDPtr)
		{
			qDebug() << pageMDPtr->url;
			qDebug() << pageMDPtr->title;
			qDebug() << pageMDPtr->timeStamp.toString();
			qDebug() << pageMDPtr->urlHash;
			qDebug() << pageMDPtr->contentHash;
			qDebug() << pageMDPtr->words.keys();
			qDebug() << "====";
		}
	}
#endif
	mURLListActive->clear();
	mURLListQueued->clear();
	emit finished();
}

#ifndef NDEBUG
void Crawler::searchTest()
{
	qDebug("Crawler::searchTest");
	QStringList words;
	words.append("meaning of life");
	const QVector<const PageMetadata *> searchResults=mIndexer->searchPagesByWords(words);
	for(const PageMetadata *page : searchResults)
	{
		qDebug() << page->url;
		qDebug() << page->title;
		qDebug() << page->timeStamp.toString();
		qDebug() << page->urlHash;
		qDebug() << page->contentHash;
		qDebug() << "====";
	}
}
#endif
