#ifndef UTIL_HPP
#define UTIL_HPP

#include <QMap>
#include <QString>

QMap<QString, quint64> ExtractAndCountWords(const QString &text);

uint64_t mwc_hash_64(const QByteArray &data);
uint64_t fnv1a_hash_64(const QByteArray &data);
uint64_t xorshift_hash_64(const QByteArray &data);
uint64_t xorshiftstar_hash_64(const QByteArray &data);

#endif // UTIL_HPP
