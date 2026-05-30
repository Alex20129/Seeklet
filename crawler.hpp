#ifndef CRAWLER_HPP
#define CRAWLER_HPP

#include <QRandomGenerator>
#include "web_page_processor.hpp"
#include "page_metadata.hpp"

class Crawler : public QObject
{
	Q_OBJECT
	QRandomGenerator mRNG;
	QTimer mPageLoadingTimer;
	QSet<Hash128> mVisitedURLsHashes;
	uint64_t mPagesRemaining;
	WebPageProcessor *mWebPageProcessor;
	QList<QUrl> *mURLListActive, *mURLListQueued;
private slots:
	void loadNextPage();
	void onPageProcessingFinished();
public:
	Crawler(QObject *parent=nullptr);
	~Crawler();
	void addURLsToQueue(const QList<QUrl> &urls);
	void addURLToQueue(const QUrl &url);
public slots:
	void start();
	void stop();
signals:
	void started();
	void finished();
	void needToAddPage(PageMetadata page_metadata);
	void needToAddWord(QString word);
};

#endif // CRAWLER_HPP
