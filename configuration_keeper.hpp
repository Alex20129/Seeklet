#ifndef CONFIGURATION_KEEPER_HPP
#define CONFIGURATION_KEEPER_HPP

#include <QSize>
#include <QHash>
#include <QSet>
#include <QUrl>

class ConfigurationKeeper
{
	QString mHttpUserAgent;
	QString mDatabaseDirectory;
	QString mFirefoxProfileDirectory;
	QString mChromiumProfileDirectory;
	QSize mCrawlerWindowSize;
	int mHttpCacheSize=0;
	int mJsCompletionTimeout=3000;
	int mPageLoadingIntervalMin=4000;
	int mPageLoadingIntervalMax=8000;
	int mPagesPerSession=100;
	int mShowBrowserWindow=0;
	int mRemoteDebuggingPort=9222;
	bool mRemoteDebuggingEnabled=false;
	bool mLoadImages=true;
	QStringList mAllowedURLSchemes;
	QList<QUrl> mStartUrls;
	QSet<QString> mBlacklistedHosts;
	QHash<QString, QStringList> mCrawlingZones;

public:
	void setHttpCacheSize(int cache_size);
	int httpCacheSize() const;

	void setHttpUserAgent(const QString &http_user_agent);
	const QString &httpUserAgent() const;

	void setDatabaseDirectory(const QString &database_directory);
	const QString &databaseDirectory() const;

	void setFirefoxProfileDirectory(const QString &firefox_profile_directory);
	const QString &firefoxProfileDirectory() const;

	void setChromiumProfileDirectory(const QString &chromium_profile_directory);
	const QString &chromiumProfileDirectory() const;

	void setWindowWidth(int window_width);
	void setWindowHeight(int window_height);
	void setWindowSize(QSize window_size);
	const QSize &windowSize() const;

	void setJsCompletionTimeout(int js_completion_timeout);
	int jsCompletionTimeout() const;

	void setPageLoadingIntervalMin(int page_loading_interval_min);
	int pageLoadingIntervalMin() const;

	void setPageLoadingIntervalMax(int page_loading_interval_max);
	int pageLoadingIntervalMax() const;

	void setPagesPerSession(int pages_per_session);
	int pagesPerSession() const;

	void setShowBrowserWindow(int show_browser_window);
	int showBrowserWindow() const;

	void setRemoteDebuggingPort(int remote_debugging_port);
	int remoteDebuggingPort() const;

	void setRemoteDebuggingEnabled(bool remote_debugging_enabled);
	bool remoteDebuggingEnabled() const;

	void setLoadImages(bool load_images);
	bool loadImages() const;

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
