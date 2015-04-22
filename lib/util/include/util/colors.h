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

#ifndef UTIL_COLORS_H_
#define UTIL_COLORS_H_

#ifdef WIN32
#undef USE_COLORS
#endif

#ifdef USE_COLORS
// TODO: Those are basically taken from test/ctest.h
// We might think about unifying those definitions
#define COLOR_G       "\033[0;32m"
#define COLOR_Y       "\033[0;33m"
#define COLOR_R       "\033[01;31m"
#define COLOR_X       "\033[0m"
#else
#define COLOR_G       ""
#define COLOR_Y       ""
#define COLOR_R       ""
#define COLOR_X       ""
#endif

#endif /* UTIL_COLORS_H_ */
