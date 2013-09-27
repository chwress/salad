/**
 * Salad - A Content Anomaly Detector based on n-Grams
 * Copyright (c) 2012-2013, Christian Wressnegger
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

#include "io.h"
#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

#include "../config.h"

#ifdef IOTYPE_FILES
#include <sys/stat.h>
#endif

#ifdef USE_ARCHIVES
	#include <time.h>
	#include <archive.h>
	#include <archive_entry.h>
#endif

const char* const to_string(const io_mode_t m)
{
	switch (m)
	{
	case LINES:   return "lines";
	case FILES:   return "files";
#ifdef USE_ARCHIVES
	case ARCHIVE: return "archive";
#endif
	default:      return "unknown";
	}
}

const io_mode_t to_iomode(const char* const str)
{
	switch (cmp(str, "lines", "files", "archive", NULL))
	{
	case 0:  return LINES;
	case 1:  return FILES;
#ifdef USE_ARCHIVES
	case 2:  return ARCHIVE;
#endif
	// Okay that actually cannot happen ;)
	default: return FILES;
	}
}

const int is_valid_iomode(const char* const str)
{
#ifdef USE_ARCHIVES
	return (cmp(str, "lines", "files", "archive", NULL) > 0);
#else
	return (cmp(str, "lines", "files", NULL) > 0);
#endif
}

const int    file_open(file_t* const f, const char* const filename);
const size_t file_read(file_t* const f, data_t* data, const size_t numLines);
const int    file_close(file_t* const f);

data_processor_t dp_lines   = { .open = file_open, .read = file_read, .close = file_close };
data_processor_t dp_files   = { .open = NULL, .read = NULL, .close = NULL };


#ifdef USE_ARCHIVES
const int    archive_open(file_t* const f, const char* const filename);
const size_t archive_read(file_t* const f, data_t* data, const size_t numFiles);
const int    archive_close(file_t* const f);

data_processor_t dp_archive = { .open = archive_open, .read = archive_read, .close = archive_close };
#endif



const data_processor_t* const to_dataprocssor(const io_mode_t m)
{
	switch (m)
	{
	case LINES:   return &dp_lines;
	case FILES:   return &dp_files;
#ifdef USE_ARCHIVES
	case ARCHIVE: return &dp_archive;
#endif
	default:      return NULL;
	}
}

const int file_open(file_t* const f, const char* const filename)
{
	assert(f != NULL);

	f->fd = fopen(filename, "r");
	if (f->fd == NULL)
	{
		return EXIT_FAILURE;
	}
	f->data = NULL;
	return EXIT_SUCCESS;
}

#ifdef USE_ARCHIVES
const int archive_open(file_t* const f, const char* const filename)
{
	int ret = file_open(f, filename);
	if (ret != EXIT_SUCCESS)
	{
		return ret;
	}

	struct archive* const a = archive_read_new();
#ifdef LIBARCHIVE2
	archive_read_support_compression_all(a);
#else
	archive_read_support_filter_all(a);
#endif
	archive_read_support_format_all(a);

	ret = archive_read_open_FILE(a, f->fd);
	if (ret == ARCHIVE_OK)
	{
		f->data = a;
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}
#endif


const size_t file_read(file_t* const f, data_t* data, const size_t numLines)
{
	assert(f != NULL);
	assert(data != NULL);

	static char strip[256] = {0};
	strip[(int) '\n'] = 1;
	strip[(int) '\r'] = 1;

	char* line = NULL;
	size_t i = 0, len = 0;

	for (i = 0; i < numLines; i++)
	{
		ssize_t read = getline(&line, &len, f->fd);
		if (read <= -1) break;

		int j;
		for (j = read -1; j >= 0; j--)
		{
			if (!strip[(int) line[j]]) break;
		}
		line[j +1] = 0x00;

		size_t n = inline_decode(line, strlen(line));
#ifdef _GNU_SOURCE
		data[i].buf = strndup(line, n); // copies n +1
#else
		data[i].buf = (char*) malloc(n +1);
		memcpy(data[i].buf, line, n +1);
#endif
		data[i].len = n;
	}

	free(line);
	return i;
}

#ifdef USE_ARCHIVES
const size_t archive_read(file_t* const f, data_t* data, const size_t numFiles)
{
	assert(f != NULL);
	assert(data != NULL);

	struct archive* a = (struct archive*) f->data;
	struct archive_entry* entry = NULL;

	size_t i = 0;
	while (i < numFiles)
	{
		if (archive_read_next_header(a, &entry) != ARCHIVE_OK)
		{
			return i;
		}

        if (archive_entry_filetype(entry) == AE_IFREG)
        {
            data[i].len = (size_t) archive_entry_size(entry);
    		data[i].buf = (char*) malloc(data[i].len * sizeof(char));
            archive_read_data(a, data[i].buf, data[i].len);
            i++;
        }
        archive_read_data_skip(a);
	}

	return i;
}
#endif


const int file_close(file_t* const f)
{
	assert(f != NULL);
	if (fclose(f->fd) == 0)
	{
		f->fd = NULL;
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}

#ifdef USE_ARCHIVES
const int archive_close(file_t* const f)
{
	assert(f != NULL);
#ifdef LIBARCHIVE2
	if (archive_read_finish((struct archive*) f->data) == ARCHIVE_OK)
#else
	if (archive_read_free((struct archive*) f->data) == ARCHIVE_OK)
#endif
	{
		f->data = NULL;
		return EXIT_SUCCESS;
	}
	return EXIT_FAILURE;
}
#endif


#ifdef IOTYPE_FILES
// file io stuff
typedef struct
{
	char* path;
	DIR* dir;
	int num_files;
} dir_t;


void fix_dtype(char *path, struct dirent *dp);

int dir_open(dir_t* d, char* path)
{
	assert(d != NULL && path != NULL);

	/* Open directory */
	d->dir = opendir(path);
	if (!d->dir)
	{
		// error("Could not open directory '%s'", path);
		return -2;
	}

	struct dirent *dp;
	/* Count files */
	d->num_files = 0;
	while (d->dir && (dp = readdir(d->dir)) != NULL)
	{
		fix_dtype(path, dp);
		if (dp->d_type == DT_REG || dp->d_type == DT_LNK)
			d->num_files++;
	}
	rewinddir(d->dir);

	return 0;
}

int dir_read(dir_t* d, data_t* data, unsigned int len)
{
	assert(d != NULL && data != NULL && len > 0);
	unsigned int j = 0;

	/* Determine maximum path length and allocate buffer */
	int maxlen = fpathconf(dirfd(d->dir), _PC_NAME_MAX);

	/* Load block of files */
	for (unsigned int i = 0; i < len; i++)
	{
		struct dirent* buf;
		struct dirent* dp;
		buf = malloc(offsetof(struct dirent, d_name) + maxlen + 1);

		/* Read directory entry to local buffer */
		int r = readdir_r(d->dir, (struct dirent*) buf, &dp);
		if (r != 0 || !dp)
		{
			free(buf);
			return j;
		}

		/* Skip all entries except for regular files and symlinks */
		fix_dtype(d->path, dp);
		if (dp->d_type == DT_REG || dp->d_type == DT_LNK)
		{
			unsigned int l;
			data[j].buf = load_file(d->path, dp->d_name, &l);
			//data[j].src = strdup(dp->d_name);
			data[j].len = l;
			j++;
		}
		free(buf);
	}
	return j;
}

void dir_close(dir_t* d)
{
	assert(d != NULL);
	closedir(d->dir);
}


void fix_dtype(char *path, struct dirent *dp)
{
	struct stat st;
	char buffer[512];

	if (dp->d_type == DT_UNKNOWN)
	{
		snprintf(buffer, 512, "%s/%s", path, dp->d_name);
		stat(buffer, &st);
		if (S_ISREG(st.st_mode))
		{
			dp->d_type = DT_REG;
		}
		if (S_ISLNK(st.st_mode))
		{
			dp->d_type = DT_LNK;
		}
	}
}
#endif
