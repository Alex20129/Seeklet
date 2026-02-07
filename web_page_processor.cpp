#include <fcntl.h>
#include <unistd.h>
#include <QCoreApplication>
#include <QNetworkCookie>
#include <QWebEngineCookieStore>
#include <QFileInfo>
#include <QSettings>
#include <QDir>
#include <QScreen>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlError>
#include <htmlcxx/html/ParserDom.h>
#include <htmlcxx/html/Uri.h>
#include "configuration_keeper.hpp"
#include "web_page_processor.hpp"

void WebPageProcessor::createNewWebPage()
{
	if(mWebPage)
	{
		disconnect(mWebPage, nullptr, nullptr, nullptr);
		mWebPage->deleteLater();
	}
	mWebPage=new QWebEnginePage(mProfile, this);
	mWebViewWidget->setPage(mWebPage);
	connect(mWebPage, &QWebEnginePage::loadFinished, this, &WebPageProcessor::waitForJSToFinish);
}

void WebPageProcessor::waitForJSToFinish(bool ok)
{
	if(ok)
	{
		mJSCooldownTimer->start();
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
			this->mPageContentTEXT = text;
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
			this->mPageContentHTML = html;
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
				std::pair<bool, std::string> href_pair = domNode.attribute("href");
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
	mWebViewWidget=new QWebEngineView();
	mWebViewWidget->setWindowFlags(Qt::WindowType::FramelessWindowHint);
	mWebViewWidget->setAttribute(Qt::WA_DontShowOnScreen);
	mWebPage=nullptr;
	mProfile=new QWebEngineProfile(this);
	mProfile->setHttpCacheType(QWebEngineProfile::MemoryHttpCache);
	mProfile->setPersistentCookiesPolicy(QWebEngineProfile::AllowPersistentCookies);
	mProfile->setHttpUserAgent("Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:146.0) Gecko/20100101 Firefox/146.0");
	createNewWebPage();
	mJSCooldownTimer=new QTimer(this);
	mJSCooldownTimer->setSingleShot(1);
	mJSCooldownTimer->setInterval(3000);
	connect(mJSCooldownTimer, &QTimer::timeout, this, &WebPageProcessor::extractPageContentTEXT);
	connect(mJSCooldownTimer, &QTimer::timeout, this, &WebPageProcessor::extractPageContentHTML);
	connect(this, &WebPageProcessor::pageLoadingSuccess, this, &WebPageProcessor::extractPageLinks);
}

void WebPageProcessor::setHttpUserAgent(const QString &user_agent)
{
	if(mProfile->httpUserAgent()!=user_agent)
	{
		mProfile->setHttpUserAgent(user_agent);
		createNewWebPage();
	}
}

void WebPageProcessor::setWindowSize(const QSize &window_size)
{
	if(window_size.width()>0 && window_size.height()>0)
	{
		mWindowSize=window_size;
	}
	else
	{
		mWindowSize=mWebViewWidget->screen()->size();
	}
	mWebViewWidget->resize(mWindowSize);
}

void WebPageProcessor::setJSCooldownInterval(int64_t interval_ms)
{
	mJSCooldownTimer->setInterval(interval_ms);
}

void WebPageProcessor::loadCookiesFromFirefoxProfile(const QString &path_to_file)
{
	if(path_to_file.isEmpty())
	{
		return;
	}
	QSettings settings(path_to_file, QSettings::IniFormat);
	QStringList profiles = settings.childGroups();
	QFileInfo iniFile(path_to_file);
	QDir profilesDir = iniFile.absoluteDir();
	QString profilePath;
	for (const QString &group : profiles)
	{
		if (group.startsWith("Profile"))
		{
			settings.beginGroup(group);
			if (settings.contains("Default") && settings.value("Default").toInt() == 1)
			{
				profilePath = settings.value("Path").toString();
				settings.endGroup();
				break;
			}
			if (profilePath.isEmpty())
			{
				profilePath = settings.value("Path").toString();
			}
			settings.endGroup();
		}
	}
	if (profilePath.isEmpty())
	{
		return;
	}
	QString cookiesFilePath = profilesDir.absoluteFilePath(profilePath + "/cookies.sqlite");
	if (!QFile::exists(cookiesFilePath))
	{
		return;
	}
	loadCookiesFromFirefoxDB(cookiesFilePath);
}

void WebPageProcessor::loadCookiesFromFirefoxDB(const QString &path_to_file)
{
	QList<QNetworkCookie> cookies;
	{
		QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "firefox_cookies");
		db.setDatabaseName(path_to_file);
		if (db.open())
		{
			QSqlQuery query(db);
			if (query.exec("SELECT host, path, isSecure, expiry, name, value FROM moz_cookies"))
			{
				while (query.next())
				{
					QString host = query.value("host").toString();
					QString path = query.value("path").toString();
					bool isSecure = query.value("isSecure").toBool();
					qint64 expiry = query.value("expiry").toLongLong();
					QString name = query.value("name").toString();
					QString value = query.value("value").toString();
					if (expiry != 0 && expiry < QDateTime::currentSecsSinceEpoch())
					{
						continue;
					}
					QNetworkCookie cookie(name.toUtf8(), value.toUtf8());
					cookie.setDomain(host);
					cookie.setPath(path);
					cookie.setSecure(isSecure);
					if (expiry != 0)
					{
						cookie.setExpirationDate(QDateTime::fromSecsSinceEpoch(expiry));
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
