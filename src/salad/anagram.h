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

#ifndef SALAD_ANAGRAM_H_
#define SALAD_ANAGRAM_H_

#include "common.h"
#include "util.h"

#include <container/bloom.h>
#include <util/vec.h>

#include <stdint.h>


BLOOM* const bloomize(const char* str, const size_t len, const size_t n);
BLOOM* const bloomizew(const char* str, const size_t len, const size_t n, const delimiter_array_t delim);

typedef struct
{
	size_t new;
	size_t uniq;
	size_t total;
} bloomize_stats_t;

typedef void (*FN_BLOOMIZE)(bloom_param_t* const p, const char* const str, const size_t len, bloomize_stats_t* const out);

/**
 * Extract bit n-grams from the specified input string in order
 * to populate the given bloom filter.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 */
void bloomizeb_ex (BLOOM* const bloom, const char* const str, const size_t len, const size_t n);
/**
 * Extract bit n-grams from the specified input string in order
 * to populate the given bloom filter. However, only n-grams with
 * positive weight are added.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 * @param[in] weights Weight values for particular n-grams.
 */
void bloomizeb_ex2(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const vec_t* const weights);
/**
 * Extract bit n-grams from the specified input string in order
 * to populate the first bloom filter. The second bloom filter
 * is cleared and used to track the 'uniqueness' of n-grams
 * within the given string.
 *
 * After checking the first filter, new n-grams are added and
 * counted as 'new' in the statistics. The first bloom filter
 * is updated with each new n-gram, hence new n-grams are counted
 * exactly once.
 *
 * Additionally, the total number of n-grams is recorded.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 * @param[out] out The statistical data collected during execution.
 */
void bloomizeb_ex3(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out);
/**
 * Extract bit n-grams from the specified input string in order
 * to populate the first bloom filter. The second bloom filter
 * is cleared and used to track the 'uniqueness' of n-grams
 * within the given string.
 *
 * After checking the first filter, new n-grams are added and
 * counted as 'new' in the statistics. The first bloom filter
 * is updated with each new n-gram, hence new n-grams are counted
 * exactly once.
 *
 * Additionally, the total number of n-grams is recorded.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 * @param[out] out The statistical data collected during execution.
 */
void bloomizeb_ex4(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out);

/**
 * Extract byte n-grams from the specified input string in order
 * to populate the given bloom filter.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 */
void bloomize_ex (BLOOM* const bloom, const char* const str, const size_t len, const size_t n);
/**
 * Extract byte n-grams from the specified input string in order
 * to populate the given bloom filter. However, only n-grams with
 * positive weight are added.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 * @param[in] weights Weight values for particular n-grams.
 */
void bloomize_ex2(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const vec_t* const weights);
/**
 * Extract byte n-grams from the specified input string in order
 * to populate the first bloom filter. The second bloom filter
 * is cleared and used to track the 'uniqueness' of n-grams
 * within the given string.
 *
 * After checking the first filter, new n-grams are added and
 * counted as 'new' in the statistics. The first bloom filter
 * is updated with each new n-gram, hence new n-grams are counted
 * exactly once.
 *
 * Additionally, the total number of n-grams is recorded.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 * @param[out] out The statistical data collected during execution.
 */
void bloomize_ex3(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out);
/**
 * Extract byte n-grams from the specified input string in order
 * to populate the first bloom filter. The second bloom filter
 * is cleared and used to track the 'uniqueness' of n-grams
 * within the given string.
 *
 * After checking the first filter, new n-grams are added and
 * counted as 'new' in the statistics. The first bloom filter
 * is updated with each new n-gram, hence new n-grams are counted
 * exactly once.
 *
 * Additionally, the total number of n-grams is recorded.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 * @param[out] out The statistical data collected during execution.
 */
void bloomize_ex4(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, bloomize_stats_t* const out);

/**
 * Extract n-grams based on byte tokens from the specified input
 * string in order to populate the given bloom filter.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 * @param[in] delim The delimiter to use for splitting the input
 *                  in tokens.
 */
void bloomizew_ex (BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim);
/**
 * Extract n-grams based on byte tokens from the specified input
 * string in order to populate the given bloom filter. However,
 * only n-grams with positive weight are added.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 * @param[in] delim The delimiter to use for splitting the input
 *                  in tokens.
 * @param[in] weights Weight values for particular n-grams.
 */
void bloomizew_ex2(BLOOM* const bloom, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, const vec_t* const weights);
/**
 * Extract n-grams based on byte tokens from the specified input
 * string in order to populate the first bloom filter. The second
 * bloom filter is cleared and used to track the 'uniqueness' of
 * n-grams within the given string.
 *
 * After checking the first filter, new n-grams are added and
 * counted as 'new' in the statistics. The first bloom filter
 * is updated with each new n-gram, hence new n-grams are counted
 * exactly once.
 *
 * Additionally, the total number of n-grams is recorded.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 * @param[in] delim The delimiter to use for splitting the input
 *                  in tokens.
 * @param[out] out The statistical data collected during execution.
 */
void bloomizew_ex3(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, bloomize_stats_t* const out);
/**
 * Extract n-grams based on byte tokens from the specified input
 * string in order to populate the first bloom filter. The second
 * bloom filter is cleared and used to track the 'uniqueness' of
 * n-grams within the given string.
 *
 * After checking the first filter, new n-grams are added and
 * counted as 'new' in the statistics. The first bloom filter
 * is updated with each new n-gram, hence new n-grams are counted
 * exactly once.
 *
 * Additionally, the total number of n-grams is recorded.
 *
 * @param[inout] bloom The bloom filter to be populated.
 * @param[in] str The string to analyze.
 * @param[in] len The length of the string to analyze.
 * @param[in] n The n-gram length to use.
 * @param[out] out The statistical data collected during execution.
 */
void bloomizew_ex4(BLOOM* const bloom1, BLOOM* const bloom2, const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, bloomize_stats_t* const out);




// macros for the generic use of the bloomize functions
#include "anagram_ex.h"


#endif /* SALAD_ANAGRAM_H_ */
