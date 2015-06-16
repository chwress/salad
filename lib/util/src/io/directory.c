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

#include "common.h"

#ifdef IOTYPE_FILES
#include <sys/stat.h>


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
