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

#include "ngrams.h"

// bit n-grams
extern inline void extract_bitgrams(const char* const str, const size_t len, const size_t n, FN_PROCESS_NGRAM fct, void* const data);
extern inline void extract_bgrams(const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, FN_PROCESS_NGRAM fct, void* const data);

// byte or character n-grams
extern inline void extract_bytegrams(const char* const str, const size_t len, const size_t n, FN_PROCESS_NGRAM const fct, void* const data);
extern inline void extract_ngrams(const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, FN_PROCESS_NGRAM const fct, void* const data);

// token or word n-grams
extern inline const char pick_delimiterchar(const delimiter_array_t delim);
extern inline char* const uniquify(const char** const str, size_t* const len, const delimiter_array_t delim, const char ch);
extern inline void extract_wgrams(const char* const str, const size_t len, const size_t n, const delimiter_array_t delim, FN_PROCESS_NGRAM fct, void* const data);
