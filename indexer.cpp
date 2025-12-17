#include <QRegularExpression>
#include <QDebug>
#include "indexer.hpp"

Indexer::Indexer(QObject *parent) : QObject(parent)
{
}

Indexer::~Indexer()
{
	qDeleteAll(localIndexStorage);
	localIndexStorage.clear();
}

//TODO: init, save, load
void Indexer::initialize(const QString &db_path)
{
	qDebug("Indexer::initialize");
	qDebug()<<db_path;
}

void Indexer::load(const QString &db_path)
{
	qDebug("Indexer::load");
	qDebug()<<db_path;
}

void Indexer::save(const QString &db_path)
{
	qDebug("Indexer::save");
	qDebug()<<db_path;
}

void Indexer::merge(const Indexer &other)
{
	qDebug("Indexer::merge");
	for (QHash<uint64_t, PageMetadata*>::const_iterator it = other.localIndexStorage.constBegin(); it != other.localIndexStorage.constEnd(); it++)
	{
		const uint64_t hash = it.key();
		if (!localIndexStorage.contains(hash))
		{
			PageMetadata *pageMetaDataCopy = new PageMetadata(*it.value());
			localIndexStorage.insert(hash, pageMetaDataCopy);
		}
	}
	for (QHash<QString, QSet<uint64_t>>::const_iterator it = other.localIndexTableOfContents.constBegin(); it != other.localIndexTableOfContents.constEnd(); it++)
	{
		const QString &word = it.key();
		const QSet<uint64_t> &hashes = it.value();
		localIndexTableOfContents[word].unite(hashes);
	}
	qDebug() << "Merged index: added" << other.localIndexStorage.size() << "pages and"
		<< other.localIndexTableOfContents.size() << "words";
}

QVector<const PageMetadata*> Indexer::searchByWords(const QStringList &words) const
{
	qDebug("Indexer::searchByWords");
	QVector<const PageMetadata*> searchResults;
	if (words.isEmpty())
	{
		return searchResults;
	}
	QStringList lowerWords;
	lowerWords.reserve(words.size());
	for (const QString &word : words)
	{
		lowerWords.append(word.toLower());
	}
	QString smallestSetKey;
	qsizetype smallestSetSize = LONG_MAX;
	for (const QString &lowerWord : lowerWords)
	{
		if (!localIndexTableOfContents.contains(lowerWord))
		{
			return searchResults;
		}
		if (localIndexTableOfContents[lowerWord].size() < smallestSetSize)
		{
			smallestSetSize = localIndexTableOfContents[lowerWord].size();
			smallestSetKey = lowerWord;
		}
	}
	lowerWords.removeAll(smallestSetKey);
	QSet<uint64_t> pageSubsetIntersection = localIndexTableOfContents[smallestSetKey];
	for(const QString &lowerWord : lowerWords)
	{
		const QSet<uint64_t> &pageSubset=localIndexTableOfContents[lowerWord];
		pageSubsetIntersection.intersect(pageSubset);
		if(pageSubsetIntersection.isEmpty())
		{
			return searchResults;
		}
	}
	for(uint64_t hash : pageSubsetIntersection)
	{
		const PageMetadata *searchResult=localIndexStorage.value(hash, nullptr);
		if(searchResult!=nullptr)
		{
			searchResults.append(searchResult);
		}
	}
	return searchResults;
}

void Indexer::addPage(const PageMetadata &page_metadata)
{
	if(page_metadata.words.isEmpty())
	{
		return;
	}
	if(page_metadata.url.isEmpty())
	{
		return;
	}
	if(localIndexStorage.contains(page_metadata.contentHash))
	{
		return;
	}
	PageMetadata *pageMetaDataCopy=new PageMetadata(page_metadata);
	localIndexStorage.insert(page_metadata.contentHash, pageMetaDataCopy);
	for (QMap<QString, uint64_t>::const_iterator it = page_metadata.words.constBegin(); it != page_metadata.words.constEnd(); it++)
	{
		const QString word = it.key();
		localIndexTableOfContents[word].insert(page_metadata.contentHash);
	}
}
