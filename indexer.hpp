#ifndef INDEXER_HPP
#define INDEXER_HPP

#include <QObject>
#include <QUrl>
#include <QMap>
#include <QStringList>
#include <QDateTime>

struct PageMetadata
{
	uint64_t urlHash;
	uint64_t contentHash;
	uint64_t wordsTotal;
	QDateTime timeStamp;
	QString title;
	QByteArray url;
	QMap<QString, uint64_t> words;
	PageMetadata();
	bool isValid() const;
};

class Indexer : public QObject
{
	Q_OBJECT
	QHash<QString, QSet<uint64_t>> localIndexTableOfContents;
	QHash<uint64_t, PageMetadata *> localIndexByContentHash;
	QHash<uint64_t, PageMetadata *> localIndexByUrlHash;
public:
	Indexer(QObject *parent = nullptr);
	~Indexer();
	//TODO: init, save, load
	void initialize(const QString &db_path);
	void load(const QString &db_path);
	void save(const QString &db_path);
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
};

#endif // INDEXER_HPP
