#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "configuration_keeper.hpp"

ConfigurationKeeper::ConfigurationKeeper(QObject *parent) : QObject(parent)
{
	uint64_t WIP; // TODO: default settings
}

ConfigurationKeeper::~ConfigurationKeeper()
{
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

void ConfigurationKeeper::setFireFoxProfileDirectory(const QString &firefox_profile_directory)
{
	mFireFoxProfileDirectory=firefox_profile_directory;
}

const QString &ConfigurationKeeper::fireFoxProfileDirectory() const
{
	return mFireFoxProfileDirectory;
}

void ConfigurationKeeper::setCrawlerWindowWidth(int crawler_window_width)
{
	mCrawlerWindowSize.setWidth(crawler_window_width);
}

void ConfigurationKeeper::setCrawlerWindowHeight(int crawler_window_height)
{
	mCrawlerWindowSize.setHeight(crawler_window_height);
}

void ConfigurationKeeper::setCrawlerWindowSize(const QSize &crawler_window_size)
{
	mCrawlerWindowSize=crawler_window_size;
}

const QSize &ConfigurationKeeper::crawlerWindowSize() const
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
	mPagesPerSessionMax=pages_per_session;
}

int ConfigurationKeeper::pagesPerSession() const
{
	return mPagesPerSessionMax;
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
	QJsonDocument configJsonDoc = QJsonDocument::fromJson(configData, &err);

	if (err.error != QJsonParseError::NoError)
	{
		return;
	}

	if(!configJsonDoc.isObject())
	{
		return;
	}

	QJsonObject configJsonObject = configJsonDoc.object();

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
		this->setFireFoxProfileDirectory(configJsonObject.value("firefox_profile_directory").toString());
	}
	if(configJsonObject.value("crawler_window_width").isDouble())
	{
		this->setCrawlerWindowWidth(configJsonObject.value("crawler_window_width").toDouble());
	}
	if(configJsonObject.value("crawler_window_height").isDouble())
	{
		this->setCrawlerWindowHeight(configJsonObject.value("crawler_window_height").toDouble());
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
		this->setPagesPerSession(configJsonObject.value("pages_per_session").toDouble());
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
