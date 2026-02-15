#include <QFile>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include "main.hpp"
#include "crawler.hpp"
#include "util.hpp"

QMap<QString, quint64> ExtractAndCountWords(const QString &text)
{
	const QString lowerText=text.toLower();
	static const QRegularExpression wordsRegex("[^a-zа-яё]+");
	static const QRegularExpression digitsRegex("^[0-9]+$");
	QMap<QString, quint64> wordMap;
	const QStringList words = lowerText.split(wordsRegex, Qt::SkipEmptyParts);
	for (const QString &word : words)
	{
		if (word.length()>2 && word.length()<33)
		{
			if (!digitsRegex.match(word).hasMatch())
			{
				wordMap[word] += 1;
			}
		}
	}
	return wordMap;
}

Crawler::Crawler(QObject *parent) : QObject(parent)
{
	uint32_t rngSeed=QDateTime::currentSecsSinceEpoch()+reinterpret_cast<uintptr_t>(this);
	mURLListActive=new QList<QUrl>;
	mURLListQueued=new QList<QUrl>;
	mRNG=new QRandomGenerator(rngSeed);
	mPageLoadingTimer=new QTimer(this);
	mPageLoadingTimer->setSingleShot(1);
	mWebPageProcessor=new WebPageProcessor(this);
	connect(mPageLoadingTimer, &QTimer::timeout, this, &Crawler::loadNextPage);
	connect(mWebPageProcessor, &WebPageProcessor::pageProcessingFinished, this, &Crawler::onPageProcessingFinished);
}

Crawler::~Crawler()
{
	stop();
	mURLListActive->clear();
	mURLListQueued->clear();
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
	QUrl nextURL = mURLListActive->takeAt(mRNG->bounded(0, mURLListActive->count()));
	qDebug() << nextURL.toString();
	qDebug() << mURLListActive->count()+mURLListQueued->count() << "URLs pending on the list";
	mWebPageProcessor->loadPage(nextURL);
}

void Crawler::onPageProcessingFinished()
{
	qDebug("Crawler::onPageProcessingFinished");

	const QString &pageContentText = mWebPageProcessor->getPageContentAsTEXT();
	const QString &pageContentHtml = mWebPageProcessor->getPageContentAsHTML();
	const QList<QUrl> &pageLinksList = mWebPageProcessor->getPageLinks();
	PageMetadata pageMetadata;

	pageMetadata.timeStamp = QDateTime::currentDateTime();
	pageMetadata.title = mWebPageProcessor->getPageTitle();
	pageMetadata.url = mWebPageProcessor->getPageURLEncoded(QUrl::RemoveFragment);
	pageMetadata.contentHash = hash_function_128(pageContentHtml.toUtf8());
	pageMetadata.urlHash = hash_function_128(pageMetadata.url);

	qDebug() << pageMetadata.title << "\n" << pageMetadata.url;

	QMap<QString, quint64> pageWords = ExtractAndCountWords(pageContentText);
	QMap<QString, quint64>::ConstIterator pageWordsIt;
	for(pageWordsIt=pageWords.constBegin(); pageWordsIt!=pageWords.constEnd(); pageWordsIt++)
	{
		const QString &pageWord=pageWordsIt.key();
		quint64 wordTf=pageWordsIt.value();
		if(wordTf>0 && !pageWord.isEmpty())
		{
			quint64 wordHash=hash_function_64(pageWord.toUtf8());
			pageMetadata.wordsAsHashes.insert(wordHash, wordTf);
			pageMetadata.wordsTotal+=wordTf;
			emit needToAddWord(pageWord);
		}
	}

	if(pageMetadata.wordsTotal>0)
	{
		emit needToAddPage(pageMetadata);
	}

	mVisitedURLsHashes.insert(pageMetadata.urlHash);

	addURLsToQueue(pageLinksList);

	if(gSettings->pageLoadingIntervalMin()<gSettings->pageLoadingIntervalMax())
	{
		mPageLoadingTimer->start(mRNG->bounded(gSettings->pageLoadingIntervalMin(), gSettings->pageLoadingIntervalMax()));
	}
	else
	{
		mPageLoadingTimer->start(gSettings->pageLoadingIntervalMin());
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
	QByteArray urlHash = hash_function_128(urlAdjusted.toEncoded());
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

void Crawler::start()
{
	qDebug("Crawler::start");
	mPagesRemaining=gSettings->pagesPerSession();
	addURLsToQueue(gSettings->startUrls());
	if(!mPageLoadingTimer->isActive())
	{
		mWebPageProcessor->loadCookiesFromFirefoxProfile(gSettings->fireFoxProfileDirectory());
		if(gSettings->pageLoadingIntervalMin()<gSettings->pageLoadingIntervalMax())
		{
			mPageLoadingTimer->start(mRNG->bounded(gSettings->pageLoadingIntervalMin(), gSettings->pageLoadingIntervalMax()));
		}
		else
		{
			mPageLoadingTimer->start(gSettings->pageLoadingIntervalMin());
		}
		emit started();
	}
}

void Crawler::stop()
{
	qDebug("Crawler::stop");
	mPageLoadingTimer->stop();
	mPagesRemaining=0;
}
