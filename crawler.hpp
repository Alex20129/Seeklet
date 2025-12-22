#ifndef CRAWLER_HPP
#define CRAWLER_HPP

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QRandomGenerator>
#include "web_page_processor.hpp"
#include "indexer.hpp"

class Crawler : public QObject
{
	Q_OBJECT
	QString mPathToFireFoxProfile;
	QStringList mAllowedURLSchemes;
	QRandomGenerator *mRNG;
	QTimer *mLoadingIntervalTimer;
	WebPageProcessor *mWebPageProcessor;
	Indexer *mIndexer;
	QList<QUrl> *mURLListActive, *mURLListQueued;
	QHash<QString, QStringList*> mCrawlingZones;
	QSet<uint64_t> mVisitedURLsHashes;
	static QSet<QString> sHostnameBlacklist;
	void swapURLLists();
private slots:
	void loadNextPage();
	void onPageProcessingFinished();
public:
	static constexpr int PAGE_LOADING_INTERVAL_MIN=1024;
	static constexpr int PAGE_LOADING_INTERVAL_MAX=4096;
	Crawler(QObject *parent=nullptr);
	~Crawler();
	int loadSettingsFromJSONFile(const QString &path_to_file);
	void setPathToFirefoxProfile(const QString &path_to_ff_profile);
	void setHttpUserAgent(const QString &user_agent);
	void setWindowSize(const QSize &window_size);
	void addAllowedURLScheme(const QString &scheme);
	const Indexer *getIndexer() const;
	void addURLsToQueue(const QList<QUrl> &urls);
	void addURLToQueue(const QUrl &url);
	void addHostnameToBlacklist(const QString &hostname);
	void addCrawlingZone(const QUrl &zone_url);
public slots:
	void start();
	void stop();
	void searchTest();
signals:
	void started(Crawler *crawler);
	void finished(Crawler *crawler);
	void needToIndexNewPage(PageMetadata page_metadata);
};

#endif // CRAWLER_HPP
