/**
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2013, Christian Wressnegger
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

#ifndef UTIL_H_
#define UTIL_H_

#include "getline.h"

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#ifdef IOTYPE_FILES
#include <dirent.h>
#endif

#include "io.h"

const int cmp(const char* const s, ...);
const size_t inline_decode(char* s, const size_t len);

#ifdef IOTYPE_FILES
char* load_file(char* path, char* name, unsigned int* size);
#endif

char* const fread_str(FILE* const f);
const float frand();

#endif /* UTIL_H_ */
