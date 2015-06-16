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

#ifndef INTERNAL_UTIL_IO_REGEX_H_
#define INTERNAL_UTIL_IO_REGEX_H_

#include <util/config.h>

#ifdef USE_REGEX_FILTER
#include <regex.h>
// XXX: This heavily relies on implementation details :/
#define REGEX_ISVALID(x) (x.__REPB_PREFIX(buffer) != NULL)
#endif

#endif /* INTERNAL_UTIL_IO_REGEX_H_ */
