#ifndef CHROMIUM_COOKIE_HPP
#define CHROMIUM_COOKIE_HPP

#include <QByteArray>
#include <QString>

QString decryptChromiumCookieV10(const QByteArray &encrypted_value, const QString &host);
QString decryptChromiumCookieV11(const QByteArray &encrypted_value, const QString &host);

#endif // CHROMIUM_COOKIE_HPP
