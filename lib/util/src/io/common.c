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


#include "common.h"
#include "../regex.h"

const int all_filter(file_t* const f, const char* const pattern)
{
#ifdef USE_REGEX_FILTER
	if (REGEX_ISVALID(f->filter)) {
		regfree(&f->filter);
	}
#endif
	return all_filter_ex(f, pattern);
}

const int all_filter_ex(file_t* const f, const char* const pattern)
{
#ifdef USE_REGEX_FILTER
    if (regcomp(&f->filter, pattern, REG_EXTENDED) != 0) {
        return EXIT_FAILURE;
    }
#endif
	return EXIT_SUCCESS;
}

const int all_filter_close(file_t* const f)
{
#ifdef USE_REGEX_FILTER
	regfree(&f->filter);
#endif
	return EXIT_SUCCESS;
}
