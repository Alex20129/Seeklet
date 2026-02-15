#ifndef INDEXER_HPP
#define INDEXER_HPP

#include <QMap>
#include <QStringList>
#include <QDateTime>
#include <QDataStream>

struct PageMetadata
{
	QString title;
	QByteArray url;
	QByteArray urlHash;
	QByteArray contentHash;
	QDateTime timeStamp;
	QHash<quint64, quint64> wordsAsHashes;
	quint64 wordsTotal;
	PageMetadata();
	void writeToStream(QDataStream &stream) const;
	void readFromStream(QDataStream &stream);
	bool isValid() const;
};

class Indexer : public QObject
{
	Q_OBJECT
	QHash<quint64, QString> mDictionaryLookupTable;
	QHash<quint64, QSet<QByteArray>> mTableOfContents;
	QHash<QByteArray, PageMetadata *> mIndexByContentHash;
	QHash<QByteArray, PageMetadata *> mIndexByUrlHash;
	QString mDatabaseDirectory;
public:
	Indexer(QObject *parent = nullptr);
	~Indexer();
#ifndef NDEBUG
	void printPageMetadata(const PageMetadata &page_md);
#endif
	void clear();
	void setDatabaseDirectory(const QString &database_directory);
	void merge(const Indexer &other);
	const PageMetadata *getPageMetadataByContentHash(const QByteArray &content_hash) const;
	const PageMetadata *getPageMetadataByUrlHash(const QByteArray &url_hash) const;
	QVector<const PageMetadata *> searchPagesByWords(QStringList words) const;
	double calculateTfIdfScore(const QByteArray &content_hash, const QStringList &words) const;
	double calculateTfIdfScore(const PageMetadata *page, const QStringList &words) const;
	double calculateTfIdfScore(const QByteArray &content_hash, const QString &word) const;
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
