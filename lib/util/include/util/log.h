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

#ifndef UTIL_LOG_H_
#define UTIL_LOG_H_

#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>

typedef enum {STATUS = 0, INFO, WARNING, ERROR, FATAL_ERROR} log_t;

void print_ex(const log_t l, const char* const color, const char* const msg, ...);
void vprint_ex(const log_t l, const char* const color, const char* const msg, va_list args);

void print(const char* const msg, ...);
void status(const char* const msg, ...);
void info(const char* const msg, ...);
void warn(const char* const msg, ...);
void error(const char* const msg, ...);
void fatal(const char* const msg, ...);

void progress(const size_t cur, const size_t total);
void hourglass(uint8_t* const state, const size_t c);
void hourglass_ex(uint8_t* const state);
void hourglass_stop();

const int bye(const int ec);
const int bye_ex(const int ec, const char* const msg);

#endif /* UTIL_LOG_H_ */
