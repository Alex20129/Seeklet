#include <QDir>
#include "main.hpp"
#include "indexer.hpp"
#include "util.hpp"

PageMetadata::PageMetadata()
{
	urlHash=0;
	contentHash=0;
	wordsTotal=0;
}

void PageMetadata::writeToStream(QDataStream &stream) const
{
	stream << this->urlHash;
	stream << this->contentHash;
	stream << this->wordsTotal;
	stream << this->timeStamp;
	stream << this->title;
	stream << this->url;
	stream << this->wordsAsHashes;
}

void PageMetadata::readFromStream(QDataStream &stream)
{
	stream >> this->urlHash;
	stream >> this->contentHash;
	stream >> this->wordsTotal;
	stream >> this->timeStamp;
	stream >> this->title;
	stream >> this->url;
	stream >> this->wordsAsHashes;
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
	if(wordsAsHashes.isEmpty())
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
	setDatabaseDirectory(gSettings->databaseDirectory());
}

Indexer::~Indexer()
{
	this->clear();
}

void Indexer::printPageMetadata(const PageMetadata &page_md)
{
	qDebug() << "==[ page metadata ]==";
	qDebug() << "title:" << page_md.title;
	qDebug() << "url:" << page_md.url;
	qDebug() << "timeStamp:" << page_md.timeStamp.toString();
	qDebug("contentHash: %llX", page_md.contentHash);
	qDebug("urlHash: %llX", page_md.urlHash);
	qDebug() << "words:";
	QHash<quint64, quint64>::const_iterator pageTfIt;
	for(pageTfIt = page_md.wordsAsHashes.constBegin(); pageTfIt != page_md.wordsAsHashes.constEnd(); pageTfIt++)
	{
		quint64 wordHash=pageTfIt.key();
		quint64 wordTf=pageTfIt.value();
		const QString &word=mDictionaryLookupTable.value(wordHash);
		qDebug() << word << wordTf;
	}
}

void Indexer::clear()
{
	qDeleteAll(mIndexByContentHash);
	mIndexByContentHash.clear();
	mIndexByUrlHash.clear();
	mIndexTableOfContents.clear();
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
	QHash<quint64, PageMetadata *>::const_iterator cHashIt;
	this->mDictionaryLookupTable.insert(other.mDictionaryLookupTable);
	for(cHashIt = other.mIndexByContentHash.constBegin(); cHashIt != other.mIndexByContentHash.constEnd(); cHashIt++)
	{
		const PageMetadata *pageMetaDataPtr=cHashIt.value();
		if(nullptr!=pageMetaDataPtr)
		{
			addPage(*pageMetaDataPtr);
		}
	}
}

const PageMetadata *Indexer::getPageMetadataByContentHash(uint64_t content_hash) const
{
	const PageMetadata *page=mIndexByContentHash.value(content_hash, nullptr);
	return page;
}

const PageMetadata *Indexer::getPageMetadataByUrlHash(uint64_t url_hash) const
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
	QString smallestSetKey;
	qsizetype smallestSetSize = LONG_LONG_MAX;
	for(const QString &word : words)
	{
		if(!mIndexTableOfContents.contains(word))
		{
			return searchResults;
		}
		if(mIndexTableOfContents[word].size() < smallestSetSize)
		{
			smallestSetSize = mIndexTableOfContents[word].size();
			smallestSetKey = word;
		}
	}
	words.removeAll(smallestSetKey);
	QSet<quint64> pageSubsetIntersection=mIndexTableOfContents[smallestSetKey];
	for(const QString &word : words)
	{
		const QSet<quint64> &pageSubset=mIndexTableOfContents[word];
		pageSubsetIntersection.intersect(pageSubset);
		if(pageSubsetIntersection.isEmpty())
		{
			return searchResults;
		}
	}
	for(quint64 hash : pageSubsetIntersection)
	{
		const PageMetadata *searchResult=mIndexByContentHash.value(hash, nullptr);
		if(nullptr!=searchResult)
		{
			searchResults.append(searchResult);
		}
	}
	return searchResults;
}

double Indexer::calculateTfIdfScore(uint64_t content_hash, const QStringList &words) const
{
	const PageMetadata *pagePtr = mIndexByContentHash.value(content_hash, nullptr);
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
	const PageMetadata *pagePtr = mIndexByContentHash.value(content_hash, nullptr);
	double score = calculateTfIdfScore(pagePtr, word);
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
	quint64 wordHash=xorshift_hash_64(word.toUtf8());
	if(page->wordsAsHashes.value(wordHash, 0)==0)
	{
		return 0.0;
	}
	double tfNormalized=page->wordsAsHashes.value(wordHash, 0);
	tfNormalized/=pageWordsTotal;
	if(!mIndexTableOfContents.contains(word))
	{
		return 0.0;
	}
	const QSet<quint64> &pageSubset=mIndexTableOfContents[word];
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
	uint64_t WIP; // TODO: Very slow implementation. Need to make it faster.

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
			if(tfIdfScore > pageScores.at(i))
			{
				pageScores.insert(i, tfIdfScore);
				pagesNewOrder.insert(i, page);
				inserted = true;
				break;
			}
		}
		if(!inserted)
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
	if(mIndexByUrlHash.contains(page_metadata.urlHash))
	{
		return;
	}
	if(mIndexByContentHash.contains(page_metadata.contentHash))
	{
		return;
	}
	PageMetadata *pageMetaDataCopy=new PageMetadata(page_metadata);
	bool dictionaryMismatch=false;
	QHash<quint64, quint64>::const_iterator pageTfIt;
	for(pageTfIt = pageMetaDataCopy->wordsAsHashes.constBegin(); pageTfIt != pageMetaDataCopy->wordsAsHashes.constEnd(); pageTfIt++)
	{
		quint64 wordHash=pageTfIt.key();
		quint64 wordTf=pageTfIt.value();
		if(mDictionaryLookupTable.contains(wordHash) && wordTf>0)
		{
			continue;
		}
		else
		{
			dictionaryMismatch=true;
		}
	}
	if(dictionaryMismatch)
	{
		delete pageMetaDataCopy;
		return;
	}
	for(pageTfIt = pageMetaDataCopy->wordsAsHashes.constBegin(); pageTfIt != pageMetaDataCopy->wordsAsHashes.constEnd(); pageTfIt++)
	{
		quint64 wordHash=pageTfIt.key();
		quint64 wordTf=pageTfIt.value();
		const QString &word=mDictionaryLookupTable.value(wordHash);
		if(!word.isEmpty() && wordTf>0)
		{
			mIndexTableOfContents[word].insert(pageMetaDataCopy->contentHash);
		}
	}
	mIndexByUrlHash.insert(pageMetaDataCopy->urlHash, pageMetaDataCopy);
	mIndexByContentHash.insert(pageMetaDataCopy->contentHash, pageMetaDataCopy);
}

void Indexer::addWord(const QString &word)
{
	quint64 wordHash=xorshift_hash_64(word.toUtf8());
	mDictionaryLookupTable.insert(wordHash, word);
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
	QString tocFilePath = dbDir.filePath("index_toc.dat");
	QString mdFilePath = dbDir.filePath("index_md.dat");

	QFile tocFile(tocFilePath);
	if(tocFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QDataStream tocFileStream(&tocFile);
		tocFileStream.setVersion(QDataStream::Qt_6_0);
		tocFileStream << dataStreamVersion;
		tocFileStream << mIndexTableOfContents;
		tocFile.close();
		qDebug() << "Table of contents has been saved successfully.";
		qDebug() << mIndexTableOfContents.size() << "entries saved.";
	}
	else
	{
		qWarning() << "Failed to open" << tocFilePath << "for writing";
	}

	QFile mdFile(mdFilePath);
	if(mdFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		QDataStream mdFileStream(&mdFile);
		mdFileStream.setVersion(QDataStream::Qt_6_0);
		mdFileStream << dataStreamVersion;
		quint64 numOfPages = mIndexByContentHash.size();
		mdFileStream << numOfPages;
		QHash<quint64, PageMetadata *>::const_iterator cHashIt;
		for(cHashIt = mIndexByContentHash.constBegin(); cHashIt!=mIndexByContentHash.constEnd(); cHashIt++)
		{
			const PageMetadata *pageMDPtr = cHashIt.value();
			if(nullptr!=pageMDPtr)
			{
				pageMDPtr->writeToStream(mdFileStream);
			}
		}
		mdFile.close();
		qDebug() << "Metadata has been saved successfully.";
		qDebug() << mIndexByContentHash.size() << "entries saved.";
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

	quint64 dataStreamVersion, numOfPages;
	QString tocFilePath = dbDir.filePath("index_toc.dat");
	QString mdFilePath = dbDir.filePath("index_md.dat");

	this->clear();

	QFile tocFile(tocFilePath);
	if(tocFile.open(QIODevice::ReadOnly))
	{
		QDataStream tocFileStream(&tocFile);
		tocFileStream.setVersion(QDataStream::Qt_6_0);
		tocFileStream >> dataStreamVersion;
		if(dataStreamVersion==(quint64)(QDataStream::Qt_6_0))
		{
			tocFileStream >> mIndexTableOfContents;
			QHash<QString, QSet<quint64>>::ConstIterator tocWordsIt;
			for(tocWordsIt=mIndexTableOfContents.constBegin(); tocWordsIt!=mIndexTableOfContents.constEnd(); tocWordsIt++)
			{
				const QString &tocWord=tocWordsIt.key();
				if(!tocWord.isEmpty())
				{
					quint64 wordHash=xorshift_hash_64(tocWord.toUtf8());
					mDictionaryLookupTable.insert(wordHash, tocWord);
				}
			}
			qDebug() << "Table of contents has been loaded successfully:" << mIndexTableOfContents.size() << "new records.";
		}
		else
		{
			qWarning() << "Unknown file version. Cannot load data from:" << tocFilePath;
		}
		tocFile.close();
	}
	else
	{
		qWarning() << "Failed to open" << tocFilePath << "for reading";
	}

	QFile mdFile(mdFilePath);
	if(mdFile.open(QIODevice::ReadOnly))
	{
		QDataStream mdFileStream(&mdFile);
		mdFileStream.setVersion(QDataStream::Qt_6_0);
		mdFileStream >> dataStreamVersion;
		if(dataStreamVersion==(quint64)(QDataStream::Qt_6_0))
		{
			mdFileStream >> numOfPages;
			for(quint64 page=0; page<numOfPages; page++)
			{
				PageMetadata newPageMetadata;
				newPageMetadata.readFromStream(mdFileStream);
				if(mIndexByUrlHash.contains(newPageMetadata.urlHash))
				{
					continue;
				}
				if(mIndexByContentHash.contains(newPageMetadata.contentHash))
				{
					continue;
				}
				PageMetadata *pageMetadataCopy=new PageMetadata(newPageMetadata);
				mIndexByUrlHash.insert(pageMetadataCopy->urlHash, pageMetadataCopy);
				mIndexByContentHash.insert(pageMetadataCopy->contentHash, pageMetadataCopy);
			}
			if(mIndexByContentHash.size()==(qsizetype)numOfPages)
			{
				qDebug() << "Metadata has been loaded successfully:" << mIndexByContentHash.size() << "new records.";
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
#ifndef NDEBUG
	QHash<quint64, PageMetadata *>::const_iterator cHashIt;
	for(cHashIt = mIndexByContentHash.constBegin(); cHashIt!=mIndexByContentHash.constEnd(); cHashIt++)
	{
		const PageMetadata *pageMDPtr = cHashIt.value();
		if(nullptr!=pageMDPtr)
		{
			printPageMetadata(*pageMDPtr);
		}
	}
#endif
}

#ifndef NDEBUG
void Indexer::searchTest()
{
	qDebug("Indexer::searchTest");
	QStringList words;
	// words.append("business");
	// words.append("suit");
	words.append("office");
	// words.append("hoodie");
	const QVector<const PageMetadata *> searchResults=this->searchPagesByWords(words);
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
			// searchResultFile.write(pageMDPtr->timeStamp.toString().toStdString().data());
			// searchResultFile.write("\n");
			// searchResultFile.write(QByteArray::number(pageMDPtr->urlHash).toStdString().data());
			// searchResultFile.write("\n");
			// searchResultFile.write(QByteArray::number(pageMDPtr->contentHash).toStdString().data());
			// searchResultFile.write("\n");
		}
		searchResultFile.write("</html>\n");
		searchResultFile.close();
	}
}
#endif
