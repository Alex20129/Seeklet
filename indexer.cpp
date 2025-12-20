#include <QDebug>
#include "indexer.hpp"

PageMetadata::PageMetadata()
{
	urlHash=
	contentHash=
	wordsTotal=0;
}

bool PageMetadata::isValid() const
{
	bool result=true;
	if(urlHash==0)
	{
		result=false;
	}
	if(contentHash==0)
	{
		result=false;
	}
	if(words.isEmpty())
	{
		result=false;
	}
	if(url.isEmpty())
	{
		result=false;
	}
	if(!timeStamp.isValid())
	{
		result=false;
	}
	return result;
}

Indexer::Indexer(QObject *parent) : QObject(parent)
{
}

Indexer::~Indexer()
{
	qDeleteAll(localIndexByContentHash);
	localIndexByContentHash.clear();
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
	for (QHash<uint64_t, PageMetadata *>::const_iterator it = other.localIndexByContentHash.constBegin(); it != other.localIndexByContentHash.constEnd(); it++)
	{
		const uint64_t hash = it.key();
		if (!localIndexByContentHash.contains(hash))
		{
			PageMetadata *pageMetaDataCopy = new PageMetadata(*it.value());
			localIndexByContentHash.insert(hash, pageMetaDataCopy);
		}
	}
	for (QHash<QString, QSet<uint64_t>>::const_iterator it = other.localIndexTableOfContents.constBegin(); it != other.localIndexTableOfContents.constEnd(); it++)
	{
		const QString &word = it.key();
		const QSet<uint64_t> &hashes = it.value();
		localIndexTableOfContents[word].unite(hashes);
	}
	qDebug() << "Merged index: added" << other.localIndexByContentHash.size() << "pages and"
		<< other.localIndexTableOfContents.size() << "words";
}

const PageMetadata *Indexer::getPageMetadataByContentHash(uint64_t content_hash) const
{
	const PageMetadata *page=localIndexByContentHash.value(content_hash, nullptr);
	return page;
}

const PageMetadata *Indexer::getPageMetadataByUrlHash(uint64_t url_hash) const
{
	const PageMetadata *page=localIndexByUrlHash.value(url_hash, nullptr);
	return page;
}

QVector<const PageMetadata *> Indexer::searchPagesByWords(const QStringList &words) const
{
	QVector<const PageMetadata *> searchResults;
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
	qsizetype smallestSetSize = SIZE_MAX;
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
		const PageMetadata *searchResult=localIndexByContentHash.value(hash, nullptr);
		if(nullptr!=searchResult)
		{
			searchResults.append(searchResult);
		}
	}
	return searchResults;
}

double Indexer::calculateTfIdfScore(uint64_t content_hash, const QString &word) const
{
	const PageMetadata *page=localIndexByContentHash.value(content_hash, nullptr);
	if(nullptr==page)
	{
		return 0.0;
	}
	if(page->wordsTotal==0)
	{
		return 0.0;
	}
	double pageWordsTotal=page->wordsTotal;
	if(page->words.value(word, 0)==0)
	{
		return 0.0;
	}
	double tfNormalized=page->words.value(word, 0);
	tfNormalized/=pageWordsTotal;
	if(!localIndexTableOfContents.contains(word))
	{
		return 0.0;
	}
	const QSet<uint64_t> &pageSubset=localIndexTableOfContents[word];
	if(pageSubset.isEmpty())
	{
		return 0.0;
	}
	double df=pageSubset.size();
	double pagesTotal=localIndexByContentHash.size();
	double idf=std::log(pagesTotal / df);
	return (tfNormalized*idf);
}

void Indexer::sortPagesByTfIdfScore(QVector<const PageMetadata *> &pages, const QStringList &words) const
{
}

void Indexer::addPage(const PageMetadata &page_metadata)
{
	if(!page_metadata.isValid())
	{
		return;
	}
	if(localIndexByUrlHash.contains(page_metadata.urlHash))
	{
		return;
	}
	if(localIndexByContentHash.contains(page_metadata.contentHash))
	{
		return;
	}
	PageMetadata *pageMetaDataCopy=new PageMetadata(page_metadata);
	localIndexByUrlHash.insert(pageMetaDataCopy->urlHash, pageMetaDataCopy);
	localIndexByContentHash.insert(pageMetaDataCopy->contentHash, pageMetaDataCopy);
	QMap<QString, uint64_t>::const_iterator wordsIt;
	for (wordsIt = pageMetaDataCopy->words.constBegin(); wordsIt != pageMetaDataCopy->words.constEnd(); wordsIt++)
	{
		const QString word = wordsIt.key();
		localIndexTableOfContents[word].insert(pageMetaDataCopy->contentHash);
	}
}
