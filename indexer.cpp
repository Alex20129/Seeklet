#include <QDir>
#include "configuration_keeper.hpp"
#include "indexer.hpp"

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
	stream << this->words;
}

void PageMetadata::readFromStream(QDataStream &stream)
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

const QString &Indexer::getDatabaseDirectory() const
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
	if(!dbDir.exists())
	{
		qWarning() << "Directory does not exist, cannot save database to:" << mDatabaseDirectory;
		return;
	}

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
	if(!dbDir.exists())
	{
		qWarning() << "Directory does not exist, cannot load database from:" << mDatabaseDirectory;
		return;
	}

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
		if(dataStreamVersion!=(quint64)(QDataStream::Qt_6_0))
		{
			qWarning() << "Unknown file version. Cannot load data from:" << tocFilePath;
		}
		else
		{
			tocFileStream >> mIndexTableOfContents;
			qDebug() << "Table of contents has been loaded successfully:" << mIndexTableOfContents.size() << "new records.";
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
		if(dataStreamVersion!=(quint64)(QDataStream::Qt_6_0))
		{
			qWarning() << "Unknown file version. Cannot load data from:" << mdFilePath;
		}
		else
		{
			mdFileStream >> numOfPages;
			for(quint64 page=0; page<numOfPages; page++)
			{
				PageMetadata *newPageMetadata=new PageMetadata;
				newPageMetadata->readFromStream(mdFileStream);
				if(mIndexByUrlHash.contains(newPageMetadata->urlHash))
				{
					delete newPageMetadata;
					continue;
				}
				if(mIndexByContentHash.contains(newPageMetadata->contentHash))
				{
					delete newPageMetadata;
					continue;
				}
				mIndexByUrlHash.insert(newPageMetadata->urlHash, newPageMetadata);
				mIndexByContentHash.insert(newPageMetadata->contentHash, newPageMetadata);
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
			qDebug() << pageMDPtr->title;
			qDebug() << pageMDPtr->url;
			qDebug() << pageMDPtr->timeStamp.toString();
			qDebug() << pageMDPtr->urlHash;
			qDebug() << pageMDPtr->contentHash;
			qDebug() << pageMDPtr->words.keys();
			qDebug() << "====";
		}
	}
#endif
}

void Indexer::merge(const Indexer &other)
{
	QHash<quint64, PageMetadata *>::const_iterator cHashIt;
	for(cHashIt = other.mIndexByContentHash.constBegin(); cHashIt != other.mIndexByContentHash.constEnd(); cHashIt++)
	{
		const quint64 contentHash = cHashIt.key();
		if(!mIndexByContentHash.contains(contentHash))
		{
			const PageMetadata *pageMetaDataPtr=other.mIndexByContentHash[contentHash];
			if(nullptr!=pageMetaDataPtr)
			{
				PageMetadata *pageMetaDataCopy = new PageMetadata(*pageMetaDataPtr);
				mIndexByContentHash.insert(pageMetaDataCopy->contentHash, pageMetaDataCopy);
				mIndexByUrlHash.insert(pageMetaDataCopy->urlHash, pageMetaDataCopy);
			}
		}
	}
	QHash<QString, QSet<quint64>>::const_iterator tocIt;
	for(tocIt = other.mIndexTableOfContents.constBegin(); tocIt != other.mIndexTableOfContents.constEnd(); tocIt++)
	{
		const QString &word = tocIt.key();
		const QSet<quint64> &cHashes = other.mIndexTableOfContents[word];
		mIndexTableOfContents[word].unite(cHashes);
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
	mIndexByUrlHash.insert(pageMetaDataCopy->urlHash, pageMetaDataCopy);
	mIndexByContentHash.insert(pageMetaDataCopy->contentHash, pageMetaDataCopy);
	QMap<QString, quint64>::const_iterator wordsIt;
	for(wordsIt = pageMetaDataCopy->words.constBegin(); wordsIt != pageMetaDataCopy->words.constEnd(); wordsIt++)
	{
		const QString word = wordsIt.key();
		mIndexTableOfContents[word].insert(pageMetaDataCopy->contentHash);
	}
}
