#include <QApplication>
#include <QTimer>
#include "main.hpp"
#include "crawler.hpp"

ConfigurationKeeper *gSettings=nullptr;

int main(int argc, char **argv)
{
	QApplication seekletApp(argc, argv);

	gSettings=new ConfigurationKeeper(&seekletApp);
	gSettings->loadSettingsFromJsonFile("crawler.json");

	Crawler *myCrawler=new Crawler(&seekletApp);
	Indexer *myIndexer=new Indexer(&seekletApp);

	QObject::connect(myCrawler, &Crawler::needToAddPage, myIndexer, &Indexer::addPage);
	QObject::connect(myCrawler, &Crawler::needToAddWord, myIndexer, &Indexer::addWord);
	QObject::connect(myCrawler, &Crawler::finished, myIndexer, &Indexer::save);
	// QObject::connect(myCrawler, &Crawler::finished, myIndexer, &Indexer::searchTest);
	QObject::connect(myCrawler, &Crawler::finished, &seekletApp, &QCoreApplication::quit);

	QTimer::singleShot(0, myIndexer, &Indexer::load);
	// QTimer::singleShot(0, myIndexer, &Indexer::searchTest);
	QTimer::singleShot(0, myCrawler, &Crawler::start);

	return(seekletApp.exec());
}
