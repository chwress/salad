/**
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2014, Christian Wressnegger
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


#ifndef LOG_H_
#define LOG_H_

#include <stdarg.h>

typedef enum {STATUS = 0, INFO, WARNING, ERROR, FATAL_ERROR} log_t;

void print_ex(const log_t l, const char* const msg, va_list args);

void print(const char* const msg, ...);
void status(const char* const msg, ...);
void info(const char* const msg, ...);
void warn(const char* const msg, ...);
void error(const char* const msg, ...);
void fatal(const char* const msg, ...);


#endif /* LOG_H_ */
