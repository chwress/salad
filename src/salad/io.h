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

#ifndef SALAD_IO_H_
#define SALAD_IO_H_

#include "util.h"
#include "salad.h"

#include <util/io.h>
#include <container/io.h>
#include <container/io/bloom.h>

#ifdef USE_ARCHIVES
#define SALAD_OUTPUTFMTS "'txt' or 'archive'"
#define DEFAULT_OUTPUTFMT SALAD_OUTPUTFMT_ARCHIVE
#else
#define SALAD_OUTPUTFMTS "'txt'"
#define DEFAULT_OUTPUTFMT SALAD_OUTPUTFMT_TXT
#endif


const char* const salad_outputfmt_to_string(const salad_outputfmt_t m);
const salad_outputfmt_t salad_to_outputfmt(const char* const str);
const BOOL salad_isvalid_outputfmt(const char* const str);

static const char* const CONFIG_HEADER;

// WRITING
const BOOL fwrite_modelconfig(const container_outputspec_t* const out, const salad_t* const s);
const BOOL fwrite_modelconfig_ex(FILE* const f, const salad_t* const s);
const BOOL fwrite_modeldata(const container_outputspec_t* const out, const salad_t* const s, container_outputstate_t* const state);

const BOOL fwrite_model(FILE* const f, const salad_t* const s);
const BOOL fwrite_model_txt(FILE* const f, const salad_t* const s);
const BOOL fwrite_model_zip(FILE* const f, const salad_t* const s);


// READING
const BOOL fread_modelconfig(FILE* const f, const char* const key, const char* const value, void* const usr);
const BOOL fread_model(FILE* const f, salad_t* const s);
const BOOL fread_model_txt(FILE* const f, salad_t* const s);
const BOOL fread_model_zip(FILE* const f, salad_t* const s);
const BOOL fread_model_032(FILE* const f, salad_t* const s);


#endif /* SALAD_CONTAINER_IO_H_ */
