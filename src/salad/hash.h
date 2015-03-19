/*
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2014, Christian Wressnegger
 * --
 * This file is part of Letter Salad or Salad for short.
 *
 * Salad is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Salad is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/**
 * @file
 */

#ifndef SALAD_HASH_H__
#define SALAD_HASH_H__

#include <stdio.h>
#include <stdint.h>


uint32_t sax_hash(const char* const key);
uint32_t sax_hash_n(const char* const key, const size_t len);

uint32_t sdbm_hash(const char* const key);
uint32_t sdbm_hash_n(const char* const key, const size_t len);

uint32_t bernstein_hash(const char* const key);
uint32_t bernstein_hash_n(const char* const key, const size_t len);


uint32_t murmur_hash0(const char* const key);
uint32_t murmur_hash0_n(const char* const key, const size_t len);

uint32_t murmur_hash1(const char* const key);
uint32_t murmur_hash1_n(const char* const key, const size_t len);

uint32_t murmur_hash2(const char* const key);
uint32_t murmur_hash2_n(const char* const key, const size_t len);


#endif /* SALAD_HASH_H_ */
