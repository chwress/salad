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

#ifndef UTIL_SIMPLE_CONF_H_
#define UTIL_SIMPLE_CONF_H_

#include <util/util.h>

#include <stdio.h>
#include <stdlib.h>

typedef const BOOL(*FN_KEYVALUE)(FILE* const f, const char* const key, const char* const value, void* const usr);

const size_t fread_config(FILE* const f, const char* const header, FN_KEYVALUE handler, void* const usr);

#endif /* UTIL_SIMPLE_CONF_H_ */
