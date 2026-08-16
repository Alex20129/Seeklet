#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "configuration_keeper.hpp"

void ConfigurationKeeper::setHttpCacheSize(int cache_size)
{
	if(cache_size<0)
	{
		cache_size=0;
	}
	mHttpCacheSize=cache_size;
}

int ConfigurationKeeper::httpCacheSize() const
{
	return mHttpCacheSize;
}

void ConfigurationKeeper::setHttpUserAgent(const QString &user_agent)
{
	mHttpUserAgent=user_agent;
}

const QString &ConfigurationKeeper::httpUserAgent() const
{
	return mHttpUserAgent;
}

void ConfigurationKeeper::setDatabaseDirectory(const QString &database_directory)
{
	mDatabaseDirectory=database_directory;
}

const QString &ConfigurationKeeper::databaseDirectory() const
{
	return mDatabaseDirectory;
}

void ConfigurationKeeper::setFirefoxProfileDirectory(const QString &firefox_profile_directory)
{
	mFirefoxProfileDirectory=firefox_profile_directory;
}

const QString &ConfigurationKeeper::firefoxProfileDirectory() const
{
	return mFirefoxProfileDirectory;
}

void ConfigurationKeeper::setChromiumProfileDirectory(const QString &chromium_profile_directory)
{
	mChromiumProfileDirectory=chromium_profile_directory;
}

const QString &ConfigurationKeeper::chromiumProfileDirectory() const
{
	return mChromiumProfileDirectory;
}

void ConfigurationKeeper::setWindowWidth(int window_width)
{
	if(window_width<0)
	{
		window_width=0;
	}
	mCrawlerWindowSize.setWidth(window_width);
}

void ConfigurationKeeper::setWindowHeight(int window_height)
{
	if(window_height<0)
	{
		window_height=0;
	}
	mCrawlerWindowSize.setHeight(window_height);
}

void ConfigurationKeeper::setWindowSize(QSize window_size)
{
	if(window_size.width()<0)
	{
		window_size.setWidth(0);
	}
	if(window_size.height()<0)
	{
		window_size.setHeight(0);
	}
	mCrawlerWindowSize=window_size;
}

const QSize &ConfigurationKeeper::windowSize() const
{
	return mCrawlerWindowSize;
}

void ConfigurationKeeper::setJsCompletionTimeout(int js_completion_timeout)
{
	if(js_completion_timeout<10)
	{
		js_completion_timeout=10;
	}
	mJsCompletionTimeout=js_completion_timeout;
}

int ConfigurationKeeper::jsCompletionTimeout() const
{
	return mJsCompletionTimeout;
}

void ConfigurationKeeper::setPageLoadingIntervalMin(int page_loading_interval_min)
{
	if(page_loading_interval_min<1)
	{
		page_loading_interval_min=1;
	}
	mPageLoadingIntervalMin=page_loading_interval_min;
	if(mPageLoadingIntervalMin>mPageLoadingIntervalMax)
	{
		mPageLoadingIntervalMax=page_loading_interval_min;
	}
}

int ConfigurationKeeper::pageLoadingIntervalMin() const
{
	return mPageLoadingIntervalMin;
}

void ConfigurationKeeper::setPageLoadingIntervalMax(int page_loading_interval_max)
{
	if(page_loading_interval_max<1)
	{
		page_loading_interval_max=1;
	}
	mPageLoadingIntervalMax=page_loading_interval_max;
	if(mPageLoadingIntervalMax<mPageLoadingIntervalMin)
	{
		mPageLoadingIntervalMin=page_loading_interval_max;
	}
}

int ConfigurationKeeper::pageLoadingIntervalMax() const
{
	return mPageLoadingIntervalMax;
}

void ConfigurationKeeper::setPagesPerSession(int pages_per_session)
{
	if(pages_per_session<0)
	{
		pages_per_session=-1;
	}
	mPagesPerSession=pages_per_session;
}

int ConfigurationKeeper::pagesPerSession() const
{
	return mPagesPerSession;
}

void ConfigurationKeeper::setShowBrowserWindow(int show_browser_window)
{
	if(show_browser_window<0)
	{
		show_browser_window=0;
	}
	else if(show_browser_window>2)
	{
		show_browser_window=2;
	}
	mShowBrowserWindow=show_browser_window;
}

int ConfigurationKeeper::showBrowserWindow() const
{
	return mShowBrowserWindow;
}

void ConfigurationKeeper::setRemoteDebuggingPort(int remote_debugging_port)
{
	if(remote_debugging_port<0)
	{
		remote_debugging_port=0;
	}
	else if(remote_debugging_port>65535)
	{
		remote_debugging_port=65535;
	}
	mRemoteDebuggingPort=remote_debugging_port;
}

int ConfigurationKeeper::remoteDebuggingPort() const
{
	return mRemoteDebuggingPort;
}

void ConfigurationKeeper::setRemoteDebuggingEnabled(bool remote_debugging_enabled)
{
	mRemoteDebuggingEnabled=remote_debugging_enabled;
}

bool ConfigurationKeeper::remoteDebuggingEnabled() const
{
	return mRemoteDebuggingEnabled;
}

void ConfigurationKeeper::setLoadImages(bool load_images)
{
	mLoadImages=load_images;
}

bool ConfigurationKeeper::loadImages() const
{
	return mLoadImages;
}

void ConfigurationKeeper::addAllowedUrlScheme(const QString &allowed_url_scheme)
{
	if(allowed_url_scheme.isEmpty())
	{
		return;
	}
	if(!mAllowedURLSchemes.contains(allowed_url_scheme))
	{
		mAllowedURLSchemes.append(allowed_url_scheme);
	}
}

void ConfigurationKeeper::removeAllowedUrlScheme(const QString &allowed_url_scheme)
{
	mAllowedURLSchemes.removeAll(allowed_url_scheme);
}

const QStringList &ConfigurationKeeper::allowedUrlSchemes() const
{
	return mAllowedURLSchemes;
}

void ConfigurationKeeper::addStartUrl(const QUrl &start_url)
{
	if(!start_url.isValid())
	{
		return;
	}
	if(!mStartUrls.contains(start_url))
	{
		mStartUrls.append(start_url);
	}
}

void ConfigurationKeeper::removeStartUrl(const QUrl &start_url)
{
	mStartUrls.removeAll(start_url);
}

const QList<QUrl> &ConfigurationKeeper::startUrls() const
{
	return mStartUrls;
}

void ConfigurationKeeper::addBlacklistedHost(const QString &blacklisted_host)
{
	if(blacklisted_host.isEmpty())
	{
		return;
	}
	if(!mBlacklistedHosts.contains(blacklisted_host))
	{
		mBlacklistedHosts.insert(blacklisted_host);
	}
}

void ConfigurationKeeper::removeBlacklistedHost(const QString &blacklisted_host)
{
	mBlacklistedHosts.remove(blacklisted_host);
}

const QSet<QString> &ConfigurationKeeper::blacklistedHosts() const
{
	return mBlacklistedHosts;
}

void ConfigurationKeeper::addCrawlingZone(const QUrl &crawling_zone)
{
	if(crawling_zone.host().isEmpty())
	{
		return;
	}
	if(!mCrawlingZones[crawling_zone.host()].contains(crawling_zone.toString()))
	{
		mCrawlingZones[crawling_zone.host()].append(crawling_zone.toString());
	}
}

void ConfigurationKeeper::removeCrawlingZone(const QUrl &crawling_zone)
{
	if(crawling_zone.host().isEmpty())
	{
		return;
	}
	mCrawlingZones[crawling_zone.host()].removeAll(crawling_zone.toString());
	if(mCrawlingZones[crawling_zone.host()].isEmpty())
	{
		mCrawlingZones.remove(crawling_zone.host());
	}
}

const QHash<QString, QStringList> &ConfigurationKeeper::crawlingZones() const
{
	return mCrawlingZones;
}

void ConfigurationKeeper::loadSettingsFromJsonFile(const QString &path_to_file)
{
	if(path_to_file.isEmpty())
	{
		return;
	}
	QFile configFile(path_to_file);
	if (!configFile.exists())
	{
		return;
	}
	if(!configFile.open(QIODevice::ReadOnly))
	{
		return;
	}

	QByteArray configData=configFile.readAll();
	configFile.close();

	QJsonParseError err;
	QJsonDocument configJsonDoc=QJsonDocument::fromJson(configData, &err);

	if (err.error != QJsonParseError::NoError)
	{
		return;
	}

	if(!configJsonDoc.isObject())
	{
		return;
	}

	QJsonObject configJsonObject=configJsonDoc.object();

	if(configJsonObject.value("http_cache_size").isDouble())
	{
		this->setHttpCacheSize(configJsonObject.value("http_cache_size").toInt());
	}
	if(configJsonObject.value("http_user_agent").isString())
	{
		this->setHttpUserAgent(configJsonObject.value("http_user_agent").toString());
	}
	if(configJsonObject.value("database_directory").isString())
	{
		this->setDatabaseDirectory(configJsonObject.value("database_directory").toString());
	}
	if(configJsonObject.value("firefox_profile_directory").isString())
	{
		this->setFirefoxProfileDirectory(configJsonObject.value("firefox_profile_directory").toString());
	}
	if(configJsonObject.value("chromium_profile_directory").isString())
	{
		this->setChromiumProfileDirectory(configJsonObject.value("chromium_profile_directory").toString());
	}
	if(configJsonObject.value("window_width").isDouble())
	{
		this->setWindowWidth(configJsonObject.value("window_width").toInt());
	}
	if(configJsonObject.value("window_height").isDouble())
	{
		this->setWindowHeight(configJsonObject.value("window_height").toInt());
	}
	if(configJsonObject.value("js_completion_timeout").isDouble())
	{
		this->setJsCompletionTimeout(configJsonObject.value("js_completion_timeout").toDouble());
	}
	if(configJsonObject.value("page_loading_interval_min").isDouble())
	{
		this->setPageLoadingIntervalMin(configJsonObject.value("page_loading_interval_min").toDouble());
	}
	if(configJsonObject.value("page_loading_interval_max").isDouble())
	{
		this->setPageLoadingIntervalMax(configJsonObject.value("page_loading_interval_max").toDouble());
	}
	if(configJsonObject.value("pages_per_session").isDouble())
	{
		this->setPagesPerSession(configJsonObject.value("pages_per_session").toInt());
	}
	if(configJsonObject.value("show_browser_window").isDouble())
	{
		this->setShowBrowserWindow(configJsonObject.value("show_browser_window").toInt());
	}
	if(configJsonObject.value("remote_debugging_port").isDouble())
	{
		this->setRemoteDebuggingPort(configJsonObject.value("remote_debugging_port").toInt());
	}
	if(configJsonObject.value("remote_debugging_enabled").isBool())
	{
		this->setRemoteDebuggingEnabled(configJsonObject.value("remote_debugging_enabled").toBool());
	}
	if(configJsonObject.value("load_images").isBool())
	{
		this->setLoadImages(configJsonObject.value("load_images").toBool());
	}

	if(configJsonObject.value("allowed_url_schemes").isArray())
	{
		const QJsonArray &allowedUrlSchemes=configJsonObject.value("allowed_url_schemes").toArray();
		mAllowedURLSchemes.clear();
		for(const QJsonValue &allowedUrlScheme : allowedUrlSchemes)
		{
			if(allowedUrlScheme.isString())
			{
				this->addAllowedUrlScheme(allowedUrlScheme.toString());
			}
		}
	}

	if(configJsonObject.value("start_urls").isArray())
	{
		const QJsonArray &startUrls=configJsonObject.value("start_urls").toArray();
		mStartUrls.clear();
		for(const QJsonValue &startUrl : startUrls)
		{
			if(startUrl.isString())
			{
				this->addStartUrl(startUrl.toString());
			}
		}
	}

	if(configJsonObject.value("black_list").isArray())
	{
		const QJsonArray &blacklistedHosts=configJsonObject.value("black_list").toArray();
		mBlacklistedHosts.clear();
		for(const QJsonValue &blacklistedHost : blacklistedHosts)
		{
			if(blacklistedHost.isString())
			{
				this->addBlacklistedHost(blacklistedHost.toString());
			}
		}
	}

	if(configJsonObject.value("crawling_zones").isArray())
	{
		const QJsonArray &crawlingZones=configJsonObject.value("crawling_zones").toArray();
		mCrawlingZones.clear();
		for(const QJsonValue &crawlingZone : crawlingZones)
		{
			if(crawlingZone.isString())
			{
				this->addCrawlingZone(crawlingZone.toString());
			}
		}
	}
}

void ConfigurationKeeper::saveSettingsToJsonFile(const QString &path_to_file) const
{
	uint64_t WIP; // TODO: save as JSON
	if(path_to_file.isEmpty())
	{
		return;
	}
}
