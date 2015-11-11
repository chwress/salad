/*
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2015, Christian Wressnegger
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

/**
 * @file
 */

#ifndef SALAD_CONTAINER_IO_H_
#define SALAD_CONTAINER_IO_H_

#include "container.h"
#include "container/io/common.h"

#include <util/io.h>
#include <stdio.h>

// WRITING
const int fwrite_containerconfig(const container_outputspec_t* const out, const container_t* const c);
const int fwrite_containerconfig_ex(FILE* const f, const container_t* const c);
const int fwrite_containerdata(const container_outputspec_t* const out, const container_t* c, container_outputstate_t* const state);

const int fwrite_container(FILE* const f, const container_t* const c);
const int fwrite_container_ex(FILE* const f, const container_t* const c, const container_outputformat_t fmt);


// READING
const int fread_containerconfig(FILE* const f, const char* const key, const char* const value, void* const usr);
const int fread_container(FILE* const f, container_t* const c);


#endif /* SALAD_CONTAINER_IO_H_ */
