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

#include "util/archive.h"

#ifdef USE_ARCHIVES
#include <archive_entry.h>
#include <string.h>

struct archive* const archive_read_easyopen(FILE* const f)
{
	struct archive* a = archive_read_new();
#ifdef LIBARCHIVE2
	archive_read_support_compression_all(a);
#else
	archive_read_support_filter_all(a);
#endif
	archive_read_support_format_all(a);

	const int ret = archive_read_open_FILE(a, f);
	return (ret == ARCHIVE_OK ? a : NULL);
}

struct archive* const archive_write_easyopen(FILE* const f, const archive_type t)
{
	struct archive* a = archive_write_new();
	switch (t)
	{
	case ZIP:
		archive_write_add_filter_none(a);
		archive_write_set_format_zip(a);
		break;

	case TARGZ:
		archive_write_add_filter_gzip(a);
		archive_write_set_format_pax_restricted(a);
		break;
	}

	const int ret = archive_write_open_FILE(a, f);
	return (ret == ARCHIVE_OK ? a : NULL);
}

const int archive_read_seek(struct archive* const a, struct archive_entry** entry, const char* const needle)
{
	int r = ARCHIVE_FAILED;

	while ((r = archive_read_next_header(a, entry)) == ARCHIVE_OK)
	{
		const char* const name = archive_entry_pathname(*entry);
		if (strcmp(name, needle) == 0)
		{
			return ARCHIVE_OK;
		}
	}
	return ARCHIVE_EOF;
}

const int archive_read_dumpfile(struct archive* const a, struct archive_entry* const entry)
{
	struct archive* const out = archive_write_disk_new();
	// archive_write_disk_set_options(out, flags);
	archive_write_disk_set_standard_lookup(out);
	archive_write_header(out, entry);

	const void *buf;
	size_t size;
	off_t offset;

	while (1)
	{
		int ret = archive_read_data_block(a, &buf, &size, &offset);
		if (ret == ARCHIVE_EOF) return ARCHIVE_OK;

		if (ret != ARCHIVE_OK) return ret;

		ret = archive_write_data_block(out, buf, size, offset);
		if (ret != ARCHIVE_OK) return ret;
	}
}

const int archive_read_dumpfile2(FILE* const f, const char* const name, const char* const filename)
{
	struct archive* a = archive_read_easyopen(f);
	if (a == NULL)
	{
		return ARCHIVE_FAILED;
	}

	struct archive_entry* entry;
	int ret = archive_read_seek(a, &entry, name);
	if (ret != ARCHIVE_OK)
	{
		archive_read_easyclose(a);
		return ret;
	}

	archive_entry_set_pathname(entry, filename);
	ret = archive_read_dumpfile(a, entry);

	archive_read_easyclose(a);
	return ret;
}

const int archive_write_file(struct archive* const a, const char* const filename, const char* const name)
{
	FILE* f = fopen(filename, "r+");
	if (f == NULL) return ARCHIVE_FAILED;

	fseek(f, 0, SEEK_END);
	const size_t n = ftell(f);
	fseek(f, 0, SEEK_SET);

	struct archive_entry* entry = archive_entry_new();
	archive_entry_clear(entry);
	archive_entry_set_pathname(entry, name);
	archive_entry_set_size(entry, n);
	archive_entry_set_filetype(entry, AE_IFREG);
	archive_entry_set_perm(entry, 0644);
	archive_write_header(a, entry);

	static const int BUFFER_SIZE = 0x1000;
	char buf[BUFFER_SIZE];

	size_t nread = 0;
	do
	{
		nread = fread(buf, 1, BUFFER_SIZE, f);
		const size_t nwrote = archive_write_data(a, buf, nread);
		if (nread != nwrote)
		{
			archive_entry_free(entry);
			return ARCHIVE_FAILED;
		}
	} while (nread == BUFFER_SIZE);

	fclose(f);
	archive_entry_free(entry);
	return ARCHIVE_OK;
}

void archive_read_easyclose(struct archive* const a)
{

	archive_read_close(a);
#ifdef LIBARCHIVE2
	archive_read_finish(a);
#else
	archive_read_free(a);
#endif
}

void archive_write_easyclose(struct archive* const a)
{
	archive_write_close(a);
#ifdef LIBARCHIVE2
	archive_write_finish(a);
#else
	archive_write_free(a);
#endif
}

#endif
