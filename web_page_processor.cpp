#include <fcntl.h>
#include <unistd.h>
#include <QCoreApplication>
#include <QNetworkCookie>
#include <QWebEngineCookieStore>
#include <QWebEngineSettings>
#include <QFileInfo>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QScreen>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <htmlcxx/html/ParserDom.h>
#include <htmlcxx/html/Uri.h>
#include "main.hpp"
#include "web_page_processor.hpp"

void WebPageProcessor::createNewWebPage()
{
	mJSCompletionTimer->stop();
	mWebPage->triggerAction(QWebEnginePage::WebAction::Stop);
	mWebPage=new QWebEnginePage(mProfile, mWebViewWidget);
	mWebViewWidget->setPage(mWebPage);
	connect(mWebPage, &QWebEnginePage::loadFinished, this, &WebPageProcessor::waitForJSToFinish);
}

void WebPageProcessor::scrollPage() const
{
	mWebPage->runJavaScript(QStringLiteral(
	R"((async function page_scroll()
	{
		const SCROLLS = 20;
		const INTERVAL_MS = 250;
		let scrollDistance = window.document.body.scrollHeight;
		let scroll = 0;
		while (scroll++ < SCROLLS)
		{
			window.scrollBy(0, Math.round(scrollDistance/SCROLLS));
			await new Promise(resolve => setTimeout(resolve, INTERVAL_MS));
		}
	})();)"));
}

void WebPageProcessor::waitForJSToFinish(bool ok)
{
	if(ok)
	{
		scrollPage();
		mJSCompletionTimer->start(gSettings->jsCompletionTimeout());
	}
	else
	{
		emit pageLoadingFail();
	}
}

void WebPageProcessor::extractPageContentTEXT()
{
	mWebPage->toPlainText(
		[this](const QString &text)
		{
			this->mPageContentTEXT=text;
			if(!this->mPageContentHTML.isEmpty())
			{
				emit pageLoadingSuccess();
			}
		});
}

void WebPageProcessor::extractPageContentHTML()
{
	mWebPage->toHtml(
		[this](const QString &html)
		{
			this->mPageContentHTML=html;
			if(!this->mPageContentTEXT.isEmpty())
			{
				emit pageLoadingSuccess();
			}
		});
}

void WebPageProcessor::extractPageLinks()
{
	using namespace htmlcxx;
	HTML::ParserDom parser;
	parser.parseTree(mPageContentHTML.toStdString());
	const tree<HTML::Node> &domTree=parser.getTree();
	QUrl baseUrl=mWebPage->url();
	for (HTML::Node &domNode : domTree)
	{
		if (domNode.isTag())
		{
			if ((domNode.tagName() != "a") && (domNode.tagName() != "A"))
			{
				continue;
			}
			else
			{
				domNode.parseAttributes();
				std::pair<bool, std::string> href_pair=domNode.attribute("href");
				if (href_pair.first)
				{
					QString hrefQString=QString::fromStdString(href_pair.second);
					hrefQString.replace("&amp;", "&");
					if(!hrefQString.isEmpty())
					{
						QUrl processedUrl;
						if (baseUrl.isValid())
						{
							processedUrl=baseUrl.resolved(QUrl(hrefQString));
						}
						else
						{
							processedUrl=QUrl(hrefQString);
						}
						if (processedUrl.isValid())
						{
							mPageLinks.append(processedUrl);
						}
					}
				}
			}
		}
	}
	emit pageProcessingFinished();
}

WebPageProcessor::WebPageProcessor(QObject *parent) : QObject(parent)
{
	mProfile=new QWebEngineProfile(this);
	mProfile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
	mProfile->setHttpCacheMaximumSize(gSettings->httpCacheSize());
	mProfile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
	if(!gSettings->httpUserAgent().isEmpty())
	{
		mProfile->setHttpUserAgent(gSettings->httpUserAgent());
	}
	mProfile->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, gSettings->loadImages());
	mWebViewWidget=new QWebEngineView(mProfile);
	mWebPage=new QWebEnginePage(mProfile, mWebViewWidget);
	mWebViewWidget->setPage(mWebPage);
	setWindowSize(gSettings->windowSize());
	if(gSettings->showBrowserWindow()==0)
	{
		mWebViewWidget->setAttribute(Qt::WidgetAttribute::WA_DontShowOnScreen, true);
		Qt::WindowFlags WebViewWidgetFlags=
			Qt::WindowType::Window |
			Qt::WindowType::FramelessWindowHint |
			Qt::WindowType::BypassWindowManagerHint;
		mWebViewWidget->setWindowFlags(WebViewWidgetFlags);
	}
	else if(gSettings->showBrowserWindow()==1)
	{
		Qt::WindowFlags WebViewWidgetFlags=
			Qt::WindowType::Window |
			Qt::WindowType::WindowTitleHint |
			Qt::WindowType::WindowMinimizeButtonHint |
			Qt::WindowType::WindowMaximizeButtonHint |
			Qt::WindowType::WindowCloseButtonHint;
		mWebViewWidget->setWindowFlags(WebViewWidgetFlags);
		mWebViewWidget->showMaximized();
	}
	else
	{
		Qt::WindowFlags WebViewWidgetFlags=
			Qt::WindowType::Window |
			Qt::WindowType::FramelessWindowHint;
		mWebViewWidget->setWindowFlags(WebViewWidgetFlags);
		mWebViewWidget->showFullScreen();
	}
	mJSCompletionTimer=new QTimer(this);
	mJSCompletionTimer->setSingleShot(1);
	connect(mWebPage, &QWebEnginePage::loadFinished, this, &WebPageProcessor::waitForJSToFinish);
	connect(mJSCompletionTimer, &QTimer::timeout, this, &WebPageProcessor::extractPageContentTEXT);
	connect(mJSCompletionTimer, &QTimer::timeout, this, &WebPageProcessor::extractPageContentHTML);
	connect(this, &WebPageProcessor::pageLoadingSuccess, this, &WebPageProcessor::extractPageLinks);
}

WebPageProcessor::~WebPageProcessor()
{
	delete mWebViewWidget;
}

void WebPageProcessor::setHttpCacheType(QWebEngineProfile::HttpCacheType cache_type)
{
	if(mProfile->httpCacheType()!=cache_type)
	{
		mProfile->setHttpCacheType(cache_type);
		uint64_t WIP;
		// gSettings->setHttpCacheType(cache_type);
		createNewWebPage();
	}
}

void WebPageProcessor::setHttpCacheSize(int cache_size)
{
	if(mProfile->httpCacheMaximumSize()!=cache_size)
	{
		mProfile->setHttpCacheMaximumSize(cache_size);
		gSettings->setHttpCacheSize(cache_size);
		createNewWebPage();
	}
}

void WebPageProcessor::setHttpUserAgent(const QString &user_agent)
{
	if(user_agent.isEmpty())
	{
		return;
	}
	if(mProfile->httpUserAgent()!=user_agent)
	{
		mProfile->setHttpUserAgent(user_agent);
		gSettings->setHttpUserAgent(user_agent);
		createNewWebPage();
	}
}

void WebPageProcessor::setLoadImages(bool load_images)
{
	if(mProfile->settings()->testAttribute(QWebEngineSettings::AutoLoadImages)!=load_images)
	{
		mProfile->settings()->setAttribute(QWebEngineSettings::AutoLoadImages, load_images);
		gSettings->setLoadImages(load_images);
		createNewWebPage();
	}
}

void WebPageProcessor::setWindowSize(QSize window_size)
{
	if(mWebViewWidget->size()!=window_size)
	{
		if(window_size.width()>0 && window_size.height()>0)
		{
			mWebViewWidget->resize(window_size);
		}
		else
		{
			mWebViewWidget->resize(mWebViewWidget->screen()->size());
		}
		gSettings->setWindowSize(window_size);
	}
}

void WebPageProcessor::loadCookiesFromFirefox(const QString &path_to_dir)
{
	if(path_to_dir.isEmpty())
	{
		return;
	}
	QDir profilesDir(path_to_dir);
	QString iniFilePath=profilesDir.absoluteFilePath(QStringLiteral("profiles.ini"));
	if (!QFile::exists(iniFilePath))
	{
		return;
	}
	QSettings settings(iniFilePath, QSettings::IniFormat);
	QStringList profiles=settings.childGroups();
	QString profileDirName;
	for (const QString &group : profiles)
	{
		if (group.startsWith("Profile"))
		{
			settings.beginGroup(group);
			if (settings.contains("Default") && settings.value("Default").toInt() == 1)
			{
				profileDirName=settings.value("Path").toString();
				settings.endGroup();
				break;
			}
			if (profileDirName.isEmpty())
			{
				profileDirName=settings.value("Path").toString();
			}
			settings.endGroup();
		}
	}
	if (profileDirName.isEmpty())
	{
		return;
	}
	QString cookiesFilePath=profilesDir.absoluteFilePath(profileDirName) + QStringLiteral("/cookies.sqlite");
	loadCookiesFromFirefoxDB(cookiesFilePath);
}

void WebPageProcessor::loadCookiesFromFirefoxDB(const QString &path_to_file)
{
	if (!QFile::exists(path_to_file))
	{
		return;
	}
	QList<QNetworkCookie> cookies;
	{
		QSqlDatabase db=QSqlDatabase::addDatabase("QSQLITE", "firefox_cookies");
		db.setDatabaseName(path_to_file);
		if (db.open())
		{
			QSqlQuery query(db);
			if (query.exec("SELECT host, path, isSecure, expiry, name, value FROM moz_cookies"))
			{
				while (query.next())
				{
					QString host=query.value("host").toString();
					QString path=query.value("path").toString();
					QString name=query.value("name").toString();
					QString value=query.value("value").toString();
					bool isSecure=query.value("isSecure").toBool();
					qint64 expiry=query.value("expiry").toLongLong();
					if (expiry != 0 && expiry < QDateTime::currentMSecsSinceEpoch())
					{
						continue;
					}
					if(value.isEmpty())
					{
						continue;
					}
					QNetworkCookie cookie(name.toUtf8(), value.toUtf8());
					cookie.setDomain(host);
					cookie.setPath(path);
					cookie.setSecure(isSecure);
					if (expiry != 0)
					{
						cookie.setExpirationDate(QDateTime::fromMSecsSinceEpoch(expiry));
					}
					cookies.append(cookie);
				}
			}
			db.close();
		}
	}
	QSqlDatabase::removeDatabase("firefox_cookies");
	for (const QNetworkCookie &cookie : cookies)
	{
		mProfile->cookieStore()->setCookie(cookie);
	}
}

void WebPageProcessor::loadCookiesFromChromium(const QString &path_to_dir)
{
	if(path_to_dir.isEmpty())
	{
		return;
	}
	QDir profilesDir(path_to_dir);
	QString lsFilePath=profilesDir.absoluteFilePath(QStringLiteral("Local State"));
	QFile lsFile(lsFilePath);
	if(!lsFile.exists())
	{
		return;
	}
	QJsonObject lsJsonObject;
	QJsonParseError lsJsonParseError;
	if(lsFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		QByteArray lsFileData=lsFile.readAll();
		QJsonDocument lsJsonDocument=QJsonDocument::fromJson(lsFileData, &lsJsonParseError);
		lsJsonObject=lsJsonDocument.object();
		lsFile.close();
	}
	else
	{
		return;
	}
	if(lsJsonParseError.error!=QJsonParseError::NoError)
	{
		return;
	}
	QJsonObject profileJsonObject=lsJsonObject.value("profile").toObject();
	QJsonArray lastActiveProfilesJsonArray=profileJsonObject.value("last_active_profiles").toArray();
	QJsonArray profilesOrderJsonArray=profileJsonObject.value("profiles_order").toArray();
	QString profileDirName;
	if(lastActiveProfilesJsonArray.size())
	{
		profileDirName=lastActiveProfilesJsonArray.first().toString();
	}
	else if(profilesOrderJsonArray.size())
	{
		profileDirName=profilesOrderJsonArray.first().toString();
	}
	if(profileDirName.isEmpty())
	{
		return;
	}
	QString cookiesFilePath=profilesDir.absoluteFilePath(profileDirName)+QStringLiteral("/Cookies");
	loadCookiesFromChromiumDB(cookiesFilePath);
}

// TODO :
QString decryptChromiumCookie(const QByteArray &encrypted_value)
{
	uint64_t WIP;
	if(encrypted_value.startsWith("v10"))
	{
		// qDebug("v10");
	}
	else if(encrypted_value.startsWith("v11"))
	{
		// qDebug("v11");
	}
	return QString();
}

void WebPageProcessor::loadCookiesFromChromiumDB(const QString &path_to_file)
{
	if (!QFile::exists(path_to_file))
	{
		return;
	}
	QList<QNetworkCookie> cookies;
	{
		QSqlDatabase db=QSqlDatabase::addDatabase("QSQLITE", "chromium_cookies");
		db.setDatabaseName(path_to_file);
		if (db.open())
		{
			QSqlQuery query(db);
			if(query.exec("SELECT host_key, name, value, path, expires_utc, is_secure, is_httponly, encrypted_value FROM cookies"))
			{
				while (query.next())
				{
					QString host=query.value("host_key").toString();
					QString path=query.value("path").toString();
					QString name=query.value("name").toString();
					QString value=query.value("value").toString();
					bool isSecure=query.value("is_secure").toBool();
					bool isHttpOnly=query.value("is_httponly").toBool();
					qint64 expiresUtc=query.value("expires_utc").toLongLong();
					QByteArray encryptedValue=query.value("encrypted_value").toByteArray();
					qint64 expiresUnixMs=0;
					if(expiresUtc != 0)
					{
						expiresUnixMs = (expiresUtc / 1000LL) - 11644473600000LL;
						if(expiresUnixMs < QDateTime::currentMSecsSinceEpoch())
						{
							continue;
						}
					}
					if(value.isEmpty() && !encryptedValue.isEmpty())
					{
						value=decryptChromiumCookie(encryptedValue);
					}
					if(value.isEmpty())
					{
						continue;
					}
					QNetworkCookie cookie(name.toUtf8(), value.toUtf8());
					cookie.setDomain(host);
					cookie.setPath(path);
					cookie.setSecure(isSecure);
					cookie.setHttpOnly(isHttpOnly);
					if (expiresUnixMs != 0)
					{
						cookie.setExpirationDate(QDateTime::fromMSecsSinceEpoch(expiresUnixMs));
					}
					cookies.append(cookie);
				}
			}
			db.close();
		}
	}
	QSqlDatabase::removeDatabase("chromium_cookies");
	for (const QNetworkCookie &cookie : cookies)
	{
		mProfile->cookieStore()->setCookie(cookie);
	}
}

void WebPageProcessor::loadPage(const QUrl &url)
{
	mPageContentHTML.clear();
	mPageContentTEXT.clear();
	mPageLinks.clear();
	mWebPage->load(url);
}

const QString &WebPageProcessor::getPageContentAsHTML() const
{
	return mPageContentHTML;
}

const QString &WebPageProcessor::getPageContentAsTEXT() const
{
	return mPageContentTEXT;
}

QString WebPageProcessor::getPageTitle() const
{
	return mWebPage->title();
}

QUrl WebPageProcessor::getPageURL() const
{
	return mWebPage->url();
}

QByteArray WebPageProcessor::getPageURLEncoded(QUrl::FormattingOptions options) const
{
	return mWebPage->url().toEncoded(options);
}

const QList<QUrl> &WebPageProcessor::getPageLinks() const
{
	return mPageLinks;
}
