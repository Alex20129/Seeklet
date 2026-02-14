#include "util.hpp"
#include "simple_hash.hpp"
#include "metrohash128.hpp"

static constexpr uint64_t SEEKLET_PUBLIC_SEED = 0x6812CF04168E45D6;

uint64_t hash_function_64(const QByteArray &data)
{
	return xorshiftstar_hash_64((const uint8_t *)data.constData(), data.size(), SEEKLET_PUBLIC_SEED);
}

QByteArray hash_function_128(const QByteArray &data)
{
	uint64_t hash[2];
	metrohash128_1((const uint8_t *)data.constData(), data.size(), SEEKLET_PUBLIC_SEED, hash);
	return QByteArray((const char *)hash, 16);
}
