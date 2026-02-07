#ifndef CONFIGURATION_KEEPER_HPP
#define CONFIGURATION_KEEPER_HPP

#include <QObject>
#include <QSize>
#include <QStringList>
#include <QHash>

class ConfigurationKeeper : public QObject
{
	Q_OBJECT
	QString mHttpUserAgent;
	QString mDatabaseDirectory;
	QSize mCrawlerWindowSize;
	QStringList mAllowedURLSchemes;
	QStringList mStartUrls;
	QStringList mBlacklistedHosts;
	QHash<QString, QStringList> mCrawlingZones;
public:
	ConfigurationKeeper(QObject *parent = nullptr);
	~ConfigurationKeeper();

	void setHttpUserAgent(const QString &http_user_agent);
	const QString &httpUserAgent() const;

	void setDatabaseDirectory(const QString &database_directory);
	const QString &databaseDirectory() const;

	void setCrawlerWindowWidth(int crawler_window_width);
	void setCrawlerWindowHeight(int crawler_window_height);
	void setCrawlerWindowSize(const QSize &crawler_window_size);
	const QSize &crawlerWindowSize() const;

	void addAllowedUrlScheme(const QString &allowed_url_scheme);
	void removeAllowedUrlScheme(const QString &allowed_url_scheme);
	const QStringList &allowedUrlSchemes() const;

	void addStartUrl(const QString &start_url);
	void removeStartUrl(const QString &start_url);
	const QStringList &startUrls() const;

	void addBlacklistedHost(const QString &blacklisted_host);
	void removeBlacklistedHost(const QString &blacklisted_host);
	const QStringList &blacklistedHosts() const;

	void addCrawlingZone(const QUrl &crawling_zone);
	void removeCrawlingZone(const QUrl &crawling_zone);
	const QHash<QString, QStringList> &crawlingZones()const;

	void loadSettingsFromJsonFile(const QString &path_to_file);
	void saveSettingsToJsonFile(const QString &path_to_file) const;
};

extern ConfigurationKeeper *gConfigurationKeeper;

#endif // CONFIGURATION_KEEPER_HPP
