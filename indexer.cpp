#include <QDir>
#include "main.hpp"
#include "indexer.hpp"
#include "xorshift_hash.hpp"

void PageMetadata::writeToStream(QDataStream &stream) const
{
	stream << this->title;
	stream << this->url;
	stream << this->timeStamp;
	stream << this->wordsAsHashes;
	stream << this->contentHash;
	stream << this->wordsTotal;
}

void PageMetadata::readFromStream(QDataStream &stream)
{
	stream >> this->title;
	stream >> this->url;
	stream >> this->timeStamp;
	stream >> this->wordsAsHashes;
	stream >> this->contentHash;
	stream >> this->wordsTotal;
}

bool PageMetadata::isValid() const
{
	if(wordsTotal==0)
	{
		return(false);
	}
	if(wordsAsHashes.isEmpty())
	{
		return(false);
	}
	if(url.isEmpty())
	{
		return(false);
	}
	if(!timeStamp.isValid())
	{
		return(false);
	}
	return(true);
}

#ifndef NDEBUG
void Indexer::printPageMetadata(const PageMetadata &page_md)
{
	qDebug() << "==[ page metadata ]==";
	qDebug() << "title:" << page_md.title;
	qDebug() << "url:" << page_md.url;
	qDebug() << "timeStamp:" << page_md.timeStamp.toString();
	qDebug() << "contentHash:"
			<< QByteArray::fromRawData((char *)&page_md.contentHash.second, 8).toHex()
			<< QByteArray::fromRawData((char *)&page_md.contentHash.first, 8).toHex();
	qDebug() << "words:";
	QHash<Hash64, quint64>::const_iterator pageTfIt;
	for(pageTfIt=page_md.wordsAsHashes.constBegin(); pageTfIt != page_md.wordsAsHashes.constEnd(); pageTfIt++)
	{
		Hash64 wordHash=pageTfIt.key();
		quint64 wordTf=pageTfIt.value();
		const QString &word=mDictionaryLookupTable.value(wordHash);
		qDebug() << word << wordTf;
	}
}
#endif

Indexer::Indexer(QObject *parent) : QObject(parent)
{
	setDatabaseDirectory(gSettings->databaseDirectory());
}

Indexer::~Indexer()
{
	this->clear();
}

void Indexer::clear()
{
	qDeleteAll(mIndexByContentHash);
	mIndexByContentHash.clear();
	mIndexByUrlHash.clear();
	mTableOfContents.clear();
	mDictionaryLookupTable.clear();
}

void Indexer::setDatabaseDirectory(const QString &database_directory)
{
	mDatabaseDirectory=database_directory;
	if(!mDatabaseDirectory.isEmpty())
	{
		QDir dir(mDatabaseDirectory);
		if(!dir.exists())
		{
			if(!dir.mkpath("."))
			{
				mDatabaseDirectory.clear();
			}
		}
	}
}

void Indexer::merge(const Indexer &other)
{
	this->mDictionaryLookupTable.insert(other.mDictionaryLookupTable);
	QHash<Hash128, PageMetadata *>::const_iterator cHashIt;
	for(cHashIt=other.mIndexByContentHash.constBegin(); cHashIt != other.mIndexByContentHash.constEnd(); cHashIt++)
	{
		const PageMetadata *pageMetaDataPtr=cHashIt.value();
		if(nullptr!=pageMetaDataPtr)
		{
			addPage(*pageMetaDataPtr);
		}
	}
}

const PageMetadata *Indexer::getPageMetadataByContentHash(const Hash128 &content_hash) const
{
	const PageMetadata *page=mIndexByContentHash.value(content_hash, nullptr);
	return page;
}

const PageMetadata *Indexer::getPageMetadataByUrlHash(const Hash128 &url_hash) const
{
	const PageMetadata *page=mIndexByUrlHash.value(url_hash, nullptr);
	return page;
}

QVector<const PageMetadata *> Indexer::searchPagesByWords(QStringList words) const
{
	QVector<const PageMetadata *> searchResults;
	if(words.isEmpty())
	{
		return searchResults;
	}
	uint64_t smallestSetKey;
	QString smallestSetWord;
	qsizetype smallestSetSize=LONG_LONG_MAX;
	for(const QString &word : words)
	{
		Hash64 wordHash=xorshiftstar_hash_64(word.toUtf8());
		if(!mTableOfContents.contains(wordHash))
		{
			return searchResults;
		}
		if(mTableOfContents[wordHash].size() < smallestSetSize)
		{
			smallestSetSize=mTableOfContents[wordHash].size();
			smallestSetKey=wordHash;
			smallestSetWord=word;
		}
	}
	words.removeAll(smallestSetWord);
	QSet<Hash128> pageSubsetIntersection=mTableOfContents[smallestSetKey];
	for(const QString &word : words)
	{
		Hash64 wordHash=xorshiftstar_hash_64(word.toUtf8());
		const QSet<Hash128> &pageSubset=mTableOfContents[wordHash];
		pageSubsetIntersection.intersect(pageSubset);
		if(pageSubsetIntersection.isEmpty())
		{
			return searchResults;
		}
	}
	for(Hash128 hash : pageSubsetIntersection)
	{
		const PageMetadata *searchResult=mIndexByContentHash.value(hash, nullptr);
		if(nullptr!=searchResult)
		{
			searchResults.append(searchResult);
		}
	}
	return searchResults;
}

double Indexer::calculateTfIdfScore(const Hash128 &content_hash, const QStringList &words) const
{
	const PageMetadata *pagePtr=mIndexByContentHash.value(content_hash, nullptr);
	double totalScore=calculateTfIdfScore(pagePtr, words);
	return totalScore;
}

double Indexer::calculateTfIdfScore(const PageMetadata *page, const QStringList &words) const
{
	double totalScore=0.0;
	for(const QString &word : words)
	{
		totalScore += calculateTfIdfScore(page, word);
	}
	return totalScore;
}

double Indexer::calculateTfIdfScore(const Hash128 &content_hash, const QString &word) const
{
	const PageMetadata *pagePtr=mIndexByContentHash.value(content_hash, nullptr);
	double score=calculateTfIdfScore(pagePtr, word);
	return score;
}

double Indexer::calculateTfIdfScore(const PageMetadata *page, const QString &word) const
{
	if(nullptr==page)
	{
		return 0.0;
	}
	if(page->wordsTotal==0)
	{
		return 0.0;
	}
	double pageWordsTotal=page->wordsTotal;
	Hash64 wordHash=xorshiftstar_hash_64(word.toUtf8());
	if(page->wordsAsHashes.value(wordHash, 0)==0)
	{
		return 0.0;
	}
	double tfNormalized=page->wordsAsHashes.value(wordHash, 0);
	tfNormalized/=pageWordsTotal;
	if(!mTableOfContents.contains(wordHash))
	{
		return 0.0;
	}
	const QSet<Hash128> &pageSubset=mTableOfContents[wordHash];
	if(pageSubset.isEmpty())
	{
		return 0.0;
	}
	double df=pageSubset.size();
	double pagesTotal=mIndexByContentHash.size();
	double idf=std::log(pagesTotal / df);
	return (tfNormalized*idf);
}

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
	qsizetype numOfPages=pages.size();
	QVector<ScoredPage> scoredPages;
	scoredPages.reserve(numOfPages);
	for(const PageMetadata *page : pages)
	{
		double score=calculateTfIdfScore(page, words);
		scoredPages.append({score, page});
	}
	std::sort(scoredPages.begin(), scoredPages.end(), pageScoreComparator);
	pages.clear();
	pages.reserve(numOfPages);
	for(const ScoredPage &sp : scoredPages)
	{
		pages.append(sp.page);
	}
}

void Indexer::addPage(const PageMetadata &page_metadata)
{
	if(!page_metadata.isValid())
	{
		return;
	}
	if(mIndexByContentHash.contains(page_metadata.contentHash))
	{
		return;
	}
	QHash<Hash64, quint64>::const_iterator pageTfIt;
	for(pageTfIt=page_metadata.wordsAsHashes.constBegin(); pageTfIt != page_metadata.wordsAsHashes.constEnd(); pageTfIt++)
	{
		Hash64 wordHash=pageTfIt.key();
		quint64 wordTf=pageTfIt.value();
		if(mDictionaryLookupTable.contains(wordHash) && wordTf>0)
		{
			continue;
		}
		else
		{
			return;
		}
	}
	PageMetadata *newPageMetaData=new PageMetadata(page_metadata);
	Hash128 urlHash=xorshiftstar_hash_128(newPageMetaData->url);
	if(mIndexByUrlHash.contains(urlHash))
	{
		PageMetadata *oldPageMetaData=mIndexByUrlHash.value(urlHash);
		deletePage(oldPageMetaData);
	}
	for(pageTfIt=newPageMetaData->wordsAsHashes.constBegin(); pageTfIt != newPageMetaData->wordsAsHashes.constEnd(); pageTfIt++)
	{
		Hash64 wordHash=pageTfIt.key();
		mTableOfContents[wordHash].insert(newPageMetaData->contentHash);
	}
	mIndexByUrlHash.insert(urlHash, newPageMetaData);
	mIndexByContentHash.insert(newPageMetaData->contentHash, newPageMetaData);
}

void Indexer::deletePage(PageMetadata *page_metadata)
{
	if(nullptr==page_metadata)
	{
		return;
	}
	Hash128 contentHash=page_metadata->contentHash;
	Hash128 urlHash=xorshiftstar_hash_128(page_metadata->url);
	mIndexByContentHash.remove(contentHash);
	mIndexByUrlHash.remove(urlHash);
	QHash<Hash64, quint64>::const_iterator wordHashIt;
	for(wordHashIt=page_metadata->wordsAsHashes.constBegin(); wordHashIt != page_metadata->wordsAsHashes.constEnd(); wordHashIt++)
	{
		Hash64 wordHash=wordHashIt.key();
		QHash<Hash64, QSet<Hash128>>::iterator tocIt=mTableOfContents.find(wordHash);
		if(tocIt != mTableOfContents.end())
		{
			tocIt.value().remove(contentHash);
			if(tocIt.value().isEmpty())
			{
				mTableOfContents.erase(tocIt);
			}
		}
	}
	delete page_metadata;
}

void Indexer::addWord(const QString &word)
{
	if(!word.isEmpty())
	{
		Hash64 wordHash=xorshiftstar_hash_64(word.toUtf8());
		mDictionaryLookupTable.insert(wordHash, word);
	}
}

void Indexer::save()
{
	qDebug("Indexer::save");
	if(mDatabaseDirectory.isEmpty())
	{
		return;
	}
	QDir dbDir(mDatabaseDirectory);

	quint64 dataStreamVersion=QDataStream::Qt_6_0;
	QString dltFilePath=dbDir.filePath("index_dlt.dat");
	QString mdFilePath=dbDir.filePath("index_md.dat");

	QFile dltFile(dltFilePath);
	if(dltFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		quint64 dltSize=mDictionaryLookupTable.size();
		QDataStream dltFileStream(&dltFile);
		dltFileStream.setVersion(QDataStream::Qt_6_0);
		dltFileStream << dataStreamVersion;
		dltFileStream << dltSize;
		dltFileStream << mDictionaryLookupTable;
		dltFile.close();
		qInfo() << "Dictionary lookup table has been saved successfully:" << mDictionaryLookupTable.size() << "records saved.";
	}
	else
	{
		qWarning() << "Failed to open" << dltFilePath << "for writing";
	}

	QFile mdFile(mdFilePath);
	if(mdFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		quint64 numOfPages=mIndexByContentHash.size();
		QDataStream mdFileStream(&mdFile);
		mdFileStream.setVersion(QDataStream::Qt_6_0);
		mdFileStream << dataStreamVersion;
		mdFileStream << numOfPages;
		QHash<Hash128, PageMetadata *>::const_iterator cHashIt;
		for(cHashIt=mIndexByContentHash.constBegin(); cHashIt!=mIndexByContentHash.constEnd(); cHashIt++)
		{
			const PageMetadata *pageMDPtr=cHashIt.value();
			if(nullptr!=pageMDPtr)
			{
				pageMDPtr->writeToStream(mdFileStream);
			}
		}
		mdFile.close();
		qInfo() << "Metadata has been saved successfully:" << mIndexByContentHash.size() << "records saved.";
	}
	else
	{
		qWarning() << "Failed to open" << mdFilePath << "for writing";
	}
}

void Indexer::load()
{
	qDebug("Indexer::load");
	if(mDatabaseDirectory.isEmpty())
	{
		return;
	}
	QDir dbDir(mDatabaseDirectory);

	quint64 dataStreamVersion;
	QString dltFilePath=dbDir.filePath("index_dlt.dat");
	QString mdFilePath=dbDir.filePath("index_md.dat");

	this->clear();

	QFile dltFile(dltFilePath);
	if(dltFile.open(QIODevice::ReadOnly))
	{
		quint64 dltSize;
		QDataStream dltFileStream(&dltFile);
		dltFileStream.setVersion(QDataStream::Qt_6_0);
		dltFileStream >> dataStreamVersion;
		if(dataStreamVersion==(quint64)(QDataStream::Qt_6_0))
		{
			dltFileStream >> dltSize;
			if(dltSize>0)
			{
				mDictionaryLookupTable.reserve(dltSize);
				mTableOfContents.reserve(dltSize);
				dltFileStream >> mDictionaryLookupTable;
				qInfo() << "Dictionary lookup table has been loaded successfully:" << mDictionaryLookupTable.size() << "new records.";
			}
		}
		else
		{
			qWarning() << "Unknown file version. Cannot load data from:" << dltFilePath;
		}
		dltFile.close();
	}
	else
	{
		qWarning() << "Failed to open" << dltFilePath << "for reading";
	}

	QFile mdFile(mdFilePath);
	if(mdFile.open(QIODevice::ReadOnly))
	{
		quint64 numOfPages;
		QDataStream mdFileStream(&mdFile);
		mdFileStream.setVersion(QDataStream::Qt_6_0);
		mdFileStream >> dataStreamVersion;
		if(dataStreamVersion==(quint64)(QDataStream::Qt_6_0))
		{
			mdFileStream >> numOfPages;
			mIndexByContentHash.reserve(numOfPages);
			mIndexByUrlHash.reserve(numOfPages);
			for(quint64 page=0; page<numOfPages; page++)
			{
				PageMetadata newPageMetadata;
				newPageMetadata.readFromStream(mdFileStream);
				addPage(newPageMetadata);
			}
			if(mIndexByContentHash.size()==(qsizetype)numOfPages)
			{
				qInfo() << "Metadata has been loaded successfully:" << mIndexByContentHash.size() << "new records.";
			}
			else
			{
				qWarning() << "Warning:" << numOfPages << "metadata records was expected, but only" <<
					mIndexByContentHash.size() << "has been loaded.";
				qWarning()<< "Metadata file possibly corrupted:" << mdFilePath;
			}
		}
		else
		{
			qWarning() << "Unknown file version. Cannot load data from:" << mdFilePath;
		}
		mdFile.close();
	}
	else
	{
		qWarning() << "Failed to open" << mdFilePath << "for reading";
	}
}

#ifndef NDEBUG
void Indexer::searchTest()
{
	qDebug("Indexer::searchTest");
	QStringList query;
	// query.append("office");
	// query.append("business");
	// query.append("grey");
	// query.append("suit");
	// query.append("hoodie");
	query.append("wedding");
	query.append("dress");
	const QVector<const PageMetadata *> searchResults=this->searchPagesByWords(query);
	QFile searchResultFile(QString("search_result.html"));
	if(searchResultFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		searchResultFile.write("<html>\n");
		for(const PageMetadata *pageMDPtr : searchResults)
		{
			printPageMetadata(*pageMDPtr);
			searchResultFile.write("<a href=\"");
			searchResultFile.write(pageMDPtr->url.toStdString().data());
			searchResultFile.write("\">");
			searchResultFile.write(pageMDPtr->title.toStdString().data());
			searchResultFile.write("</a><br>\n");
		}
		searchResultFile.write("</html>\n");
		searchResultFile.close();
	}
}
#endif
