#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "util.hpp"
#include "crawler.hpp"
#include "simple_hash_func.hpp"

QSet<QString> Crawler::sVisitedURLList;
QSet<QString> Crawler::sHostnameBlacklist;

QMutex Crawler::sUnwantedLinksMutex;

Crawler::Crawler(QObject *parent) : QObject(parent)
{
	uint32_t rngSeed=QDateTime::currentSecsSinceEpoch()+reinterpret_cast<uintptr_t>(this);
	mURLListActive=new QList<QUrl>;
	mURLListQueued=new QList<QUrl>;
	mRNG=new QRandomGenerator(rngSeed);
	mLoadingIntervalTimer=new QTimer(this);
	mWebPageProcessor=new WebPageProcessor(this);
	mIndexer=new Indexer(this);
	mIndexer->initialize("in_test.bin");
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

void Crawler::swapURLLists()
{
	qSwap(mURLListActive, mURLListQueued);
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

	QJsonObject rawlerConfigJsonObject = crawlerConfigJsonDoc.object();

	QStringList schemes;
	const QJsonArray &allowedURLSchemes=rawlerConfigJsonObject.value("allowed_url_schemes").toArray();
	for (const QJsonValue &allowedURLScheme : allowedURLSchemes)
	{
		QString schemeString=allowedURLScheme.toString();
		if(!schemeString.isEmpty())
		{
			schemes.append(schemeString);
		}
	}
	this->setAllowedURLSchemes(schemes);

	const QJsonArray &startURLs=rawlerConfigJsonObject.value("start_urls").toArray();
	for (const QJsonValue &startURL : startURLs)
	{
		this->addURLToQueue(startURL.toString());
	}

	const QJsonArray &crawlingZones=rawlerConfigJsonObject.value("crawling_zones").toArray();
	for (const QJsonValue &crawlingZone : crawlingZones)
	{
		this->addCrawlingZone(crawlingZone.toString());
	}

	const QJsonArray &blHosts=rawlerConfigJsonObject.value("black_list").toArray();
	for (const QJsonValue &blHost : blHosts)
	{
		this->addHostnameToBlacklist(blHost.toString());
	}

	return 0;
}

void Crawler::setPathToFirefoxProfile(const QString &path)
{
	qDebug("Crawler::setPathToFirefoxProfile");
	mPathToFireFoxProfile=path;
	qDebug()<<path;
}

void Crawler::setAllowedURLSchemes(const QStringList &schemes)
{
	qDebug("Crawler::setAllowedURLSchemes");
	mAllowedURLSchemes=schemes;
	qDebug()<<mAllowedURLSchemes;
}

const Indexer *Crawler::getIndexer() const
{
	return mIndexer;
}

void Crawler::loadNextPage()
{
	qDebug("Crawler::loadNextPage");
	if (mURLListActive->isEmpty())
	{
		swapURLLists();
		if (mURLListActive->isEmpty())
		{
			emit finished(this);
			return;
		}
	}
	QUrl nextURL = mURLListActive->takeAt(mRNG->bounded(0, mURLListActive->count()));
	qDebug() << nextURL.toString();
	sUnwantedLinksMutex.lock();
	sVisitedURLList.insert(nextURL.toString());
	sUnwantedLinksMutex.unlock();
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

	pageMetadata.contentHash = xorshift_hash_64((const uint8_t *)pageContentHtml.toStdString().data(), pageContentHtml.toStdString().size());
	pageMetadata.timeStamp = QDateTime::currentDateTime();
	pageMetadata.title = mWebPageProcessor->getPageTitle();
	pageMetadata.url = mWebPageProcessor->getPageURLEncoded();
	pageMetadata.words = ExtractWordsAndFrequencies(pageContentText);

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
#endif

#ifndef NDEBUG
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
	QString URLString=url.toString();
	bool skipThisURL=0;
	qDebug() << URLString;
	sUnwantedLinksMutex.lock();
	if (sHostnameBlacklist.contains(url.host()))
	{
		skipThisURL=1;
		qDebug() << "Skipping blacklisted host";
	}
	else if (sVisitedURLList.contains(URLString))
	{
		skipThisURL=1;
		qDebug() << "Skipping visited page";
	}
	sUnwantedLinksMutex.unlock();

	bool urlAllowedByZonePrefix=1;
	const QStringList* const zonePrefixList=mCrawlingZones.value(url.host(), nullptr);
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
		skipThisURL=1;
		qDebug() << "Skipping page outside crawling zone";
	}
	if(!mAllowedURLSchemes.isEmpty())
	{
		if(!mAllowedURLSchemes.contains(url.scheme()))
		{
			skipThisURL=1;
			qDebug() << "Skipping URL due to inacceptable scheme:" << url.scheme();
		}
	}
	if(!skipThisURL)
	{
		if(mURLListQueued->contains(url))
		{
			qDebug() << "Skipping duplicate URL";
		}
		else
		{
			qDebug() << "Adding URL to the processing list";
			mURLListQueued->append(url);
		}
	}
}

void Crawler::addHostnameToBlacklist(const QString &hostname)
{
	qDebug("Crawler::addHostnameToBlacklist");
	sUnwantedLinksMutex.lock();
	if (!sHostnameBlacklist.contains(hostname))
	{
		sHostnameBlacklist.insert(hostname);
		qDebug() << "Host name has been added to the blacklist:" << hostname;
	}
	sUnwantedLinksMutex.unlock();
}

void Crawler::addCrawlingZone(const QUrl &zone_prefix)
{
	qDebug("Crawler::addCrawlingZone");
	if(zone_prefix.isEmpty())
	{
		return;
	}
	if(zone_prefix.host().isEmpty())
	{
		return;
	}
	if(!zone_prefix.isValid())
	{
		return;
	}
	QString zonePrefixString=zone_prefix.toString();
	qDebug() << "Prefix:" << zonePrefixString;
	qDebug() << "Host:" << zone_prefix.host();
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

void Crawler::start()
{
	qDebug("Crawler::start");
	mWebPageProcessor->loadCookiesFromFirefoxProfile(mPathToFireFoxProfile);
	if(!mLoadingIntervalTimer->isActive())
	{
		mLoadingIntervalTimer->setInterval(mRNG->bounded(PAGE_LOADING_INTERVAL_MIN, PAGE_LOADING_INTERVAL_MAX));
		mLoadingIntervalTimer->start();
		emit started(this);
	}
}

void Crawler::stop()
{
	qDebug("Crawler::stop");
	mLoadingIntervalTimer->stop();
	qDebug() << "unvisited pages:";
	qDebug() << *mURLListActive;
	qDebug() << *mURLListQueued;
	mURLListActive->clear();
	mURLListQueued->clear();
	sUnwantedLinksMutex.lock();
	qDebug() << "Visited Pages:";
	qDebug() << sVisitedURLList.values();
	sUnwantedLinksMutex.unlock();
	emit finished(this);
}

void Crawler::searchTest()
{
	qDebug("Crawler::searchTest");
	QStringList words;
	words.append("qtwebengine");
	const QVector<const PageMetadata*> searchResults=mIndexer->searchByWords(words);
	for(const PageMetadata *page : searchResults)
	{
		qDebug() << page->contentHash;
		qDebug() << page->title;
		qDebug() << page->timeStamp;
		qDebug() << page->url;
		qDebug() << "====";
	}
}
