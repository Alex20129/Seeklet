#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "configuration_keeper.hpp"

ConfigurationKeeper::ConfigurationKeeper()
{
	setHttpCacheSize(mHttpCacheSize);
	setHttpUserAgent(mHttpUserAgent);
	setDatabaseDirectory(mDatabaseDirectory);
	setFirefoxProfileDirectory(mFirefoxProfileDirectory);
	setChromiumProfileDirectory(mChromiumProfileDirectory);
	setWindowSize(QSize(0, 0));
	setJsCompletionTimeout(mJsCompletionTimeout);
	setPageLoadingIntervalMin(mPageLoadingIntervalMin);
	setPageLoadingIntervalMax(mPageLoadingIntervalMax);
	setPagesPerSession(mPagesPerSession);
	setShowBrowserWindow(mShowBrowserWindow);
	setRemoteDebuggingPort(mRemoteDebuggingPort);
	setRemoteDebuggingEnabled(mRemoteDebuggingEnabled);
	setLoadImages(mLoadImages);
	mSettingsJson["start_urls"]=QJsonArray();
	mSettingsJson["allowed_url_schemes"]=QJsonArray();
	mSettingsJson["black_list"]=QJsonArray();
	mSettingsJson["crawling_zones"]=QJsonArray();
}

void ConfigurationKeeper::setHttpCacheSize(int cache_size)
{
	if(cache_size<0)
	{
		cache_size=0;
	}
	mHttpCacheSize=cache_size;
	mSettingsJson["http_cache_size"]=mHttpCacheSize;
}

int ConfigurationKeeper::httpCacheSize() const
{
	return mHttpCacheSize;
}

void ConfigurationKeeper::setHttpUserAgent(const QString &user_agent)
{
	mHttpUserAgent=user_agent;
	mSettingsJson["http_user_agent"]=mHttpUserAgent;
}

const QString &ConfigurationKeeper::httpUserAgent() const
{
	return mHttpUserAgent;
}

void ConfigurationKeeper::setDatabaseDirectory(const QString &database_directory)
{
	mDatabaseDirectory=database_directory;
	mSettingsJson["database_directory"]=mDatabaseDirectory;
}

const QString &ConfigurationKeeper::databaseDirectory() const
{
	return mDatabaseDirectory;
}

void ConfigurationKeeper::setFirefoxProfileDirectory(const QString &firefox_profile_directory)
{
	mFirefoxProfileDirectory=firefox_profile_directory;
	mSettingsJson["firefox_profile_directory"]=mFirefoxProfileDirectory;
}

const QString &ConfigurationKeeper::firefoxProfileDirectory() const
{
	return mFirefoxProfileDirectory;
}

void ConfigurationKeeper::setChromiumProfileDirectory(const QString &chromium_profile_directory)
{
	mChromiumProfileDirectory=chromium_profile_directory;
	mSettingsJson["chromium_profile_directory"]=mChromiumProfileDirectory;
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
	mSettingsJson["window_width"]=mCrawlerWindowSize.width();
}

void ConfigurationKeeper::setWindowHeight(int window_height)
{
	if(window_height<0)
	{
		window_height=0;
	}
	mCrawlerWindowSize.setHeight(window_height);
	mSettingsJson["window_height"]=mCrawlerWindowSize.height();
}

void ConfigurationKeeper::setWindowSize(QSize window_size)
{
	setWindowWidth(window_size.width());
	setWindowHeight(window_size.height());
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
	mSettingsJson["js_completion_timeout"]=mJsCompletionTimeout;
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
	mSettingsJson["page_loading_interval_min"]=mPageLoadingIntervalMin;
	mSettingsJson["page_loading_interval_max"]=mPageLoadingIntervalMax;
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
	mSettingsJson["page_loading_interval_min"]=mPageLoadingIntervalMin;
	mSettingsJson["page_loading_interval_max"]=mPageLoadingIntervalMax;
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
	mSettingsJson["pages_per_session"]=mPagesPerSession;
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
	mSettingsJson["show_browser_window"]=mShowBrowserWindow;
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
	mSettingsJson["remote_debugging_port"]=mRemoteDebuggingPort;
}

int ConfigurationKeeper::remoteDebuggingPort() const
{
	return mRemoteDebuggingPort;
}

void ConfigurationKeeper::setRemoteDebuggingEnabled(bool remote_debugging_enabled)
{
	mRemoteDebuggingEnabled=remote_debugging_enabled;
	mSettingsJson["remote_debugging_enabled"]=mRemoteDebuggingEnabled;
}

bool ConfigurationKeeper::remoteDebuggingEnabled() const
{
	return mRemoteDebuggingEnabled;
}

void ConfigurationKeeper::setLoadImages(bool load_images)
{
	mLoadImages=load_images;
	mSettingsJson["load_images"]=mLoadImages;
}

bool ConfigurationKeeper::loadImages() const
{
	return mLoadImages;
}

void ConfigurationKeeper::appendJsonEntry(const QString &key, const QString &entry)
{
	QJsonArray entries=mSettingsJson.take(key).toArray();
	entries.append(entry);
	mSettingsJson.insert(key, entries);
}

void ConfigurationKeeper::removeJsonEntry(const QString &key, const QString &entry)
{
	QJsonArray entries=mSettingsJson.take(key).toArray();
	for(qsizetype index=0; index<entries.size(); ++index)
	{
		if(entries.at(index)==entry)
		{
			entries.removeAt(index);
			break;
		}
	}
	mSettingsJson.insert(key, entries);
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
		appendJsonEntry("start_urls", start_url.toString(QUrl::FullyEncoded));
	}
}

void ConfigurationKeeper::removeStartUrl(const QUrl &start_url)
{
	const qsizetype index=mStartUrls.indexOf(start_url);
	if(index>=0)
	{
		removeJsonEntry("start_urls", mStartUrls.at(index).toString(QUrl::FullyEncoded));
		mStartUrls.removeAt(index);
	}
}

const QList<QUrl> &ConfigurationKeeper::startUrls() const
{
	return mStartUrls;
}

void ConfigurationKeeper::addAllowedUrlScheme(const QString &allowed_url_scheme)
{
	if(allowed_url_scheme.isEmpty())
	{
		return;
	}
	const QString lowerSchemeString=allowed_url_scheme.toLower();
	if(!mAllowedURLSchemes.contains(lowerSchemeString))
	{
		mAllowedURLSchemes.insert(lowerSchemeString);
		appendJsonEntry("allowed_url_schemes", lowerSchemeString);
	}
}

void ConfigurationKeeper::removeAllowedUrlScheme(const QString &allowed_url_scheme)
{
	if(allowed_url_scheme.isEmpty())
	{
		return;
	}
	const QString lowerSchemeString=allowed_url_scheme.toLower();
	if(mAllowedURLSchemes.remove(lowerSchemeString))
	{
		removeJsonEntry("allowed_url_schemes", lowerSchemeString);
	}
}

const QSet<QString> &ConfigurationKeeper::allowedUrlSchemes() const
{
	return(mAllowedURLSchemes);
}

void ConfigurationKeeper::addBlacklistedHost(const QString &blacklisted_host)
{
	if(blacklisted_host.isEmpty())
	{
		return;
	}
	const QString lowerHostString=blacklisted_host.toLower();
	if(!mBlacklistedHosts.contains(lowerHostString))
	{
		mBlacklistedHosts.insert(lowerHostString);
		appendJsonEntry("black_list", lowerHostString);
	}
}

void ConfigurationKeeper::removeBlacklistedHost(const QString &blacklisted_host)
{
	if(blacklisted_host.isEmpty())
	{
		return;
	}
	const QString lowerHostString=blacklisted_host.toLower();
	if(mBlacklistedHosts.remove(lowerHostString))
	{
		removeJsonEntry("black_list", lowerHostString);
	}
}

const QSet<QString> &ConfigurationKeeper::blacklistedHosts() const
{
	return(mBlacklistedHosts);
}

void ConfigurationKeeper::addCrawlingZone(const QString &crawling_zone)
{
	if(crawling_zone.isEmpty())
	{
		return;
	}
	const QString lowerZoneString=crawling_zone.toLower();
	const QUrl zoneURL(lowerZoneString);
	if(zoneURL.host().isEmpty())
	{
		return;
	}
	if(!mCrawlingZones[zoneURL.host()].contains(lowerZoneString))
	{
		mCrawlingZones[zoneURL.host()].append(lowerZoneString);
		appendJsonEntry("crawling_zones", lowerZoneString);
	}
}

void ConfigurationKeeper::removeCrawlingZone(const QString &crawling_zone)
{
	if(crawling_zone.isEmpty())
	{
		return;
	}
	const QString lowerZoneString=crawling_zone.toLower();
	const QUrl zoneURL(lowerZoneString);
	if(zoneURL.host().isEmpty())
	{
		return;
	}
	if(mCrawlingZones[zoneURL.host()].removeAll(lowerZoneString)>0)
	{
		removeJsonEntry("crawling_zones", lowerZoneString);
	}
	if(mCrawlingZones[zoneURL.host()].isEmpty())
	{
		mCrawlingZones.remove(zoneURL.host());
	}
}

const QHash<QString, QStringList> &ConfigurationKeeper::crawlingZones() const
{
	return(mCrawlingZones);
}

bool ConfigurationKeeper::urlSchemeAllowed(const QUrl &url) const
{
	const QString lowerSchemeString=url.scheme().toLower();
	if(mAllowedURLSchemes.isEmpty())
	{
		return(true);
	}
	return(mAllowedURLSchemes.contains(lowerSchemeString));
}

bool ConfigurationKeeper::urlHostAllowed(const QUrl &url) const
{
	const QString lowerHostString=url.host().toLower();
	if(mBlacklistedHosts.isEmpty())
	{
		return(true);
	}
	return(!mBlacklistedHosts.contains(lowerHostString));
}

bool ConfigurationKeeper::urlZoneAllowed(const QUrl &url) const
{
	const QString lowerUrlString=url.toString().toLower();
	const QString lowerHostString=url.host().toLower();
	if(mCrawlingZones.contains(lowerHostString))
	{
		bool allowedByZonePrefix=0;
		const QStringList &zonePrefixes=mCrawlingZones[lowerHostString];
		for(const QString &zonePrefix : zonePrefixes)
		{
			if(lowerUrlString.startsWith(zonePrefix))
			{
				allowedByZonePrefix=1;
				break;
			}
		}
		return(allowedByZonePrefix);
	}
	return(true);
}

int ConfigurationKeeper::loadSettingsFromJsonFile(const QString &path_to_file)
{
	if(path_to_file.isEmpty())
	{
		return(EXIT_FAILURE);
	}
	QFile configFile(path_to_file);
	if (!configFile.exists())
	{
		return(EXIT_FAILURE);
	}
	if(!configFile.open(QIODevice::ReadOnly))
	{
		return(EXIT_FAILURE);
	}

	QByteArray configData=configFile.readAll();
	configFile.close();

	QJsonParseError err;
	QJsonDocument configJsonDoc=QJsonDocument::fromJson(configData, &err);

	if (err.error != QJsonParseError::NoError)
	{
		return(EXIT_FAILURE);
	}

	if(!configJsonDoc.isObject())
	{
		return(EXIT_FAILURE);
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
		mSettingsJson["allowed_url_schemes"]=QJsonArray();
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
		mSettingsJson["start_urls"]=QJsonArray();
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
		mSettingsJson["black_list"]=QJsonArray();
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
		mSettingsJson["crawling_zones"]=QJsonArray();
		for(const QJsonValue &crawlingZone : crawlingZones)
		{
			if(crawlingZone.isString())
			{
				this->addCrawlingZone(crawlingZone.toString());
			}
		}
	}

	return(EXIT_SUCCESS);
}

int ConfigurationKeeper::saveSettingsToJsonFile(const QString &path_to_file) const
{
	if(path_to_file.isEmpty())
	{
		return(EXIT_FAILURE);
	}
	QSaveFile configFile(path_to_file);
	if(!configFile.open(QIODevice::WriteOnly))
	{
		return(EXIT_FAILURE);
	}
	const QByteArray data=QJsonDocument(mSettingsJson).toJson(QJsonDocument::JsonFormat::Indented);
	if(configFile.write(data)!=data.size() || !configFile.commit())
	{
		return(EXIT_FAILURE);
	}
	return(EXIT_SUCCESS);
}
