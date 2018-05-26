
/* test-uri.c: ADD DESCRIPTION HERE
 *
 * Copyright (C) 2017 Konrad Lother <konrad@hiddenbox.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */
#include <stdio.h>

#include "distexec/uri.h"
#include "distexec/error.h"
#include "distexec/logger.h"

#define TEST_SAME(u1, u2, a) \
	printf("Comparing '%s' with '%s'\n", u1->a, u2->a); \
	if (strcmp(u1->a, u2->a) != 0) \
		LIBDISTEXEC_ABORT(-1, "not same");

#define TEST_SAME_PARAM(u1, u2, p) \
	do { \
		char *t1 = libdistexec_uri_get_param(u1, p); \
		char *t2 = libdistexec_uri_get_param(u2, p); \
		printf("Comparing param %s: '%s' == '%s'\n", p, t1, t2); \
		if (strcmp(t1, t2) != 0) \
			LIBDISTEXEC_ABORT(-1, "param not same"); \
	} while(0);

int main(void) {

	libdistexec_logger_init(stdout, stdout);
	libdistexec_logger_set_level(LALL);

	libdistexec_uri_t *uri = libdistexec_uri_new();
	libdistexec_uri_t *uri1 = libdistexec_uri_new();
	int res = libdistexec_uri_parse("http://host.name:1234/path/index.php?key=value%20space", uri1);
	if (res != 0)
		LIBDISTEXEC_ABORT(-1, "libdistexec_uri_parse failed");

	uri->scheme = "http";
	uri->hostname = "host.name";
	uri->port = "1234";
	uri->path = "path/index.php";
	libdistexec_uri_add_get_param(uri, "key", "value space");
//	libdistexec_uri_add_get_param(uri, "key1", "*value");

	TEST_SAME(uri, uri1, scheme);
	TEST_SAME(uri, uri1, hostname);
	TEST_SAME(uri, uri1, port);
	TEST_SAME(uri, uri1, path);

	TEST_SAME_PARAM(uri, uri1, "key");
	//TEST_SAME_PARAM(uri, uri1, "key1");

	printf("test success\n");
	return 0;
}
