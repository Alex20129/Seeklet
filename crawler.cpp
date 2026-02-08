#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "configuration_keeper.hpp"
#include "crawler.hpp"
#include "util.hpp"

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
	addURLsToQueue(gSettings->startUrls());
}

Crawler::~Crawler()
{
	stop();
	delete mRNG;
	delete mURLListQueued;
	delete mURLListActive;
}

void Crawler::loadNextPage()
{
	qDebug("Crawler::loadNextPage");
	if(mURLListActive->isEmpty())
	{
		qSwap(mURLListActive, mURLListQueued);
		if(mURLListActive->isEmpty())
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
	if(pageHTMLFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		pageHTMLFile.write(pageContentHtml.toUtf8());
		pageHTMLFile.close();
	}
	else
	{
		qWarning() << "Failed to open page.html";
	}

	QFile pageTXTFile(QString("page_")+QString::number(pageMetadata.contentHash&UINT32_MAX, 16).toUpper() + QString(".txt"));
	if(pageTXTFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		pageTXTFile.write(pageContentText.toUtf8());
		pageTXTFile.close();
	}
	else
	{
		qWarning() << "Failed to open page.txt";
	}

	QFile pageLinksFile(QString("page_")+QString::number(pageMetadata.contentHash&UINT32_MAX, 16).toUpper() + QString("_links.txt"));
	if(pageLinksFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
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
	if(pageWordsFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
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
	for(const QUrl &url : urls)
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
	if(gSettings->blacklistedHosts().contains(urlAdjusted.host()))
	{
		skipThisURL=true;
		qDebug() << "Skipping blacklisted host";
	}
	else if(mVisitedURLsHashes.contains(urlHash))
	{
		skipThisURL=true;
		qDebug() << "Skipping visited page";
	}
	bool urlAllowedByZonePrefix=1;
	if(gSettings->crawlingZones().contains(urlAdjusted.host()))
	{
		urlAllowedByZonePrefix=0;
		const QStringList &zonePrefixList=gSettings->crawlingZones()[urlAdjusted.host()];
		for(const QString &zonePrefix : zonePrefixList)
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
	if(!gSettings->allowedUrlSchemes().isEmpty())
	{
		if(!gSettings->allowedUrlSchemes().contains(urlAdjusted.scheme()))
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
	mWebPageProcessor->loadCookiesFromFirefoxProfile(gSettings->fireFoxProfileDirectory());
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
