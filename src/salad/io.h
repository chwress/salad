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

#ifndef SALAD_IO_H_
#define SALAD_IO_H_

#include "common.h"

#include <container/bloom.h>

const int fwrite_model(FILE* const f, BLOOM* const bloom, const size_t ngram_length, const char* const delimiter, const int as_binary);
const int fread_model(FILE* const f, BLOOM** const bloom, size_t* const ngram_length, delimiter_array_t delim, int* const use_wgrams, int* const as_binary);
const int fread_model_txt(FILE* const f, BLOOM** const bloom, size_t* const ngram_length, delimiter_array_t delim, int* const use_wgrams, int* const as_binary);
const int fread_model_032(FILE* const f, BLOOM** const bloom, size_t* const ngram_length, delimiter_array_t delim, int* const use_wgrams);


#endif /* SALAD_CONTAINER_IO_H_ */
