#include "indexer.hpp"

PageMetadata::PageMetadata()
{
	urlHash=0;
	contentHash=0;
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
	if(wordsTotal==0)
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
	localIndexByUrlHash.clear();
}

//TODO: init, save, load
void Indexer::initialize(const QString &db_path)
{
	if(db_path.isEmpty())
	{
		return;
	}
}

void Indexer::load(const QString &db_path)
{
	if(db_path.isEmpty())
	{
		return;
	}
}

void Indexer::save(const QString &db_path)
{
	if(db_path.isEmpty())
	{
		return;
	}
}

void Indexer::merge(const Indexer &other)
{
	QHash<uint64_t, PageMetadata *>::const_iterator cHashIt;
	for(cHashIt = other.localIndexByContentHash.constBegin(); cHashIt != other.localIndexByContentHash.constEnd(); cHashIt++)
	{
		const uint64_t contentHash = cHashIt.key();
		if(!localIndexByContentHash.contains(contentHash))
		{
			const PageMetadata *pageMetaDataPtr=other.localIndexByContentHash[contentHash];
			if(nullptr!=pageMetaDataPtr)
			{
				PageMetadata *pageMetaDataCopy = new PageMetadata(*pageMetaDataPtr);
				localIndexByContentHash.insert(pageMetaDataCopy->contentHash, pageMetaDataCopy);
				localIndexByUrlHash.insert(pageMetaDataCopy->urlHash, pageMetaDataCopy);
			}
		}
	}
	QHash<QString, QSet<uint64_t>>::const_iterator tocIt;
	for(tocIt = other.localIndexTableOfContents.constBegin(); tocIt != other.localIndexTableOfContents.constEnd(); tocIt++)
	{
		const QString &word = tocIt.key();
		const QSet<uint64_t> &cHashes = other.localIndexTableOfContents[word];
		localIndexTableOfContents[word].unite(cHashes);
	}
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

QVector<const PageMetadata *> Indexer::searchPagesByWords(QStringList words) const
{
	QVector<const PageMetadata *> searchResults;
	if (words.isEmpty())
	{
		return searchResults;
	}
	QString smallestSetKey;
	qsizetype smallestSetSize = SIZE_MAX;
	for(const QString &word : words)
	{
		if(!localIndexTableOfContents.contains(word))
		{
			return searchResults;
		}
		if(localIndexTableOfContents[word].size() < smallestSetSize)
		{
			smallestSetSize = localIndexTableOfContents[word].size();
			smallestSetKey = word;
		}
	}
	words.removeAll(smallestSetKey);
	QSet<uint64_t> pageSubsetIntersection=localIndexTableOfContents[smallestSetKey];
	for(const QString &word : words)
	{
		const QSet<uint64_t> &pageSubset=localIndexTableOfContents[word];
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

double Indexer::calculateTfIdfScore(uint64_t content_hash, const QStringList &words) const
{
	const PageMetadata *pagePtr = localIndexByContentHash.value(content_hash, nullptr);
	double totalScore = calculateTfIdfScore(pagePtr, words);
	return totalScore;
}

double Indexer::calculateTfIdfScore(const PageMetadata *page, const QStringList &words) const
{
	double totalScore = 0.0;
	for(const QString &word : words)
	{
		totalScore += calculateTfIdfScore(page, word);
	}
	return totalScore;
}

double Indexer::calculateTfIdfScore(uint64_t content_hash, const QString &word) const
{
	const PageMetadata *pagePtr = localIndexByContentHash.value(content_hash, nullptr);
	double score = calculateTfIdfScore(pagePtr, word);
	return score;
}

double Indexer::calculateTfIdfScore(const PageMetadata *page, const QString &word) const
{
	if(nullptr == page)
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

// WIP. Very slow implementation.
// TODO: need to make it faster
void Indexer::sortPagesByTfIdfScore(QVector<const PageMetadata *> &pages, const QStringList &words) const
{
	if(pages.isEmpty())
	{
		return;
	}
	if(words.isEmpty())
	{
		return;
	}

	QVector<const PageMetadata *> pagesNewOrder;
	pagesNewOrder.reserve(pages.size());

	QVector<double> pageScores;
	pageScores.reserve(pages.size());

	for(const PageMetadata *page : pages)
	{
		double tfIdfScore=calculateTfIdfScore(page, words);
		bool inserted = false;
		for (int i = 0; i < pageScores.size(); ++i)
		{
			if (tfIdfScore > pageScores.at(i))
			{
				pageScores.insert(i, tfIdfScore);
				pagesNewOrder.insert(i, page);
				inserted = true;
				break;
			}
		}
		if (!inserted)
		{
			pageScores.append(tfIdfScore);
			pagesNewOrder.append(page);
		}
	}
	pages=pagesNewOrder;
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
	for(wordsIt = pageMetaDataCopy->words.constBegin(); wordsIt != pageMetaDataCopy->words.constEnd(); wordsIt++)
	{
		const QString word = wordsIt.key();
		localIndexTableOfContents[word].insert(pageMetaDataCopy->contentHash);
	}
}
