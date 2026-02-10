#ifndef CRAWLER_HPP
#define CRAWLER_HPP

#include <QTimer>
#include <QRandomGenerator>
#include "web_page_processor.hpp"
#include "indexer.hpp"

class Crawler : public QObject
{
	Q_OBJECT
	uint64_t mPagesRemaining;
	QRandomGenerator *mRNG;
	QTimer *mPageLoadingTimer;
	WebPageProcessor *mWebPageProcessor;
	QList<QUrl> *mURLListActive, *mURLListQueued;
	QSet<uint64_t> mVisitedURLsHashes;
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
