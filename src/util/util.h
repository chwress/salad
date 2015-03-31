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

#ifndef UTIL_UTIL_H_
#define UTIL_UTIL_H_

#include <config.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifdef IOTYPE_FILES
#include <dirent.h>
#endif


const int cmp(const char* const s, ...);
const size_t inline_decode(char* s, const size_t len);
const int starts_with(const char* const s, const char* const prefix);
const size_t count_char(const char* const s, const char ch);
char* const join(const char* const sep, const char** strs);
char* const join_ex(const char* const prefix, const char* const sep, const char** strs, const char* const fmt);

#ifdef IOTYPE_FILES
char* load_file(char* path, char* name, unsigned int* size);
#endif

char* const fread_str(FILE* const f);
const float frand();

#if __STDC_VERSION__ >= 199901L
#include <tgmath.h>
#define POW pow
#define MIN fmin
#define MAX fmax
#else
#include <math.h>
#define POW (size_t) pow
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) < (Y) ? (Y) : (X))
#endif

#if (_XOPEN_SOURCE -0) >= 700
	#define STRDUP(from, to)                               \
			{	                                           \
				to = strdup(from);                         \
			}

	#define STRNDUP(n, from, to)                           \
			{	                                           \
				to = strndup(from, n);                     \
			}  // copies n +1

	#define STRNLEN(s, n) strnlen(s, n)
#else
	#define STRDUP(from, to)                               \
			{	                                           \
				const char* const _from_ = from;           \
				char** _to_ = &(to);                       \
				                                           \
				*_to_ = (char*) malloc(strlen(_from_) +1); \
				strcpy(*_to_, _from_);                     \
			}

	#define STRNDUP(n, from, to)                           \
			{	                                           \
				const size_t _n_ = n;                      \
				const char* const _from_ = from;           \
				char** _to_ = &(to);                       \
				                                           \
				*_to_ = (char*) malloc(_n_ +1);            \
				strncpy(*_to_, _from_, _n_);               \
				(*_to_)[_n_] = 0x00;                       \
			}

	// simply ignore the max-length parameter :/
	#define STRNLEN(s, n) strlen(s)
#endif

#define FALSE 0
#define TRUE  1

#ifdef HAS_Z_MODIFIER
	#define Z "zu"
	#define SIZE_T size_t
#else
	#define Z "lu"
	#define SIZE_T unsigned long
#endif

#endif /* UTIL_UTIL_H_ */
