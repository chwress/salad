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
 *
 * Based on the implementation at http://en.literateprograms.org/Bloom_filter_(C)
 */

#ifndef SALAD_CONTAINER_BLOOM_EX_H__
#define SALAD_CONTAINER_BLOOM_EX_H__

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef unsigned int hash_t;

typedef hash_t (*hashfunc_t)(const char* const, const size_t n);

typedef struct {
	size_t bitsize; ///< The number of bit used by the bloom filter
	size_t size; ///< The number of bytes allocated to store the bloom filter
	unsigned char* a;

	uint8_t nfuncs;
	hashfunc_t* funcs;
} BLOOM;

BLOOM* const bloom_create(const size_t size, const uint8_t nfuncs, ...);
BLOOM* const vbloom_create(const size_t size, const uint8_t nfuncs, va_list args);
BLOOM* const bloom_create_ex(const size_t size, hashfunc_t* const funcs, const uint8_t nfuncs);
BLOOM* const bloom_create_from_file(FILE* const f, const uint8_t nfuncs, ...);
BLOOM* const vbloom_create_from_file(FILE* const f, const uint8_t nfuncs, va_list args);
BLOOM* const bloom_create_from_file_ex(FILE* const f, hashfunc_t* const funcs, const uint8_t nfuncs);

void bloom_clear(BLOOM* const bloom);
void bloom_destroy(BLOOM* const bloom);
void bloom_add_str(BLOOM* const bloom, const char *s, const size_t len);
void bloom_add_num(BLOOM* const bloom, const size_t num);
const int bloom_check_str(BLOOM* const bloom, const char *s, const size_t len);
const int bloom_check_num(BLOOM* const bloom, const size_t num);
const size_t bloom_count(BLOOM* const bloom);
const int bloom_compare(BLOOM* const a, BLOOM* const b);
void bloom_print(BLOOM* const bloom);
void bloom_print_ex(FILE* const f, BLOOM* const bloom);

const int bloom_to_file(BLOOM* const bloom, FILE* const f);

#endif /* SALAD_CONTAINER_BLOOM_EX_H__ */
