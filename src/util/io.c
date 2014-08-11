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

#include "io.h"
#include "util.h"
#include "log.h"

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

#ifdef USE_NETWORK
	#include <nids.h>
	#include <errno.h>
	#include <signal.h>

	struct
	{
		FN_DATA callback;
		void* data;
		int merge_payloads;
		size_t num_chunks;
	} nids_user;

#endif

const char* const to_string(const io_mode_t m)
{
	switch (m)
	{
	case LINES:        return "lines";
	case FILES:        return "files";
#ifdef USE_ARCHIVES
	case ARCHIVE:      return "archive";
#endif
#ifdef USE_NETWORK
	case NETWORK:      return "network";
	case NETWORK_DUMP: return "network-dump";
#endif
	default:           return "unknown";
	}
}

const io_mode_t to_iomode(const char* const str)
{
	switch (cmp(str, "lines", "files", "archive", "network", "network-dump", NULL))
	{
	case 0:  return LINES;
	case 1:  return FILES;
#ifdef USE_ARCHIVES
	case 2:  return ARCHIVE;
#endif
#ifdef USE_NETWORK
	case 3:  return NETWORK;
	case 4:  return NETWORK_DUMP;
#endif
	// Okay that actually cannot happen ;)
	default: return FILES;
	}
}

const int is_valid_iomode(const char* const str)
{
	switch (cmp(str, "lines", "files", "archive", "network", "network-dump", NULL))
	{
#ifdef USE_NETWORK
	case 4:
	case 3:
#endif
#ifdef USE_ARCHIVES
	case 2:
#endif
	case 1:
	case 0:  return TRUE;
	default: return FALSE;
	}
}

void data_free(data_t* const d)
{
	assert(d != NULL);
	free(d->buf);
}

void data_destroy(data_t* const d)
{
	assert(d != NULL);
	free(d->buf);
	free(d);
}

const int    file_open(file_t* const f, const char* const filename, void *const p);
const size_t file_read(file_t* const f, data_t* data, const size_t numLines);
const size_t file_recv(file_t* const f, FN_DATA callback, void* const usr);
const int    file_close(file_t* const f);

data_processor_t dp_lines   = { .open = file_open, .read = file_read, .recv = file_recv, .close = file_close };
data_processor_t dp_files   = { .open = NULL, .read = NULL, .recv = NULL, .close = NULL };


#ifdef USE_ARCHIVES
const int    archive_open(file_t* const f, const char* const filename, void *const p);
const size_t archive_read(file_t* const f, data_t* data, const size_t numFiles);
const size_t archive_recv(file_t* const f, FN_DATA callback, void* const usr);
const int    archive_close(file_t* const f);

data_processor_t dp_archive = { .open = archive_open, .read = archive_read, .recv = archive_recv, .close = archive_close };
#endif


#ifdef USE_NETWORK
const int    net_open(file_t* const f, const char* const filename, void *const p);
const size_t net_read(file_t* const f, data_t* data, const size_t numFiles);
const size_t net_recv(file_t* const f, FN_DATA callback, void* const usr);
const int    net_close(file_t* const f);

data_processor_t dp_network = { .open = net_open, .read = NULL, .recv = net_recv, .close = net_close };
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
#ifdef USE_NETWORK
	case NETWORK_DUMP:
	case NETWORK: return &dp_network;
#endif
	default:      return NULL;
	}
}

const int file_open(file_t* const f, const char* const filename, void *const p)
{
	assert(f != NULL);

	f->fd = fopen(filename, "rb");
	if (f->fd == NULL)
	{
		return EXIT_FAILURE;
	}
	f->data = NULL;
	return EXIT_SUCCESS;
}

#ifdef USE_ARCHIVES
const int archive_open(file_t* const f, const char* const filename, void *const p)
{
	int ret = file_open(f, filename, p);
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

#ifdef USE_NETWORK
// This is a stripped down version of libnids initialization functionality
// for opening files and devices.
pcap_t* const nids_open()
{
	// nids.c#nids_init
	if (nids_params.filename)
	{
		return pcap_open_offline(nids_params.filename, nids_errbuf);
	}

	// nids.c#open_live
	int promisc = 0;

	if (nids_params.device == NULL)
	{
	   nids_params.device = pcap_lookupdev(nids_errbuf);
	}

	if (nids_params.device == NULL) return NULL;

	// This simplified version does not support the "all" option
	// if (strcmp(nids_params.device, "all") == 0) { ... }

	promisc = (nids_params.promisc != 0);
	return pcap_open_live(nids_params.device, 16384, promisc, nids_params.pcap_timeout, nids_errbuf);
}

void sigfunc(int sig)
{
	pcap_breakloop(nids_params.pcap_desc);
	print("");
}

const int net_open(file_t* const f, const char* const filename, void *const p)
{
	assert(f != NULL);

	net_param_t default_params = { FALSE, NULL };
	net_param_t* const params = (p != NULL ? (net_param_t*) p : &default_params);

	// Install signal handlers
	signal(SIGINT,  sigfunc);
	signal(SIGTERM, sigfunc);
	signal(SIGABRT, sigfunc);

	// Initialize libnids
	nids_params.n_tcp_streams = 4096;   // Streams to track for re-assembly
	nids_params.n_hosts = 1024;         // Hosts to track for defragmentation
	nids_params.scan_num_hosts = 0;     // Disable port scan detection

	nids_params.device = NULL;
	nids_params.filename = (params->is_device ? NULL : (char*) filename);
	nids_params.device = (params->is_device ? (char*) filename : NULL);
	nids_params.pcap_filter = (char*) params->pcap_filter;

	f->fd = NULL;
	f->data = &nids_params;

	nids_params.pcap_desc = nids_open();

	if (!nids_init())
	{
		params->error_msg = nids_errbuf;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
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

const size_t recv_stub(file_t* const f, FN_READ read, FN_DATA data, void* const usr)
{
	assert(f != NULL);
	assert(read != NULL);
	assert(data != NULL);

	data_t buf[BATCH_SIZE];
	size_t n = 0, N = 0;
	do
	{
		n = read(f, buf, BATCH_SIZE);
#ifndef NDEBUG
		const int ret = data(buf, n, usr);
		assert(ret == EXIT_SUCCESS);
#else
		data(buf, n, usr);
#endif
		for (size_t i = 0; i < n; i++)
		{
			free(buf[i].buf);
		}

		N += n;
	} while (n >= BATCH_SIZE);

	return N;
}

const size_t file_recv(file_t* const f, FN_DATA callback, void* const usr)
{
	return recv_stub(f, file_read, callback, usr);
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

const size_t archive_recv(file_t* const f, FN_DATA callback, void* const usr)
{
	return recv_stub(f, archive_read, callback, usr);
}
#endif

#ifdef USE_NETWORK

#define PROCESS(hs,count) { \
	data[0].buf = (hs)->data; \
	data[0].len = (hs)->count; \
	nids_user.callback(data, 1, nids_user.data); \
	nids_user.num_chunks++; \
}

void net_recv_tcp(struct tcp_stream* const s, void** reserved)
{
	// For the network mode we would like to process the data as fast as
	// possible on a stream basis -> BUFFER_SIZE = 1
	static data_t data[1];

	switch (s->nids_state)
	{
	case NIDS_JUST_EST:
		s->client.collect++;
		s->server.collect++;
#ifdef WE_WANT_URGENT_DATA
		s->server.collect_urg++;
		a_tcp->client.collect_urg++;
#endif
		return;

	case NIDS_DATA:
        if (nids_user.merge_payloads)
        {
            nids_discard(s, 0);
            return;
        }

        // Check who sent the data
		struct half_stream* hlf = (s->client.count_new ? &s->client : &s->server);
		PROCESS(hlf, count_new);
        break;

	case NIDS_CLOSE: case NIDS_RESET: case NIDS_TIMED_OUT:
        if (nids_user.merge_payloads)
        {
			if (s->server.count != 0) PROCESS(&s->server, count);
			if (s->client.count != 0) PROCESS(&s->client, count);
        }
        break;
	}
}


const size_t net_recv(file_t* const f, FN_DATA callback, void* const usr)
{
	nids_user.callback = callback;
	nids_user.data = usr;
	nids_user.merge_payloads = TRUE;

	//nids_register_udp((void *) net_recv_udp);
	nids_register_tcp((void*) net_recv_tcp);

	// Disable checksum control
	static struct nids_chksum_ctl ctl;
	ctl.netaddr = 0;
	ctl.mask = 0;
	ctl.action = NIDS_DONT_CHKSUM;
	nids_register_chksum_ctl(&ctl, 1);

	nids_run();
	return nids_user.num_chunks;
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

#ifdef USE_NETWORK
const int net_close(file_t* const f)
{
	assert(f != NULL);

	nids_exit();
	pcap_close(nids_params.pcap_desc);

	return EXIT_SUCCESS;
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
