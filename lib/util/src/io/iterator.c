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

#include "iterator.h"

const size_t ITERATOR_READALL = SIZE_MAX;


void init_iterator_context(iterator_context_t* const c, file_t* const f)
{
#ifdef GROUPED_INPUT
	c->fid = 0;
	c->gid = 0;
#endif
	c->i = 0;
}
