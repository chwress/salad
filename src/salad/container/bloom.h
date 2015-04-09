/*
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2015, Christian Wressnegger
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

#ifndef SALAD_CONTAINER_BLOOM_H_
#define SALAD_CONTAINER_BLOOM_H_

#include "bloom_ex.h"
#include "hash.h"


#define NUM_HASHFCTS 6
extern hashfunc_t HASH_FCTS[NUM_HASHFCTS];

#define HASHSET_SIMPLE (hashfunc_t[]) {sax_hash_n, sdbm_hash_n, bernstein_hash_n}, 3
#define HASHSET_MURMUR (hashfunc_t[]) {murmur_hash0_n, murmur_hash1_n, murmur_hash2_n}, 3

const int to_hashid(hashfunc_t h);

typedef enum { HASHES_UNDEFINED, HASHES_SIMPLE, HASHES_MURMUR } hashset_t;
#define VALID_HASHES "'simple' or 'murmur'"

const hashset_t to_hashset(const char* const str);
const char* const hashset_to_string(hashset_t hs);


#define DEFAULT_BFSIZE 24
#define DEFAULT_HASHSET HASHSET_SIMPLE

BLOOM* const bloom_init(const unsigned short size, const hashset_t hs);
BLOOM* const bloom_init_from_file(FILE* const f);

const int bloomfct_cmp(BLOOM* const bloom, ...);


#endif /* SALAD_CONTAINER_BLOOM_H_ */
