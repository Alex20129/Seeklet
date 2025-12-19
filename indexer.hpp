#ifndef INDEXER_HPP
#define INDEXER_HPP

#include <QObject>
#include <QUrl>
#include <QMap>
#include <QStringList>
#include <QDateTime>

struct PageMetadata
{
	uint64_t contentHash;
	uint64_t wordsTotal;
	QDateTime timeStamp;
	QString title;
	QByteArray url;
	QMap<QString, uint64_t> words;
};

class Indexer : public QObject
{
	Q_OBJECT
	QHash<QString, QSet<uint64_t>> localIndexTableOfContents;
	QHash<uint64_t, PageMetadata*> localIndexStorage;
public:
	Indexer(QObject *parent = nullptr);
	~Indexer();
	//TODO: init, save, load
	void initialize(const QString &db_path);
	void load(const QString &db_path);
	void save(const QString &db_path);
	void merge(const Indexer &other);
	double calculateTfIdfScore(const PageMetadata *page, const QString &word) const;
	QVector<const PageMetadata*> searchByWords(const QStringList &words) const;
public slots:
	void addPage(const PageMetadata &page_metadata);
};

#endif // INDEXER_HPP
