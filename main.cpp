#include "main.hpp"
#include "crawler.hpp"

int main(int argc, char **argv)
{
	QApplication fossenApp(argc, argv);

	Crawler *myCrawler=new Crawler;

	myCrawler->setPathToFirefoxProfile("/home/alex/snap/firefox/common/.mozilla/firefox/profiles.ini");
	myCrawler->loadSettingsFromJSONFile("crawler.json");

	// ======
	// QObject::connect(myCrawler, &Crawler::finished, myCrawler, &Crawler::searchTest);
	QObject::connect(myCrawler, &Crawler::finished, myCrawler, &Crawler::saveIndex);
	QObject::connect(myCrawler, &Crawler::finished, &fossenApp, &QCoreApplication::quit);
	// ======

	QTimer::singleShot(0, myCrawler, &Crawler::start);

	return(fossenApp.exec());
}
