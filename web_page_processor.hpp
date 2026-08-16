#ifndef WEB_PAGE_PROCESSOR_HPP
#define WEB_PAGE_PROCESSOR_HPP

#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>
#include <QString>
#include <QTimer>
#include <QObject>

class WebPageProcessor : public QObject
{
	Q_OBJECT
	QWebEnginePage *mWebPage=nullptr;
	QWebEngineProfile *mProfile=nullptr;
	QWebEngineView *mWebViewWidget=nullptr;
	QTimer *mJSCompletionTimer=nullptr;
	QString mPageContentHTML;
	QString mPageContentTEXT;
	QList<QUrl> mPageLinks;
	void createNewWebPage();
private slots:
	void waitForJSToFinish(bool ok);
	void extractPageContentTEXT();
	void extractPageContentHTML();
	void extractPageLinks();
public:
	WebPageProcessor(QObject *parent=nullptr);
	~WebPageProcessor();
	void setHttpCacheType(QWebEngineProfile::HttpCacheType cache_type);
	void setHttpCacheSize(int cache_size);
	void setHttpUserAgent(const QString &user_agent);
	void setLoadImages(bool load_images);
	void setWindowSize(QSize window_size);
	void loadCookiesFromFirefox(const QString &path_to_dir);
	void loadCookiesFromChromium(const QString &path_to_dir);
	void loadCookiesFromFirefoxDB(const QString &path_to_file);
	void loadPage(const QUrl &url);
	const QString &getPageContentAsHTML() const;
	const QString &getPageContentAsTEXT() const;
	QString getPageTitle() const;
	QUrl getPageURL() const;
	QByteArray getPageURLEncoded(QUrl::FormattingOptions options) const;
	const QList<QUrl> &getPageLinks() const;
signals:
	void pageLoadingSuccess();
	void pageLoadingFail();
	void pageProcessingFinished();
};

#endif // WEB_PAGE_PROCESSOR_HPP
