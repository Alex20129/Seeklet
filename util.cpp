#include "util.hpp"
#include "xorshift_hash.hpp"

uint64_t hash_function_64(const QByteArray &data)
{
	return xorshiftstar_hash_64((const uint8_t *)data.constData(), data.size());
}

QByteArray hash_function_128(const QByteArray &data)
{
	uint64_t hash[2];
	xorshiftstar_hash_128((const uint8_t *)data.constData(), data.size(), hash);
	return QByteArray((const char *)hash, 16);
}
