#include "indexer.hpp"
#include <QDir>

PageMetadata::PageMetadata()
{
	urlHash=0;
	contentHash=0;
	wordsTotal=0;
}

void PageMetadata::WriteToStream(QDataStream &stream) const
{
	stream << this->urlHash;
	stream << this->contentHash;
	stream << this->wordsTotal;
	stream << this->timeStamp;
	stream << this->title;
	stream << this->url;
	stream << this->words;
}

void PageMetadata::ReadFromStream(QDataStream &stream)
{
	stream >> this->urlHash;
	stream >> this->contentHash;
	stream >> this->wordsTotal;
	stream >> this->timeStamp;
	stream >> this->title;
	stream >> this->url;
	stream >> this->words;
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
	this->clear();
}

void Indexer::clear()
{
	qDeleteAll(localIndexByContentHash);
	localIndexByContentHash.clear();
	localIndexByUrlHash.clear();
	localIndexTableOfContents.clear();
}

void Indexer::setDatabaseDirectory(const QString &database_directory)
{
	qDebug("Indexer::setDatabaseDirectory");
	mDatabaseDirectory=database_directory;
	if(mDatabaseDirectory.isEmpty())
	{
		qDebug() << "Path is empty.";
		return;
	}
	QDir dir(mDatabaseDirectory);
	if(!dir.exists())
	{
		if(!dir.mkpath("."))
		{
			qDebug() << "Failed to create database directory:" << mDatabaseDirectory;
			return;
		}
		qDebug() << "Created new database directory:" << mDatabaseDirectory;
	}
	else
	{
		qDebug() << "Existing database directory will be used:" << mDatabaseDirectory;
	}
}

QString Indexer::getDatabaseDirectory() const
{
	return mDatabaseDirectory;
}

void Indexer::save(const QString &database_directory)
{
	qDebug("Indexer::save");
	if(!database_directory.isEmpty())
	{
		setDatabaseDirectory(database_directory);
	}
	if(mDatabaseDirectory.isEmpty())
	{
		return;
	}
	QDir dbDir(mDatabaseDirectory);
	if (!dbDir.exists())
	{
		qDebug() << "Directory does not exist, cannot save to:" << mDatabaseDirectory;
		return;
	}

	quint64 dataStreamVersion=QDataStream::Qt_6_0;

	QString tocFilePath = dbDir.filePath("index_toc.dat");
	QString mdFilePath = dbDir.filePath("index_md.dat");

	QFile tocFile(tocFilePath);
	if (!tocFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		qDebug() << "Could not open file for writing:" << tocFilePath;
		return;
	}
	QDataStream tocFileStream(&tocFile);
	tocFileStream.setVersion(QDataStream::Qt_6_0);
	tocFileStream << dataStreamVersion;
	tocFileStream << localIndexTableOfContents;
	tocFile.close();
	qDebug() << "Saved TOC file:" << tocFilePath;

	QFile mdFile(mdFilePath);
	if (!mdFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		qDebug() << "Could not open file for writing:" << mdFilePath;
		return;
	}
	QDataStream mdFileStream(&mdFile);
	mdFileStream.setVersion(QDataStream::Qt_6_0);
	mdFileStream << dataStreamVersion;
	quint64 numOfPages = localIndexByContentHash.size();
	mdFileStream << numOfPages;
	QHash<quint64, PageMetadata *>::const_iterator cHashIt;
	for(cHashIt = localIndexByContentHash.constBegin(); cHashIt!=localIndexByContentHash.constEnd(); cHashIt++)
	{
		const PageMetadata *pm = cHashIt.value();
		if(nullptr!=pm)
		{
			pm->WriteToStream(mdFileStream);
		}
	}
	mdFile.close();
	qDebug() << "Saved MD file:" << mdFilePath;
}

void Indexer::load(const QString &database_directory)
{
	qDebug("Indexer::load");
	if(!database_directory.isEmpty())
	{
		setDatabaseDirectory(database_directory);
	}
	if(mDatabaseDirectory.isEmpty())
	{
		return;
	}
	QDir dbDir(mDatabaseDirectory);
	if (!dbDir.exists())
	{
		qDebug() << "Directory does not exist, cannot save to:" << mDatabaseDirectory;
		return;
	}

	quint64 dataStreamVersion=QDataStream::Qt_6_0;

	QString tocFilePath = dbDir.filePath("index_toc.dat");
	QString mdFilePath = dbDir.filePath("index_md.dat");
}

void Indexer::merge(const Indexer &other)
{
	QHash<quint64, PageMetadata *>::const_iterator cHashIt;
	for(cHashIt = other.localIndexByContentHash.constBegin(); cHashIt != other.localIndexByContentHash.constEnd(); cHashIt++)
	{
		const quint64 contentHash = cHashIt.key();
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
	QHash<QString, QSet<quint64>>::const_iterator tocIt;
	for(tocIt = other.localIndexTableOfContents.constBegin(); tocIt != other.localIndexTableOfContents.constEnd(); tocIt++)
	{
		const QString &word = tocIt.key();
		const QSet<quint64> &cHashes = other.localIndexTableOfContents[word];
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
	QSet<quint64> pageSubsetIntersection=localIndexTableOfContents[smallestSetKey];
	for(const QString &word : words)
	{
		const QSet<quint64> &pageSubset=localIndexTableOfContents[word];
		pageSubsetIntersection.intersect(pageSubset);
		if(pageSubsetIntersection.isEmpty())
		{
			return searchResults;
		}
	}
	for(quint64 hash : pageSubsetIntersection)
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
	const QSet<quint64> &pageSubset=localIndexTableOfContents[word];
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
	QMap<QString, quint64>::const_iterator wordsIt;
	for(wordsIt = pageMetaDataCopy->words.constBegin(); wordsIt != pageMetaDataCopy->words.constEnd(); wordsIt++)
	{
		const QString word = wordsIt.key();
		localIndexTableOfContents[word].insert(pageMetaDataCopy->contentHash);
	}
}
