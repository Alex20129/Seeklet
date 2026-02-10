#ifndef INDEXER_HPP
#define INDEXER_HPP

#include <QMap>
#include <QStringList>
#include <QDateTime>
#include <QDataStream>

struct PageMetadata
{
	quint64 urlHash;
	quint64 contentHash;
	quint64 wordsTotal;
	QDateTime timeStamp;
	QString title;
	QByteArray url;
	QHash<quint64, quint64> wordsAsHashes;
	PageMetadata();
	void writeToStream(QDataStream &stream) const;
	void readFromStream(QDataStream &stream);
	bool isValid() const;
};

class Indexer : public QObject
{
	Q_OBJECT
	QHash<quint64, QString> mDictionaryLookupTable;
	QHash<QString, QSet<quint64>> mIndexTableOfContents;
	QHash<quint64, PageMetadata *> mIndexByContentHash;
	QHash<quint64, PageMetadata *> mIndexByUrlHash;
	QString mDatabaseDirectory;
public:
	Indexer(QObject *parent = nullptr);
	~Indexer();
	void clear();
	void setDatabaseDirectory(const QString &database_directory);
	void merge(const Indexer &other);
	const PageMetadata *getPageMetadataByContentHash(uint64_t content_hash) const;
	const PageMetadata *getPageMetadataByUrlHash(uint64_t url_hash) const;
	QVector<const PageMetadata *> searchPagesByWords(QStringList words) const;
	double calculateTfIdfScore(uint64_t content_hash, const QStringList &words) const;
	double calculateTfIdfScore(const PageMetadata *page, const QStringList &words) const;
	double calculateTfIdfScore(uint64_t content_hash, const QString &word) const;
	double calculateTfIdfScore(const PageMetadata *page, const QString &word) const;
	void sortPagesByTfIdfScore(QVector<const PageMetadata *> &pages, const QStringList &words) const;
public slots:
	void addPage(const PageMetadata &page_metadata);
	void addWord(const QString &word);
	void save();
	void load();
#ifndef NDEBUG
	void searchTest();
#endif
};

#endif // INDEXER_HPP
