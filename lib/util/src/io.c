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

#include "util/io.h"
#include "util/util.h"
#include "util/log.h"
#include "util/getline.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>

#include "util/config.h"

#ifdef USE_REGEX_FILTER
#include <regex.h>
// XXX: This heavily relies on implementation details :/
#define REGEX_ISVALID(x) (x.__REPB_PREFIX(buffer) != NULL)
#endif

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
		metadata_t* meta;
		FN_DATA callback;
		void* data;
		int merge_payloads;
		size_t num_chunks;
		uint8_t state;
	} nids_user;

#endif

const char* const iomode_to_string(const iomode_t m)
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

const iomode_t to_iomode(const char* const str)
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

void metadata_free(metadata_t* const meta)
{
	assert(meta != NULL);

#ifdef EXTENDED_METADATA
	if (meta->filenames != NULL)
	{
		for (size_t i = 0; i < meta->num_items; i++)
		{
			if (meta->filenames[i] != NULL)
			{
				free(meta->filenames[i]);
			}
		}
		free(meta->filenames);
	}
#endif

#ifdef GROUPED_INPUT
	if (meta->groups != NULL)
	{
		for (size_t i = 0; i < meta->num_groups; i++)
		{
			free(meta->groups[i].name);
		}
		free(meta->groups);
	}
#endif
}

void metadata_destroy(metadata_t* const meta)
{
	assert(meta != NULL);
	metadata_free(meta);
	free(meta);
}

void destroy_slices(slices_t* const c)
{
	free(c->x);
	c->x = NULL;
	c->capacity = 0;
	c->n = 0;
}

void free_slices(slices_t* s)
{
	destroy_slices(s);
	free(s);
}

void data_free(data_t* const d)
{
	assert(d != NULL);
	free(d->buf);

	destroy_slices(&d->slices);
}

void data_destroy(data_t* const d)
{
	assert(d != NULL);
	data_free(d);
	free(d);
}

void dataset_free(dataset_t* const ds)
{
	assert(ds != NULL);

	assert((ds->n == 0 && ds->capacity == 0 && ds->data == NULL)
		|| (ds->n  > 0 && ds->capacity >= ds->n && ds->data != NULL));

	if (ds->capacity > 0)
	{
		for (int i = 0; i < ds->n; i++)
		{
			data_free(&ds->data[i]);
		}
		free(ds->data);
	}
}

void dataset_destroy(dataset_t* const ds)
{
	assert(ds != NULL);
	dataset_free(ds);
	free(ds);
}

const file_iomode_t to_fileiomode(const char* const s)
{
	switch (cmp(s, FILE_IO_READ, FILE_IO_WRITE, NULL))
	{
	case 0:  return FILE_IOMODE_READ;
	case 1:  return FILE_IOMODE_WRITE;
	default: return 0;
	}
}

const char* const fileiomode_tostring(file_iomode_t mode)
{
	switch (mode)
	{
	case FILE_IOMODE_READ:  return FILE_IO_READ;
	case FILE_IOMODE_WRITE: return FILE_IO_WRITE;
	default:                return NULL;
	}
}


const int all_filter(file_t* const f, const char* const pattern);
const int all_filter_ex(file_t* const f, const char* const pattern);
const int all_filter_close(file_t* const f);

const int    file_open(file_t* const f, const char* const filename, const char* mode, void *const p);
const int    file_meta(file_t* const f, const int group_input);
const size_t file_read(file_t* const f, dataset_t* const ds, const size_t num_lines);
const size_t file_recv(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr);
const size_t file_write(file_t* const f, const dataset_t* const ds, void* const usr);
const int    file_close(file_t* const f);
const int    file_close_ex(file_t* const f, int keep_metadata);

data_processor_t dp_lines   = { .open = file_open, .meta = file_meta, .filter = all_filter, .read = file_read, .recv = file_recv, .write = file_write, .close = file_close };
data_processor_t dp_files   = { .open = NULL, .meta = NULL, .filter = all_filter, .read = NULL, .recv = NULL, .write = NULL, .close = NULL };


#ifdef USE_ARCHIVES
const int    archive_open(file_t* const f, const char* const filename, const char* mode, void *const p);
const int    archive_meta(file_t* const f, const int group_input);
const size_t archive_read(file_t* const f, dataset_t* const ds, const size_t num_files);
const size_t archive_recv(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr);
const size_t archive_write(file_t* const f, const dataset_t* const ds, void* const usr);
const int    archive_close(file_t* const f);
const int    archive_close_ex(file_t* const f, int keep_metadata);

data_processor_t dp_archive = { .open = archive_open, .meta = archive_meta, .filter = all_filter, .read = archive_read, .recv = archive_recv, .write = archive_write, .close = archive_close };
#endif


#ifdef USE_NETWORK
const int    net_open(file_t* const f, const char* const filename, const char* mode, void *const p);
const int    net_meta(file_t* const f, const int group_input);
const int    net_filter(file_t* const f, const char* const pattern);
const size_t net_read(file_t* const f, dataset_t* const ds, const size_t num_streams);
const size_t net_recv(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr);
const size_t net_write(file_t* const f, const dataset_t* const ds, void* const usr);
const int    net_close(file_t* const f);

data_processor_t dp_network = { .open = net_open, .meta = net_meta, .filter = net_filter, .read = NULL, .recv = net_recv, .write = NULL, .close = net_close };
#endif

const data_processor_t* const to_dataprocssor(const iomode_t m)
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


// FILTER
const int all_filter(file_t* const f, const char* const pattern)
{
#ifdef USE_REGEX_FILTER
	if (REGEX_ISVALID(f->filter)) {
		regfree(&f->filter);
	}
#endif
	return all_filter_ex(f, pattern);
}

const int all_filter_ex(file_t* const f, const char* const pattern)
{
#ifdef USE_REGEX_FILTER
    if (regcomp(&f->filter, pattern, REG_EXTENDED) != 0) {
        return EXIT_FAILURE;
    }
#endif
	return EXIT_SUCCESS;
}

const int net_filter(file_t* const f, const char* const pattern)
{
	return EXIT_SUCCESS;
}

const int all_filter_close(file_t* const f)
{
#ifdef USE_REGEX_FILTER
	regfree(&f->filter);
#endif
	return EXIT_SUCCESS;
}


// OPEN
const int file_open(file_t* const f, const char* const filename, const char* mode, void *const p)
{
	assert(f != NULL);
	f->mode = to_fileiomode(mode);
	assert(mode != NULL && fileiomode_tostring(f->mode) != NULL);

	f->fd = fopen(filename, (f->mode == FILE_IOMODE_READ ? "rb" : "wb"));
	if (f->fd == NULL)
	{
		return EXIT_FAILURE;
	}
	f->data = NULL;
	// XXX: This doesn't work on Microsoft Windows OSs
	f->is_device = (strncmp(filename, "/dev/", 5) == 0);

	if (p != REOPEN)
	{
		memset(&f->meta, 0x00, sizeof(metadata_t));
		f->meta.filename = filename;
		all_filter_ex(f, "");
	}
	return EXIT_SUCCESS;
}

#ifdef USE_ARCHIVES
const int archive_open(file_t* const f, const char* const filename, const char* mode, void* const p)
{
	int ret = file_open(f, filename, mode, p);
	if (ret != EXIT_SUCCESS)
	{
		return ret;
	}


	struct archive* a = NULL;
	switch (f->mode)
	{
	case FILE_IOMODE_READ:
	{
		a = archive_read_new();
	#ifdef LIBARCHIVE2
		archive_read_support_compression_all(a);
	#else
		archive_read_support_filter_all(a);
	#endif
		archive_read_support_format_all(a);

		ret = archive_read_open_FILE(a, f->fd);
		break;
	}
	case FILE_IOMODE_WRITE:
	{
		a = archive_write_new();
		archive_write_add_filter_gzip(a);
		archive_write_set_format_pax_restricted(a);
		ret = archive_write_open_FILE(a, f->fd);
		break;
	}}


	if (ret != ARCHIVE_OK)
	{
		return EXIT_FAILURE;
	}

	f->data = a;
	return EXIT_SUCCESS;
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
}

const int net_open(file_t* const f, const char* const filename, const char* mode, void *const p)
{
	assert(f != NULL);
	f->mode = to_fileiomode(mode);
	assert(mode != NULL && f->mode == FILE_IOMODE_READ);

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
	f->is_device = params->is_device;

	// No "real" reopen possible
	memset(&f->meta, 0x00, sizeof(metadata_t));
	f->meta.filename = filename;
	all_filter_ex(f, "");

	nids_params.pcap_desc = nids_open();

	if (!nids_init())
	{
		params->error_msg = nids_errbuf;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
#endif


// META
void _progress(metadata_t* const meta, const size_t cur)
{
	progress(cur, meta->num_items);
}

void _hourglass(metadata_t* const meta, const size_t cur)
{
	static uint8_t state = 0;
	hourglass(&state, cur);
}

const int file_meta(file_t* const f, const int group_input)
{
	assert(f != NULL);
	assert(group_input == FALSE); // NOT SUPPORTED

	f->meta.num_items = 0;
	f->meta.total_size = 0; // TODO: some up lengths -- strlen() -3*num_% -num_\n
	size_t num_perc = 0;

	// TODO: Apply the input-filter in order to correctly determine the
	//       correct number of input strings.

	int last = 0x00;
	for (int ch = fgetc(f->fd); ch != EOF; ch = fgetc(f->fd))
	{
		switch (ch)
		{
		case '\n': f->meta.num_items++;  break;
		case '%':  num_perc++;           break;
		default:   f->meta.total_size++; break;
		}
		last = ch;
	}

	if (last != '\n')
	{
		f->meta.num_items++;
	}

	int is_percentencoded = FALSE;
	if (is_percentencoded)
	{
		// This expects the input to be well-formatted of course an therefore
		// might only be an approximation.
		f->meta.total_size -= 2 *num_perc;
	}
	else
	{
		f->meta.total_size += num_perc;
	}

#ifdef EXTENDED_METADATA
	f->meta.filenames = NULL;
#endif
#ifdef GROUPED_INPUT
	f->meta.groups = NULL;
	f->meta.num_groups = 0;
#endif

	// reopen file
	file_close_ex(f, TRUE);
	return file_open(f, f->meta.filename, fileiomode_tostring(f->mode), REOPEN);
}

#ifdef USE_ARCHIVES
const int archive_meta(file_t* const f, const int group_input)
{
	assert(f != NULL);

	metadata_t* const meta = &f->meta;
	meta->num_items  = 0;
	meta->total_size = 0;

#ifdef EXTENDED_METADATA
	size_t fnames_capacity = 1;
	meta->filenames = (char**) calloc(fnames_capacity, sizeof(char*));
#endif

#ifdef GROUPED_INPUT
	size_t groups_capacity = 1;
	meta->groups = (group_t*) calloc(groups_capacity, sizeof(group_t));
	meta->num_groups = 0;

	group_t* cur = &meta->groups[0]; cur->name = "";
#endif
#ifdef USE_REGEX_FILTER
    regmatch_t m[1];
#endif

	struct archive* a = (struct archive*) f->data;
	struct archive_entry* entry = NULL;

	int r = ARCHIVE_FAILED;
	while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
	{
        if (archive_entry_filetype(entry) == AE_IFREG)
        {
        	// is optimized away ifndef EXTENDED_METADATA, GROUPED_INPUT, USE_REGEX_FILTER
        	const char* const name = archive_entry_pathname(entry); UNUSED(name);

#ifdef GROUPED_INPUT
        	const char* const slash = strrchr(name, '/');

    		if (slash == NULL || strncmp(name, cur->name, slash -name +1) != 0)
    		{
    			// new class
    			if (meta->num_groups >= groups_capacity)
				{
					groups_capacity *= 2;
					const size_t s = groups_capacity *sizeof(group_t);
					meta->groups = (group_t*) realloc(meta->groups, s);
				}
				cur = &meta->groups[meta->num_groups++];
				STRNDUP((slash == NULL ? strlen(name) : slash -name) +1, name, cur->name);
				cur->n = 1;
    		}
    		else
    		{
    			cur->n++;
    		}
#endif
#ifdef USE_REGEX_FILTER
    	    if (regexec(&f->filter, name, 1, m, 0) != 0)
    	    {
    	    	continue;
    	    }
#endif
#ifdef EXTENDED_METADATA
			if (meta->num_items >= fnames_capacity)
			{
				fnames_capacity *= 2;
				const size_t s = fnames_capacity *sizeof(char*);
				meta->filenames = (char**) realloc(meta->filenames, s);
			}
        	STRDUP(name, meta->filenames[meta->num_items]);
#endif
    		meta->num_items++;
            meta->total_size += (size_t) archive_entry_size(entry);
        }
        archive_read_data_skip(a);
	}
#ifdef EXTENDED_METADATA
	meta->filenames = (char**) realloc(meta->filenames, meta->num_items *sizeof(char*));
#endif
#ifdef GROUPED_INPUT
	meta->groups = (group_t*) realloc(meta->groups, meta->num_groups *sizeof(group_t));
#endif

	// reopen archive
	archive_close_ex(f, TRUE);
	archive_open(f, f->meta.filename, fileiomode_tostring(f->mode), REOPEN);

	return (r == ARCHIVE_EOF ? EXIT_SUCCESS : EXIT_FAILURE);
}
#endif

#ifdef USE_NETWORK
const int net_meta(file_t* const f, const int group_input)
{
	assert(f != NULL);
	assert(group_input == FALSE); // NOT SUPPORTED

	// We are gonna set the the number of items and the total size
	// on the fly. There is no way to know this beforehand for live
	// network traffic and without parsing/ reassembling the streams.
	f->meta.num_items = 0;
	f->meta.total_size = 0;

#ifdef EXTENDED_METADATA
	f->meta.filenames = NULL;
#endif
#ifdef GROUPED_INPUT
	f->meta.groups = NULL;
	f->meta.num_groups = 0;
#endif

	return EXIT_SUCCESS;
}
#endif

// READ
const size_t file_read(file_t* const f, dataset_t* const ds, const size_t num_lines)
{
	assert(f != NULL);
	assert(ds != NULL);
	assert(num_lines <= ds->capacity);

	if (ds->capacity <= 0 || ds->data == NULL)
	{
		return 0;
	}

#ifdef USE_REGEX_FILTER
    regmatch_t m[1];
#endif

	static char strip[256] = {0};
	strip[(int) '\n'] = 1;
	strip[(int) '\r'] = 1;

	char* line = NULL;
	size_t i = 0, len = 0;

	const size_t n = MIN(num_lines, ds->capacity);
	while (i < n)
	{
		ssize_t read = getline(&line, &len, f->fd);
		if (read <= -1) break;

		int j;
		for (j = read -1; j >= 0; j--)
		{
			if (!strip[(int) line[j]]) break;
		}
		line[j +1] = 0x00;

#ifdef USE_REGEX_FILTER
	    /* No match found */
	    if (regexec(&f->filter, line, 1, m, 0) != 0)
	    {
	    	continue;
	    }
#endif

		size_t n = inline_decode(line, strlen(line));
		STRNDUP(n, line, ds->data[i].buf);
		ds->data[i].len = n;
		i++;
	}

	ds->n = i;
	free(line);
	return i;
}

#ifdef USE_ARCHIVES
#ifdef GROUPED_INPUT
group_t* const group_next(metadata_t* const meta, int* const gid, int* const fid)
{
	if (*gid >= meta->num_groups)
	{
		return NULL;
	}

	if (*fid >= meta->groups[*gid].n)
	{
		(*gid)++;
		(*fid) = 0;
		return group_next(meta, gid, fid);
	}

	(*fid)++;
	return &meta->groups[*gid];
}
#endif

const size_t archive_read(file_t* const f, dataset_t* const ds, const size_t num_files)
{
	assert(f != NULL);
	assert(ds != NULL);
	assert(num_files <= ds->capacity);

	if (ds->capacity <= 0 || ds->data == NULL)
	{
		return 0;
	}

	data_t* const data = ds->data;

	struct archive* a = (struct archive*) f->data;
	struct archive_entry* entry = NULL;

#ifdef GROUPED_INPUT
	int fid = 0, gid = 0;
#endif

#ifdef USE_REGEX_FILTER
    regmatch_t m[1];
#endif

	size_t i = 0;
	const size_t n = MIN(num_files, ds->capacity);
	while (i < n)
	{
		if (archive_read_next_header(a, &entry) != ARCHIVE_OK)
		{
			ds->n = i;
			return i;
		}

#ifdef USE_REGEX_FILTER
    	const char* const name = archive_entry_pathname(entry);
	    if (regexec(&f->filter, name, 1, m, 0) != 0)
	    {
	    	continue;
	    }
#endif

        if (archive_entry_filetype(entry) == AE_IFREG)
        {
        	data_t* const cur = &data[i++];
        	if (archive_entry_size_is_set(entry))
        	{
				cur->len = (size_t) archive_entry_size(entry);
				cur->buf = (char*) malloc(cur->len * sizeof(char));
				archive_read_data(a, cur->buf, cur->len);
        	}
        	else
        	{
        		// "length at end" (zip) file
        		static const unsigned int BLOCK_SIZE = 102400;
        		unsigned long capacity = BLOCK_SIZE;

				cur->buf = (char*) malloc(capacity * sizeof(char));
				cur->len = 0;

				size_t read = 0;
				do
				{
					if (cur->len +BLOCK_SIZE > capacity)
					{
						capacity *= 2;
						cur->buf = (char*) realloc(cur->buf, capacity * sizeof(char));
					}

					char* x = cur->buf +cur->len;
					read = archive_read_data(a, x, BLOCK_SIZE);
					cur->len += read;

				} while (read > 0);

				cur->buf = (char*) realloc(cur->buf, cur->len * sizeof(char));
        	}

#ifdef GROUPED_INPUT
            cur->meta = group_next(&f->meta, &fid, &gid);
#endif
        }
        archive_read_data_skip(a);
	}
	ds->n = i;
	return i;
}
#endif

// RECV
const size_t recv_stub(file_t* const f, FN_READ read, FN_DATA data, const size_t batch_size, void* const usr)
{
	assert(f != NULL);
	assert(read != NULL);
	assert(data != NULL);

	dataset_t buf;
	buf.capacity = batch_size;
	buf.data = (data_t*) calloc(buf.capacity, sizeof(data_t));
	buf.n = 0;

	size_t n = 0, N = 0;
	while ((n = read(f, &buf, batch_size)) > 0)
	{
#ifndef NDEBUG
		const int ret = data(buf.data, buf.n, usr);
		assert(ret == EXIT_SUCCESS);
#else
		data(buf.data, buf.n, usr);
#endif
		// Do not use dataset_free(.) since we want to reuse the
		// memory consumed by dataset_t#data
		for (int i = 0; i < buf.n; i++)
		{
			data_free(&buf.data[i]);
		}
		N += n;
		progress(N, f->meta.num_items);
	};

	free(buf.data); // cf. comment above
	return N;
}

const size_t file_recv(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr)
{
	return recv_stub(f, file_read, callback, batch_size, usr);
}

#ifdef USE_ARCHIVES
const size_t archive_recv(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr)
{
	return recv_stub(f, archive_read, callback, batch_size, usr);
}
#endif

#ifdef USE_NETWORK

#define PROCESS(hs,count) { \
	data[0].buf = (hs)->data; \
	data[0].len = (hs)->count; \
	nids_user.meta->num_items++; \
	nids_user.meta->total_size += data[0].len; \
	nids_user.callback(data, 1, nids_user.data); \
	nids_user.num_chunks++; \
	hourglass(&nids_user.state, nids_user.num_chunks); \
}

void net_recv_tcp(struct tcp_stream* const s, void** reserved)
{
	// --batch-size is ignored for network streams
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


const size_t net_recv(file_t* const f, FN_DATA callback, const size_t batch_size, void* const usr)
{
	// For the network mode we would like to process the data as recent as
	// possible on a stream basis
	assert(batch_size == 1);

	nids_user.meta = &f->meta;
	nids_user.callback = callback;
	nids_user.data = usr;
	nids_user.merge_payloads = TRUE;
	nids_user.state = 0;

	//nids_register_udp((void *) net_recv_udp);
	nids_register_tcp((void*) net_recv_tcp);

	// Disable checksum control
	static struct nids_chksum_ctl ctl;
	ctl.netaddr = 0;
	ctl.mask = 0;
	ctl.action = NIDS_DONT_CHKSUM;
	nids_register_chksum_ctl(&ctl, 1);

	nids_run();

	hourglass_stop();
	return nids_user.num_chunks;
}
#endif


// WRITE
#define FILE_WRITESLICE(f, buf, _start_, end) {               \
	const size_t start = _start_;                             \
	const size_t slen = (end) -start;                         \
	const size_t x = encode(&line, &len, buf +start, slen);   \
	                                                          \
	const size_t nwrote = fwrite(line, sizeof(char), x, f);   \
	N += nwrote;                                              \
	if (nwrote != x)                                          \
	{	                                                      \
		break;                                                \
	}	                                                      \
}

const size_t file_write(file_t* const f, const dataset_t* const ds, void* const usr)
{
	assert(f != NULL);
	assert(ds != NULL);

	if (ds->n <= 0 || ds->capacity <= 0 || ds->data == NULL)
	{
		return 0;
	}

	char* line = NULL;
	size_t N = 0, len = 0;

	for (size_t i = 0; i < ds->n; i++)
	{
		const data_t* const d = &ds->data[i];
		if (d->slices.n > 0)
		{
			for (size_t j = 0; j < d->slices.n; j++)
			{
				const slice_t* const s = &d->slices.x[j];
				FILE_WRITESLICE(f->fd, d->buf, s->start, s->end);
			}
		}
		else
		{
			FILE_WRITESLICE(f->fd, d->buf, 0, d->len);
		}

		const size_t nwrote = fwrite("\n", sizeof(char), 1, f->fd);
		N += nwrote;
		if (nwrote != 1) break;
	}

	free(line);
	return N;
}


#ifdef USE_ARCHIVES
#define ARCHIVE_WRITESLICE(a, buf, _start_, end) {            \
	const size_t start = _start_;                             \
	const size_t len = ((end) -start);                        \
	const size_t n = archive_write_data(a, buf +start, len);  \
	if (n != len)                                             \
	{	                                                      \
		return i;                                             \
	}	                                                      \
}

const size_t archive_write(file_t* const f, const dataset_t* const ds, void* const usr)
{
	assert(f != NULL);
	assert(ds != NULL);

	if (ds->n <= 0 || ds->capacity <= 0 || ds->data == NULL)
	{
		return 0;
	}

	struct archive* a = (struct archive*) f->data;
	struct archive_entry* entry = archive_entry_new();

	for (size_t i = 0; i < ds->n; i++)
	{
		const data_t* const d = &ds->data[i];
		const char* const fname = d->meta->name;
		size_t len = d->len;

		const int has_slices = (d->slices.n > 0);

		if (has_slices)
		{
			len = 0;
			for (size_t j = 0; j < d->slices.n; j++)
			{
				slice_t* const s = &d->slices.x[j];
				len += (s->end -s->start);
			}
		}

		archive_entry_clear(entry);
		archive_entry_set_pathname(entry, fname);
		archive_entry_set_size(entry, len);
		archive_entry_set_filetype(entry, AE_IFREG);
		archive_entry_set_perm(entry, 0644);
		archive_write_header(a, entry);

		if (has_slices)
		{
			for (size_t j = 0; j < d->slices.n; j++)
			{
				slice_t* const s = &d->slices.x[j];
				ARCHIVE_WRITESLICE(a, d->buf, s->start, s->end);
			}
		}
		else
		{
			ARCHIVE_WRITESLICE(a, d->buf, 0, d->len);
		}
		archive_entry_free(entry);
	}
	return ds->n;
}
#endif


// CLOSE
const int file_close_ex(file_t* const f, int keep_metadata)
{
	assert(f != NULL);

	if (fclose(f->fd) != 0)
	{
		return EXIT_FAILURE;
	}
	f->fd = NULL;

	if (!keep_metadata)
	{
		metadata_free(&f->meta);
		all_filter_close(f);
	}
	return EXIT_SUCCESS;
}

const int file_close(file_t* const f)
{
	return file_close_ex(f, FALSE);
}

#ifdef USE_ARCHIVES
typedef int (*FN_ARCHIVE)(struct archive *);

const int archive_close_ex(file_t* const f, int keep_metadata)
{
	assert(f != NULL);
	FN_ARCHIVE close_archive = NULL;
	FN_ARCHIVE free_archive = NULL;

	if (f->mode == FILE_IOMODE_READ)
	{
		close_archive = archive_read_close;
#ifdef LIBARCHIVE2
		free_archive = archive_read_finish;
#else
		free_archive = archive_read_free;
#endif
	}
	else if (f->mode == FILE_IOMODE_WRITE)
	{
		close_archive = archive_write_close;
#ifdef LIBARCHIVE2
		free_archive = archive_write_finish;
#else
		free_archive = archive_write_free;
#endif
	}

	assert(close_archive != NULL);
	assert(free_archive != NULL);

	if (close_archive((struct archive*) f->data) != ARCHIVE_OK ||
		free_archive((struct archive*) f->data) != ARCHIVE_OK)
	{
		return EXIT_FAILURE;
	}
	f->data = NULL;

	if (!keep_metadata)
	{
		metadata_free(&f->meta);
		all_filter_close(f);
	}
	return file_close_ex(f, TRUE);
}

const int archive_close(file_t* const f)
{
	return archive_close_ex(f, FALSE);
}
#endif

#ifdef USE_NETWORK
const int net_close(file_t* const f)
{
	assert(f != NULL);
	all_filter_close(f);

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
