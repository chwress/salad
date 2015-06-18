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


#ifdef USE_NETWORK
#include <nids.h>
#include <errno.h>
#include <signal.h>

#include <util/log.h>

struct
{
	metadata_t* meta;
	FN_DATA callback;
	void* data;
	int merge_payloads;
	size_t num_chunks;
	uint8_t state;
} nids_user;


// OPEN
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


// META
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


// FILTER
const int net_filter(file_t* const f, const char* const pattern)
{
	return EXIT_SUCCESS;
}


// RECV
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


// CLOSE
const int net_close(file_t* const f)
{
	assert(f != NULL);
	all_filter_close(f);

	nids_exit();
	pcap_close(nids_params.pcap_desc);

	return EXIT_SUCCESS;
}
#endif
