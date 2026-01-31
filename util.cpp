#include <QRegularExpression>
#include "util.hpp"
#include "simple_hash_func.hpp"

QMap<QString, uint64_t> ExtractWordsAndFrequencies(const QString &text)
{
	const QString lowerText=text.toLower();
	static const QRegularExpression wordsRegex("[^a-zа-яё]+");
	static const QRegularExpression digitsRegex("^[0-9]+$");
	QMap<QString, uint64_t> wordMap;
	const QStringList words = lowerText.split(wordsRegex, Qt::SkipEmptyParts);
	for (const QString &word : words)
	{
		if (word.length()>2 && word.length()<33)
		{
			if (!digitsRegex.match(word).hasMatch())
			{
				wordMap[word] += 1;
			}
		}
	}
	return wordMap;
}

uint64_t mwc_hash_64(const QByteArray &data)
{
	return mwc_hash_64((const uint8_t *)data.data(), data.size());
}

uint64_t fnv1a_hash_64(const QByteArray &data)
{
	return fnv1a_hash_64((const uint8_t *)data.data(), data.size());
}

uint64_t xorshift_hash_64(const QByteArray &data)
{
	return xorshift_hash_64((const uint8_t *)data.data(), data.size());
}

uint64_t xorshiftstar_hash_64(const QByteArray &data)
{
	return xorshiftstar_hash_64((const uint8_t *)data.data(), data.size());
}
