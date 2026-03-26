/*
 * xorshift_hash.hpp
 *
 * Copyright 2025 Alexander Kart
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published
 * by the Free Software Foundation. You should have received a copy of the
 * GNU General Public License along with this program. If not, see
 * < https://www.gnu.org/licenses/gpl-3.0.txt >
 *
 * This program is distributed without any warranty.
 */

#ifndef XORSHIFT_HASH_HPP
#define XORSHIFT_HASH_HPP

#include <stdint.h>
#include <QByteArray>

uint64_t xorshiftstar_hash_64(const uint8_t *data, uint64_t len);
std::pair<uint64_t, uint64_t> xorshiftstar_hash_128(const uint8_t *data, uint64_t len);

quint64 xorshiftstar_hash_64(const QByteArray &data);
QPair<quint64, quint64> xorshiftstar_hash_128(const QByteArray &data);

#endif // XORSHIFT_HASH_HPP
