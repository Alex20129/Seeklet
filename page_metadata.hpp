#ifndef PAGE_METADATA_HPP
#define PAGE_METADATA_HPP

#include <QMap>
#include <QHash>
#include <QDateTime>
#include <QDataStream>

typedef QPair<quint64, quint64> Hash128;
typedef quint64 Hash64;

struct PageMetadata
{
	QString title;
	QByteArray url;
	QDateTime timeStamp;
	QHash<Hash64, quint64> tfAsHashes;
	Hash128 contentHash;
	quint64 wordsTotal=0;
	void writeToStream(QDataStream &stream) const;
	void readFromStream(QDataStream &stream);
	bool isValid() const;
};

#endif // PAGE_METADATA_HPP
