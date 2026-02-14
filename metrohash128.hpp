// metrohash128.hpp
//
// Copyright 2015-2018 J. Andrew Rogers
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef METROHASH_128_HPP
#define METROHASH_128_HPP

#include <stdint.h>

class MetroHash128
{
	static const uint64_t k0 = 0xC83A91E1;
	static const uint64_t k1 = 0x8648DBDB;
	static const uint64_t k2 = 0x7BDEC03B;
	static const uint64_t k3 = 0x2F5870A5;
	struct { uint64_t v[4]; } state;
	struct { uint8_t b[32]; } input;
	uint64_t bytes;
public:
	static const uint32_t bits = 128;
	MetroHash128(const uint64_t seed = 0);
	void Initialize(const uint64_t seed = 0);
	void Update(const uint8_t *buffer, const uint64_t length);
	void Finalize(uint8_t *const hash);
	static void Hash(const uint8_t *buffer, const uint64_t length, uint8_t *hash, uint64_t seed=0);
};

void metrohash128_1(const uint8_t *data, uint64_t len, uint64_t seed, uint64_t *hash);
void metrohash128_2(const uint8_t *data, uint64_t len, uint64_t seed, uint64_t *hash);

#endif // METROHASH_128_HPP
