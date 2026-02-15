#include <QApplication>
#include <QTimer>
#include "main.hpp"
#include "crawler.hpp"

ConfigurationKeeper *gSettings;

int main(int argc, char **argv)
{
	gSettings = new ConfigurationKeeper();
	gSettings->loadSettingsFromJsonFile("crawler.json");

	QApplication fossenApp(argc, argv);

	Crawler *myCrawler=new Crawler;
	Indexer *myIndexer=new Indexer;

	QObject::connect(myCrawler, &Crawler::needToAddPage, myIndexer, &Indexer::addPage);
	QObject::connect(myCrawler, &Crawler::needToAddWord, myIndexer, &Indexer::addWord);
	QObject::connect(myCrawler, &Crawler::finished, myIndexer, &Indexer::save);
	// QObject::connect(myCrawler, &Crawler::finished, myIndexer, &Indexer::searchTest);
	QObject::connect(myCrawler, &Crawler::finished, &fossenApp, &QCoreApplication::quit);

	QTimer::singleShot(0, myIndexer, &Indexer::load);
	// QTimer::singleShot(0, myIndexer, &Indexer::searchTest);
	QTimer::singleShot(0, myCrawler, &Crawler::start);

	return(fossenApp.exec());
}
