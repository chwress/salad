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

#ifndef SALAD_ANAGRAM_H_
#define SALAD_ANAGRAM_H_

#include "common.h"
#include "bloom.h"
#include "hash.h"

#include <util/vec.h>

#include <stdint.h>


#define NUM_HASHFCTS 6
extern hashfunc_t HASH_FCTS[NUM_HASHFCTS];

#define HASHSET_SIMPLE (hashfunc_t[]) {sax_hash_n, sdbm_hash_n, bernstein_hash_n}, 3
#define HASHSET_MURMUR (hashfunc_t[]) {murmur_hash0_n, murmur_hash1_n, murmur_hash2_n}, 3

const int to_hashid(hashfunc_t h);

typedef enum { HASHES_UNDEFINED, HASHES_SIMPLE, HASHES_MURMUR } hashset_t;
#define VALID_HASHES "'simple' or 'murmur'"

const hashset_t to_hashset(const char* const str);
const char* const hashset_to_string(hashset_t hs);


typedef struct {
	BLOOM* const bloom1; // e.g. good content filter
	BLOOM* const bloom2; // e.g. bad content filter
	const size_t n;      // n-gram length
	const delimiter_array_t delim;
} bloom_param_t;

typedef struct
{
	size_t new;
	size_t uniq;
	size_t total;
} bloomize_stats_t;

typedef void (*FN_BLOOMIZE)(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out);

void bloomize_ex(BLOOM* const bloom, const char* const str, const size_t len, const size_t n);
void bloomize_ex2(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const vec_t* const weights);
void bloomize_ex3(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out);
void bloomize_ex4(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out);
void bloomizew_ex(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim);
void bloomizew_ex2(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, const vec_t* const weights);
void bloomizew_ex3(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, bloomize_stats_t* const out);
void bloomizew_ex4(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, bloomize_stats_t* const out);

typedef const double (*FN_ANACHECK)(bloom_param_t* const p, const char* const input, const size_t len);

const double anacheck_ex(BLOOM* const bloom, const char* const input, const size_t len, const size_t n);
const double anacheck_ex2(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n);
const double anacheckw_ex(BLOOM* const bloom, const char* const input, const size_t len, const size_t n, const delimiter_array_t delim);
const double anacheckw_ex2(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n, const delimiter_array_t delim);

BLOOM* const bloomize(const char* str, const size_t len, const size_t n);
BLOOM* const bloomizew(const char* str, const size_t len, const size_t n, const delimiter_array_t delim);

#define DEFAULT_BFSIZE 24
#define DEFAULT_HASHSET HASHSET_SIMPLE

BLOOM* const bloom_init(const unsigned short size, const hashset_t hs);
BLOOM* const bloom_init_from_file(FILE* const f);
const int bloomfct_cmp(BLOOM* const bloom, ...);

const int fwrite_hashspec(FILE* const f, BLOOM* const bloom);
const int fwrite_model(FILE* const f, BLOOM* const bloom, const size_t ngramLength, const char* const delimiter);

const int fread_hashspec(FILE* const f, hashfunc_t** const hashfuncs, uint8_t* const nfuncs);
const int fread_model(FILE* const f, BLOOM** const bloom, size_t* const ngramLength, delimiter_array_t delim, int* const useWGrams);

#endif /* SALAD_ANAGRAM_H_ */
