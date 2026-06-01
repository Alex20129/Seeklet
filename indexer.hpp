#ifndef INDEXER_HPP
#define INDEXER_HPP

#include <QHash>
#include <QStringList>
#include "page_metadata.hpp"

class Indexer : public QObject
{
	Q_OBJECT
	QHash<Hash64, QString> mDictionaryLookupTable;
	QHash<Hash64, QSet<Hash128>> mTableOfContents;
	QHash<Hash128, PageMetadata *> mIndexByContentHash;
	QHash<Hash128, PageMetadata *> mIndexByUrlHash;
	QString mDatabaseDirectory;
	struct ScoredPage
	{
		double score;
		const PageMetadata *page;
	};
	static bool pageScoreComparator(const ScoredPage &a, const ScoredPage &b)
	{
		return(a.score > b.score);
	};
#ifndef NDEBUG
	void printPageMetadata(const PageMetadata &page_md);
#endif
public:
	Indexer(QObject *parent=nullptr);
	~Indexer();
	void clear();
	void setDatabaseDirectory(const QString &database_directory);
	void merge(const Indexer &other);
	const PageMetadata *getPageMetadataByContentHash(const Hash128 &content_hash) const;
	const PageMetadata *getPageMetadataByUrlHash(const Hash128 &url_hash) const;
	QVector<const PageMetadata *> searchPagesByWords(QStringList words) const;
	double calculateTfIdfScore(const Hash128 &content_hash, const QStringList &words) const;
	double calculateTfIdfScore(const PageMetadata *page, const QStringList &words) const;
	double calculateTfIdfScore(const Hash128 &content_hash, const QString &word) const;
	double calculateTfIdfScore(const PageMetadata *page, const QString &word) const;
	void sortPagesByTfIdfScore(QVector<const PageMetadata *> &pages, const QStringList &words) const;
public slots:
	void addPage(const PageMetadata &page_metadata);
	void deletePage(PageMetadata *page_metadata);
	void addWord(const QString &word);
	void save();
	void load();
#ifndef NDEBUG
	void searchTest();
#endif
};

#endif // INDEXER_HPP
