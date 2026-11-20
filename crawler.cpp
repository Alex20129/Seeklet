#include <QFile>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "main.hpp"
#include "crawler.hpp"
#include "xorshift_hash.hpp"

QMap<QString, quint64> ExtractAndCountWords(const QString &text)
{
	QString lowerText=text.toLower();
	lowerText.remove(QChar(0x0301));
	static const QRegularExpression wordsRegex("[^a-zа-яё0-9]+");
	QMap<QString, quint64> wordMap;
	const QStringList words=lowerText.split(wordsRegex, Qt::SkipEmptyParts);
	for (const QString &word : words)
	{
		if (word.length()>2 && word.length()<33)
		{
			wordMap[word]++;
		}
	}
	return wordMap;
}

Crawler::Crawler(QObject *parent) : QObject(parent)
{
	uint64_t rngSeed=QDateTime::currentSecsSinceEpoch()^reinterpret_cast<uint64_t>(this);
	mRNG.seed(rngSeed);
	mPageLoadingTimer.setSingleShot(true);
	mURLListActive=new QList<QUrl>;
	mURLListQueued=new QList<QUrl>;
	mWebPageProcessor=new WebPageProcessor(this);
	mWebPageProcessor->loadCookiesFromFirefox(gSettings->firefoxProfileDirectory());
	mWebPageProcessor->loadCookiesFromChromium(gSettings->chromiumProfileDirectory());
	connect(&mPageLoadingTimer, &QTimer::timeout, this, &Crawler::loadNextPage);
	connect(mWebPageProcessor, &WebPageProcessor::pageProcessingFinished, this, &Crawler::onPageProcessingFinished);
	connect(mWebPageProcessor, &WebPageProcessor::pageLoadingFail, this, &Crawler::scheduleNextPage);
}

Crawler::~Crawler()
{
	stop();
	mURLListActive->clear();
	mURLListQueued->clear();
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
	qDebug()<<"Pages remaining:"<<mPagesRemaining;
	if(mPagesRemaining>0)
	{
		mPagesRemaining--;
	}
	else
	{
		emit finished();
		return;
	}
	QUrl nextURL=mURLListActive->takeAt(mRNG.bounded(0, mURLListActive->count()));
	qDebug() << nextURL.toString();
	qDebug() << mURLListActive->count()+mURLListQueued->count() << "URLs pending on the list";
	Hash128 nextURLHash=xorshiftstar_hash_128(nextURL.toEncoded());
	mQueuedURLsHashes.remove(nextURLHash);
	mVisitedURLsHashes.insert(nextURLHash);
	mWebPageProcessor->loadPage(nextURL);
}

void Crawler::onPageProcessingFinished()
{
	qDebug("Crawler::onPageProcessingFinished");

	const QString &pageContentText=mWebPageProcessor->getPageContentAsTEXT();
	const QString &pageContentHtml=mWebPageProcessor->getPageContentAsHTML();
	const QList<QUrl> &pageLinksList=mWebPageProcessor->getPageLinks();
	PageMetadata pageMetadata;

	pageMetadata.timeStamp=QDateTime::currentDateTime();
	pageMetadata.title=mWebPageProcessor->getPageTitle();
	pageMetadata.url=mWebPageProcessor->getPageURLEncoded(QUrl::RemoveFragment);
	pageMetadata.contentHash=xorshiftstar_hash_128(pageContentHtml.toUtf8());

	qDebug() << pageMetadata.title << "\n" << pageMetadata.url;

	QMap<QString, quint64> pageWords=ExtractAndCountWords(pageContentText);
	QMap<QString, quint64>::ConstIterator pageWordsIt;
	for(pageWordsIt=pageWords.constBegin(); pageWordsIt!=pageWords.constEnd(); pageWordsIt++)
	{
		const QString &pageWord=pageWordsIt.key();
		quint64 wordTf=pageWordsIt.value();
		if(wordTf>0 && !pageWord.isEmpty())
		{
			Hash64 wordHash=xorshiftstar_hash_64(pageWord.toUtf8());
			pageMetadata.tfAsHashes.insert(wordHash, wordTf);
			pageMetadata.wordsTotal+=wordTf;
			emit needToAddWord(pageWord);
		}
	}

	if(pageMetadata.wordsTotal>0)
	{
		emit needToAddPage(pageMetadata);
	}
	addURLsToQueue(pageLinksList);
	scheduleNextPage();
}

void Crawler::scheduleNextPage()
{
	if(gSettings->pageLoadingIntervalMin()<gSettings->pageLoadingIntervalMax())
	{
		mPageLoadingTimer.start(mRNG.bounded(gSettings->pageLoadingIntervalMin(), gSettings->pageLoadingIntervalMax()));
	}
	else
	{
		mPageLoadingTimer.start(gSettings->pageLoadingIntervalMin());
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
	Hash128 urlHash=xorshiftstar_hash_128(urlAdjusted.toEncoded());
	qDebug() << urlAdjusted.toEncoded();
	if(mVisitedURLsHashes.contains(urlHash))
	{
		qDebug() << "Skipping visited page";
		return;
	}
	const QString lowerHost=urlAdjusted.host().toLower();
	if(gSettings->blacklistedHosts().contains(lowerHost))
	{
		qDebug() << "Skipping blacklisted host";
		return;
	}
	if(gSettings->crawlingZones().contains(lowerHost))
	{
		bool allowedByZonePrefix=0;
		const QStringList &zonePrefixes=gSettings->crawlingZones()[lowerHost];
		for(const QString &zonePrefix : zonePrefixes)
		{
			if(!zonePrefix.isEmpty())
			{
				if(urlAdjusted.toString().startsWith(zonePrefix))
				{
					allowedByZonePrefix=1;
					break;
				}
			}
		}
		if(!allowedByZonePrefix)
		{
			qDebug() << "Skipping page outside crawling zone";
			return;
		}
	}
	if(!gSettings->allowedUrlSchemes().isEmpty())
	{
		if(!gSettings->allowedUrlSchemes().contains(urlAdjusted.scheme()))
		{
			qDebug() << "Skipping URL due to inacceptable scheme:" << urlAdjusted.scheme();
			return;
		}
	}
	if(mQueuedURLsHashes.contains(urlHash))
	{
		qDebug() << "Skipping duplicate URL";
		return;
	}
	qDebug() << "Adding URL to the processing list";
	mURLListQueued->append(urlAdjusted);
	mQueuedURLsHashes.insert(urlHash);
}

void Crawler::markURLAsVisited(const QUrl &url)
{
	qDebug("Crawler::markURLAsVisited");
	QUrl urlAdjusted=url.adjusted(QUrl::RemoveFragment);
	Hash128 urlHash=xorshiftstar_hash_128(urlAdjusted.toEncoded());
	qDebug() << urlAdjusted.toEncoded();
	mVisitedURLsHashes.insert(urlHash);
}

void Crawler::start()
{
	qDebug("Crawler::start");
	mPagesRemaining=gSettings->pagesPerSession();
	addURLsToQueue(gSettings->startUrls());
	if(!mPageLoadingTimer.isActive())
	{
		if(gSettings->pageLoadingIntervalMin()<gSettings->pageLoadingIntervalMax())
		{
			mPageLoadingTimer.start(mRNG.bounded(gSettings->pageLoadingIntervalMin(), gSettings->pageLoadingIntervalMax()));
		}
		else
		{
			mPageLoadingTimer.start(gSettings->pageLoadingIntervalMin());
		}
		emit started();
	}
}

void Crawler::stop()
{
	qDebug("Crawler::stop");
	mPageLoadingTimer.stop();
	mPagesRemaining=0;
}
