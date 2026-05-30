#include <QDir>
#include "page_metadata.hpp"

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
