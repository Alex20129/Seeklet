/*
 * xorshift_hash.cpp
 *
 * Copyright 2025 Alexander Kart
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation.You should have received a copy of the
 * GNU General Public License along with this program. If not, see
 * < https://www.gnu.org/licenses/gpl-3.0.txt >
 *
 * This program is distributed without any warranty;
 */

#include "xorshift_hash.hpp"

static constexpr uint64_t XORSHIFT64_ALPHA=0x2545F4914F6CDD1D;
static constexpr uint64_t XORSHIFT64_INITIAL_OFFSET=0x7A643C25D6EDAD19;

static constexpr uint64_t XORSHIFT128_INITIAL_OFFSET_A=XORSHIFT64_INITIAL_OFFSET;
static constexpr uint64_t XORSHIFT128_INITIAL_OFFSET_B=0x5B210293948B3912;

static inline void xorshiftstar_proc(uint64_t &val)
{
	val^=val>>25;
	val^=val<<13;
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
	return result;
}

void xorshiftstar_hash_128(const uint8_t *data, uint64_t len, uint64_t *result)
{
	uint64_t result_a=len+XORSHIFT128_INITIAL_OFFSET_A;
	uint64_t result_b=len+XORSHIFT128_INITIAL_OFFSET_B;
	uint64_t mix, i;
	for(i=0; i<len; i++)
	{
		result_a += data[i];
		result_b += data[i];
		mix=result_a ^ result_b;
		xorshiftstar_proc(result_a);
		xorshiftstar_proc(result_b);
		result_a ^= mix;
		result_b ^= mix;
	}
	for(i=0; i<len; i++)
	{
		result_a += data[i];
		result_b += data[i];
		mix=result_a ^ result_b;
		xorshiftstar_proc(result_a);
		xorshiftstar_proc(result_b);
		result_a ^= mix;
		result_b ^= mix;
	}
	result[0] = result_a;
	result[1] = result_b;
}
