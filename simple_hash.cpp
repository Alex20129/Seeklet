#include "simple_hash.hpp"

static constexpr uint64_t MWC64_ALPHA=0x00007F5454545437;
static constexpr uint64_t FNV64_INITIAL_OFFSET=0xCBF29CE484222325;
static constexpr uint64_t FNV64_PRIME=0x00000100000001B3;
static constexpr uint64_t XORSHIFT64_ALPHA=0x2545F4914F6CDD1D;
static constexpr uint64_t XORSHIFT64_INITIAL_OFFSET=0x7A643C25D6EDAD19;

uint64_t mwc_hash_64(const uint8_t *data, uint64_t len)
{
	uint64_t result=FNV64_INITIAL_OFFSET, i;
	for(i=0; i<len; i++)
	{
		result+=data[i];
		result=(result & UINT32_MAX) * MWC64_ALPHA + (result >> 32);
	}
	return(result);
}

uint64_t fnv1a_hash_64(const uint8_t *data, uint64_t len)
{
	uint64_t result=FNV64_INITIAL_OFFSET, i;
	for(i=0; i<len; i++)
	{
		result^=data[i];
		result*=FNV64_PRIME;
	}
	return(result);
}

static inline void xorshiftstar_proc(uint64_t &val)
{
	val^=val>>13;
	val^=val<<25;
	val^=val>>27;
	val*=XORSHIFT64_ALPHA;
}

uint64_t xorshiftstar_hash_64(const uint8_t *data, uint64_t len)
{
	uint64_t result=len+XORSHIFT64_INITIAL_OFFSET, i;
	for(i=0; i<len; i++)
	{
		result+=data[i];
		xorshiftstar_proc(result);
	}
	for(i=0; i<len; i++)
	{
		result+=data[i];
		xorshiftstar_proc(result);
	}
	return(result);
}
