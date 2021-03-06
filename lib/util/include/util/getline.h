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

#ifndef UTIL_GETLINE_H_
#define UTIL_GETLINE_H_

#include <util/config.h>
#include <stdio.h>

#if (_XOPEN_SOURCE -0) < 700

#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>

ssize_t getdelim(char** buf, size_t* bufsiz, int delimiter, FILE* fp);
ssize_t getline(char** buf, size_t* bufsiz, FILE* fp);

#endif


char* const getlines(const char* const fname);
char* const getlines_ex(const char* const fname, const char* const prefix);

#endif /* UTIL_GETLINE_H_ */
