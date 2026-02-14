#include <QRegularExpression>
#include "util.hpp"
#include "simple_hash.hpp"
#include "metrohash128.hpp"

static constexpr uint64_t seeklet_public_seed = 0x6812CF04168E45D6;

QMap<QString, quint64> ExtractAndCountWords(const QString &text)
{
	const QString lowerText=text.toLower();
	static const QRegularExpression wordsRegex("[^a-zа-яё]+");
	static const QRegularExpression digitsRegex("^[0-9]+$");
	QMap<QString, quint64> wordMap;
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

uint64_t hash_function_64(const QByteArray &data)
{
	return xorshiftstar_hash_64((const uint8_t *)data.constData(), data.size(), seeklet_public_seed);
}

QByteArray hash_function_128(const QByteArray &data)
{
	uint64_t hash[2];
	metrohash128_1((const uint8_t *)data.constData(), data.size(), seeklet_public_seed, hash);
	return QByteArray((const char *)hash, 16);
}
