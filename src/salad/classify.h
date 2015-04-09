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

#ifndef SALAD_CLASSIFY_H_
#define SALAD_CLASSIFY_H_


#include "util.h"


typedef enum { BIT_NGRAM, BYTE_NGRAM, TOKEN_NGRAM } model_type_t;
const model_type_t to_model_type(const int as_binary, const int use_tokens);


typedef const double (*FN_CLASSIFIER)(bloom_param_t* const p, const char* const input, const size_t len);

const double anacheckb_ex (BLOOM* const bloom, const char* const input, const size_t len, const size_t n);
const double anacheckb_ex2(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n);

const double anacheck_ex (BLOOM* const bloom, const char* const input, const size_t len, const size_t n);
const double anacheck_ex2(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n);

const double anacheckw_ex (BLOOM* const bloom, const char* const input, const size_t len, const size_t n, const delimiter_array_t delim);
const double anacheckw_ex2(BLOOM* const bloom, BLOOM* const bbloom, const char* const input, const size_t len, const size_t n, const delimiter_array_t delim);


FN_CLASSIFIER pick_classifier(const model_type_t t, const int anomaly_detection);


#endif /* SALAD_CLASSIFY_H_ */
