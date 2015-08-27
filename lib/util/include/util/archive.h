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

#ifndef UTIL_ARCHIVE_H_
#define UTIL_ARCHIVE_H_

#include <stdlib.h>
#include <stdio.h>
#include <util/config.h>

#ifdef USE_ARCHIVES

#include <archive.h>

typedef enum {ZIP, TARGZ} archive_type;

struct archive* const archive_read_easyopen(FILE* const f);
struct archive* const archive_write_easyopen(FILE* const f, const archive_type t);

const int archive_read_seek(struct archive* const a, struct archive_entry** entry, const char* const name);
const int archive_read_dumpfile(struct archive* const a, struct archive_entry* const entry);
const int archive_read_dumpfile2(FILE* const f, const char* const name, const char* const filename);
const int archive_write_file(struct archive* const a, const char* const filename, const char* const name);

void archive_read_easyclose(struct archive* const a);
void archive_write_easyclose(struct archive* const a);


#endif

#endif /* UTIL_ARCHIVE_H_ */
