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

#ifndef INTERNAL_UTIL_IO_ITERATOR_H_
#define INTERNAL_UTIL_IO_ITERATOR_H_

#include <util/config.h>
#include <util/io.h>

// Iterators
extern const size_t ITERATOR_READALL;

typedef struct {
#ifdef USE_REGEX_FILTER
	regmatch_t m[1];
#endif
#ifdef GROUPED_INPUT
	int fid, gid;
#endif
	size_t i;
} iterator_context_t;


void init_iterator_context(iterator_context_t* const c, file_t* const f);


#endif /* INTERNAL_UTIL_IO_ITERATOR_H_ */
