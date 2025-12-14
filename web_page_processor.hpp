#ifndef WEB_PAGE_PROCESSOR_HPP
#define WEB_PAGE_PROCESSOR_HPP

#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <QString>
#include <QObject>

class WebPageProcessor : public QObject
{
	Q_OBJECT
	QWebEnginePage *mWebPage;
	QWebEngineProfile *mProfile;
	QWebEngineView *mWebViewWidget;
	QString mPageContentHTML;
	QString mPageContentTEXT;
	QList<QUrl> mPageLinks;
	QSize mWindowSize;
	void createNewWebPage();
private slots:
	void extractPageContentTEXT(bool ok);
	void extractPageContentHTML(bool ok);
	void extractPageLinks();
public:
	WebPageProcessor(QObject *parent=nullptr);
	void setHttpUserAgent(const QString &user_agent);
	void setWindowSize(const QSize &window_size);
	void loadCookiesFromFirefoxProfile(const QString &path_to_file);
	void loadCookiesFromFirefoxDB(const QString &path_to_file);
	void loadPage(const QUrl &url);
	const QString &getPageContentAsHTML() const;
	const QString &getPageContentAsTEXT() const;
	QString getPageTitle() const;
	QByteArray getPageURLEncoded() const;
	const QList<QUrl> &getPageLinks() const;
signals:
	void pageLoadingSuccess();
	void pageProcessingFinished();
};

#endif // WEB_PAGE_PROCESSOR_HPP
