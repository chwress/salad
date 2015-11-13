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

#ifndef SALAD_CONTAINER_IO_BLOOM_H_
#define SALAD_CONTAINER_IO_BLOOM_H_

#include <util/config.h>
#include <util/io.h>

#include <container/bloom_ex.h>
#include <container/container.h>
#include <container/io/common.h>

#include <stdio.h>
#include <stdlib.h>

// WRITING
const BOOL fwrite_bloomconfig(const container_outputspec_t* const out, const BLOOM* const b);
const BOOL fwrite_bloomconfig_ex(FILE* const f, const BLOOM* const b);
const BOOL fwrite_bloomdata(const container_outputspec_t* const out, const BLOOM* const b, container_outputstate_t* const state);

const BOOL fwrite_bloom(FILE* const f, const BLOOM* const b);
const BOOL fwrite_bloom_ex(FILE* const f, const BLOOM* const b, const container_outputformat_t fmt);


// READING
const BOOL fread_bloomconfig(FILE* const f, const char* const key, const char* const value, void* const usr);
const BOOL fread_bloom(FILE* const f, BLOOM** b);
const BOOL fread_bloom_032(FILE* const f, BLOOM** b);


#endif /* SALAD_CONTAINER_IO_BLOOM_H_ */
