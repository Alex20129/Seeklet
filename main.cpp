#include <QApplication>
#include <QTimer>
#include "configuration_keeper.hpp"
#include "crawler.hpp"

int main(int argc, char **argv)
{
	gSettings = new ConfigurationKeeper();
	gSettings->loadSettingsFromJsonFile("crawler.json");

	QApplication fossenApp(argc, argv);

	Crawler *myCrawler=new Crawler;

	QObject::connect(myCrawler, &Crawler::finished, myCrawler, &Crawler::saveIndex);
	QObject::connect(myCrawler, &Crawler::finished, myCrawler, &Crawler::searchTest);
	QObject::connect(myCrawler, &Crawler::finished, &fossenApp, &QCoreApplication::quit);

	QTimer::singleShot(0, myCrawler, &Crawler::loadIndex);
	QTimer::singleShot(0, myCrawler, &Crawler::start);

	return(fossenApp.exec());
}
