#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <iostream>
#include <string>

#include "main.hpp"
#include "crawler.hpp"

using namespace std;

int main(int argc, char **argv)
{
	QGuiApplication fossenApp(argc, argv);

	Crawler *myCrawler=new Crawler;

	myCrawler->setPathToFireFoxProfile("/home/alex/snap/firefox/common/.mozilla/firefox/profiles.ini");

	QFile crawlerConfigFile("crawler.json");
	if (!crawlerConfigFile.exists())
	{
		qDebug() << "crawler.json doesn't exist";
		return 22;
	}

	if(!crawlerConfigFile.open(QIODevice::ReadOnly))
	{
		qDebug() << "Couldn't open crawler.json";
		return 33;
	}

	QByteArray crawlerConfigData=crawlerConfigFile.readAll();
	crawlerConfigFile.close();

	QJsonParseError err;
	QJsonDocument crawlerConfigJsonDoc = QJsonDocument::fromJson(crawlerConfigData, &err);

	if (err.error != QJsonParseError::NoError)
	{
		qDebug() << "Couldn't parse crawler.json :" << err.errorString();
		return 44;
	}

	if(!crawlerConfigJsonDoc.isObject())
	{
		qDebug() << "Bad JSON in crawler.json file";
		return 55;
	}

	QJsonObject rawlerConfigJsonObject = crawlerConfigJsonDoc.object();

	const QJsonArray &startURLs=rawlerConfigJsonObject.value("start_urls").toArray();
	for (const QJsonValue &startURL : startURLs)
	{
		myCrawler->addURLToQueue(startURL.toString());
	}

	const QJsonArray &crawlingZones=rawlerConfigJsonObject.value("crawling_zones").toArray();
	for (const QJsonValue &crawlingZone : crawlingZones)
	{
		myCrawler->addCrawlingZone(crawlingZone.toString());
	}

	const QJsonArray &blHosts=rawlerConfigJsonObject.value("black_list").toArray();
	for (const QJsonValue &blHost : blHosts)
	{
		myCrawler->addHostnameToBlacklist(blHost.toString());
	}

	// ======
	// QObject::connect(myCrawler, &Crawler::finished, myCrawler, &Crawler::searchTest);
	QObject::connect(myCrawler, &Crawler::finished, &fossenApp, &QCoreApplication::quit);
	// ======

	QTimer::singleShot(0, myCrawler, &Crawler::start);

	return(fossenApp.exec());
}
