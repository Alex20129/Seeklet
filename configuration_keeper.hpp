#ifndef CONFIGURATION_KEEPER_HPP
#define CONFIGURATION_KEEPER_HPP

#include <QObject>
#include <QSize>
#include <QStringList>
#include <QHash>
#include <QSet>

class ConfigurationKeeper : public QObject
{
	Q_OBJECT
	QString mHttpUserAgent;
	QString mDatabaseDirectory;
	QString mFireFoxProfileDirectory;
	QSize mCrawlerWindowSize;
	int mJsCompletionTimeout;
	int mPageLoadingIntervalMin;
	int mPageLoadingIntervalMax;
	int mPagesPerSessionMax;
	QStringList mAllowedURLSchemes;
	QList<QUrl> mStartUrls;
	QSet<QString> mBlacklistedHosts;
	QHash<QString, QStringList> mCrawlingZones;
public:
	ConfigurationKeeper(QObject *parent = nullptr);
	~ConfigurationKeeper();

	void setHttpUserAgent(const QString &http_user_agent);
	const QString &httpUserAgent() const;

	void setDatabaseDirectory(const QString &database_directory);
	const QString &databaseDirectory() const;

	void setFireFoxProfileDirectory(const QString &firefox_profile_directory);
	const QString &fireFoxProfileDirectory() const;

	void setCrawlerWindowWidth(int crawler_window_width);
	void setCrawlerWindowHeight(int crawler_window_height);
	void setCrawlerWindowSize(const QSize &crawler_window_size);
	const QSize &crawlerWindowSize() const;

	void setJsCompletionTimeout(int js_completion_timeout);
	int jsCompletionTimeout() const;

	void setPageLoadingIntervalMin(int page_loading_interval_min);
	int pageLoadingIntervalMin() const;

	void setPageLoadingIntervalMax(int page_loading_interval_max);
	int pageLoadingIntervalMax() const;

	void setPagesPerSession(int pages_per_session);
	int pagesPerSession() const;

	void addAllowedUrlScheme(const QString &allowed_url_scheme);
	void removeAllowedUrlScheme(const QString &allowed_url_scheme);
	const QStringList &allowedUrlSchemes() const;

	void addStartUrl(const QUrl &start_url);
	void removeStartUrl(const QUrl &start_url);
	const QList<QUrl> &startUrls() const;

	void addBlacklistedHost(const QString &blacklisted_host);
	void removeBlacklistedHost(const QString &blacklisted_host);
	const QSet<QString> &blacklistedHosts() const;

	void addCrawlingZone(const QUrl &crawling_zone);
	void removeCrawlingZone(const QUrl &crawling_zone);
	const QHash<QString, QStringList> &crawlingZones()const;

	void loadSettingsFromJsonFile(const QString &path_to_file);
	void saveSettingsToJsonFile(const QString &path_to_file) const;
};

#endif // CONFIGURATION_KEEPER_HPP
