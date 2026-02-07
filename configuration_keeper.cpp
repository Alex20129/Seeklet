#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "configuration_keeper.hpp"

ConfigurationKeeper *gConfigurationKeeper=nullptr;

ConfigurationKeeper::ConfigurationKeeper(QObject *parent) : QObject(parent)
{
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

void ConfigurationKeeper::addAllowedUrlScheme(const QString &allowed_url_scheme)
{
	if(allowed_url_scheme.isEmpty())
	{
		return;
	}
	if(mAllowedURLSchemes.contains(allowed_url_scheme))
	{
		return;
	}
	mAllowedURLSchemes.append(allowed_url_scheme);
}

void ConfigurationKeeper::removeAllowedUrlScheme(const QString &allowed_url_scheme)
{
	mAllowedURLSchemes.removeAll(allowed_url_scheme);
}

const QStringList &ConfigurationKeeper::allowedUrlSchemes() const
{
	return mAllowedURLSchemes;
}

void ConfigurationKeeper::addStartUrl(const QString &start_url)
{
	if(start_url.isEmpty())
	{
		return;
	}
	if(mStartUrls.contains(start_url))
	{
		return;
	}
	mStartUrls.append(start_url);
}

void ConfigurationKeeper::removeStartUrl(const QString &start_url)
{
	mStartUrls.removeAll(start_url);
}

const QStringList &ConfigurationKeeper::startUrls() const
{
	return mStartUrls;
}

void ConfigurationKeeper::addBlacklistedHost(const QString &blacklisted_host)
{
	if(blacklisted_host.isEmpty())
	{
		return;
	}
	if(mBlacklistedHosts.contains(blacklisted_host))
	{
		return;
	}
	mBlacklistedHosts.append(blacklisted_host);
}

void ConfigurationKeeper::removeBlacklistedHost(const QString &blacklisted_host)
{
	mBlacklistedHosts.removeAll(blacklisted_host);
}

const QStringList &ConfigurationKeeper::blacklistedHosts() const
{
	return mBlacklistedHosts;
}

void ConfigurationKeeper::addCrawlingZone(const QUrl &crawling_zone)
{
	if(crawling_zone.host().isEmpty())
	{
		return;
	}
	if(mCrawlingZones[crawling_zone.host()].contains(crawling_zone.toString()))
	{
		return;
	}
	mCrawlingZones[crawling_zone.host()].append(crawling_zone.toString());
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
	if(configJsonObject.value("crawler_window_width").isDouble())
	{
		this->setCrawlerWindowWidth(configJsonObject.value("crawler_window_width").toDouble());
	}
	if(configJsonObject.value("crawler_window_height").isDouble())
	{
		this->setCrawlerWindowHeight(configJsonObject.value("crawler_window_height").toDouble());
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
	// WIP
	// TODO: save as JSON
}
