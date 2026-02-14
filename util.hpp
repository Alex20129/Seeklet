#ifndef UTIL_HPP
#define UTIL_HPP

#include <QMap>
#include <QString>

QMap<QString, quint64> ExtractAndCountWords(const QString &text);

uint64_t hash_function_64(const QByteArray &data);
QByteArray hash_function_128(const QByteArray &data);

#endif // UTIL_HPP
