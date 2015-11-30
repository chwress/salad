/*
 * libutil - Yet Another Utility Library
 * Copyright (c) 2012-2015, Christian Wressnegger
 * --
 * This file is part of the library libutil.
 *
 * libutil is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libutil is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/**
 * @file
 */

#ifndef UTIL_FORMAT_H_
#define UTIL_FORMAT_H_

#include <stdlib.h>

extern const char* SYMBOLS_CUSTOMARY[];
extern const char* SYMBOLS_CUSTOMARY_EXT[];
extern const char* SYMBOLS_IEC[];
extern const char* SYMBOLS_IEC_EX[];

typedef enum {
	SYM_CUSTOMARY=0,
	SYM_CUSTOMARY_EXT,
	SYM_IEC,
	SYM_IEC_EXT,
	SYM_NUMVARIANTS
} symbol_variants_t;

extern const char* const* SYMBOLS[];


const char* const bytes2human(char* const buf, const size_t n, const size_t x);
const char* const bytes2human_ex(char* const buf, const size_t n, const size_t x, const symbol_variants_t variant);
const size_t human2bytes(const char* const x);

#endif /* UTIL_FORMAT_H_ */
