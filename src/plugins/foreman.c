
/* foreman.c: foreman backend for distexec
 *
 * Copyright (C) 2017 Konrad Lother <konrad@hiddenbox.org>
 *               2017 Lukas Kropatschek <lukas@ĸropatschek.net>
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

/*
 *  TODOS:
 *	- api calls must have per_page parameter or foreman default (20 per page)
 *	  is used. (this means only 20 results are returned)
 *
 *	  foreman includes the total count of the result in its response so either
 *	    a) call once to get total count then call again for all(!) results
 *	       (this might generate load on the server, so maybe not the best solution)
 *	    b) call page for page. (check subtotal field in json response, seems to be
 *	       the total page count, not sure)
 *
 *
 */

#include <curl/curl.h>
#include <json-c/json.h>
#include <math.h>

#include "distexec/export.h"
#include "distexec/error.h"
#include "distexec/plugin.h"
#include "distexec/logger.h"
#include "distexec/util.h"
#include "distexec/node.h"
#include "distexec/uri.h"

#undef MACRO_LOGGER
#define MACRO_LOGGER logger

#define CHECK_CONFIG \
	if(cfg.url == NULL \
			|| cfg.username == NULL \
			|| cfg.password == NULL \
			|| cfg.ssl_verify == NULL) \
			LIBDISTEXEC_ABORT(-1, "Backend is not configured");

static libdistexec_logger_t logger;
static libdistexec_uri_t baseuri;

static CURL *curl;
static CURLcode get_json(const char *url);
struct curl_slist *headers = NULL;
static int current_page;
static int pages;

static int collect(const char *filter);

static struct config {
	const char *url;
	const char *username;
	const char *password;
	char *ssl_verify;
	char *per_page;
} cfg;

static const char *backend_config_values[] = {
	"url",
	"username",
	"password",
	"per-page",
	"ssl-verify",
};

static struct http_response {
	char *memory;
	size_t size;
} chunk;

static int set_config_value(const char *key, const char *value)
{
	LOG_DEBUG("set config %s -> %s", key, value);
	if (strcmp(key, "url") == 0)
		cfg.url = strdup(value);
	else if (strcmp(key, "username") == 0)
		cfg.username = strdup(value);
	else if (strcmp(key, "password") == 0)
		cfg.password = strdup(value);
	else if (strcmp(key, "ssl-verify") == 0)
		cfg.ssl_verify = strdup(value);
	else if (strcmp(key, "per-page") == 0)
		cfg.per_page = strdup(value);
	else
		LIBDISTEXEC_ABORT(-1, "Unknown config key: %s", key);
	return 0;

}

static size_t write_memory_callback(void *contents, size_t size,
				    size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct http_response *resp = (struct http_response *)userp;

	resp->memory = realloc(resp->memory, resp->size + realsize + 1);
	if (resp->memory == NULL)
		LIBDISTEXEC_ABORT(-1, "Out of memory!");

	memcpy(&(resp->memory[resp->size]), contents, realsize);
	resp->size += realsize;
	resp->memory[resp->size] = 0;

	return realsize;
}

static int collect_init()
{
	LOG_DEBUG("INITTTTT");
	cfg.url = NULL;
	cfg.username = NULL;
	cfg.password = NULL;
	cfg.ssl_verify = "false";
	cfg.per_page = "50";	// default value

	return 0;
}

EXPORT int load()
{
	libdistexec_logger_new(&logger, "foreman");
	LOG_DEBUG("loading");
	if (libdistexec_register_callback_collect
	    ("foreman", collect, collect_init, NULL, set_config_value,
	     backend_config_values, ARRAY_SIZE(backend_config_values))) {
		return -1;
	}
	return 0;
}

static int get_hosts()
{
	json_object *json;
	json_object *tmp = NULL;
	enum json_tokener_error json_err = json_tokener_success;

	json = json_tokener_parse_verbose(chunk.memory, &json_err);

	if (json_err != json_tokener_success)
		LIBDISTEXEC_ABORT(-1, "Could not parse json");

#define __parse_int(type, varname, field) \
	json_object_object_get_ex(json, field, &tmp); \
	type varname = json_object_get_int(tmp); \
	tmp = NULL;

	__parse_int(int, total, "total");
	__parse_int(int, subtotal, "subtotal");
	__parse_int(int, page, "page");
	__parse_int(int, per_page, "per_page");

	if (subtotal == 0)	/* no results */
		return 0;

	// current page in chunk
	current_page = page;

	// total page count
	if (subtotal >= per_page) {
		float fpages = (float)subtotal / (float)per_page;
		pages = ceil(fpages);
	} else {
		pages = 1;
	}

	json_object_object_get_ex(json, "results", &tmp);
	json_object_array_length(tmp);

	LOG_DEBUG("total: %d, subtotal: %d, page: %d, per_page: %d, pages: %d",
		  total, subtotal, page, per_page, pages);

	int n = 0;
	do {
		// save current chunk of nodes to buffer
		int i;
		for (i = 0; i < json_object_array_length(tmp); i++) {
			libdistexec_node_t *node = libdistexec_node_new();
			if (NULL == node)
				LIBDISTEXEC_ABORT(-1, "node can not be NULL");

			json_object *o = json_object_array_get_idx(tmp, i);
			json_object *t;
			json_object_object_get_ex(o, "name", &t);

			node->hostname = strdup(json_object_get_string(t));
			if (NULL == node->hostname)
				LIBDISTEXEC_ABORT(-1,
						  "hostname must not be NULL");

			libdistexec_node_add(node);
		}
		if (pages == 1)
			break;

		char pg_str[8];
		snprintf(pg_str, 8, "%d", ++current_page);
		LOG_DEBUG("collecting page... %s", pg_str);
		libdistexec_uri_set_get_param(&baseuri, "page", pg_str);
		// collect next page of nodes
		char *url = libdistexec_uri_build(&baseuri);
		if (get_json(url) != 0)
			LIBDISTEXEC_ABORT(-1, "get_json failed");

		json_object *o;
		enum json_tokener_error json_err = json_tokener_success;
		o = json_tokener_parse_verbose(chunk.memory, &json_err);
		if (json_err != json_tokener_success)
			LIBDISTEXEC_ABORT(-1, "Could not parse json");

		free(tmp);
		tmp = NULL;
		json_object_object_get_ex(o, "results", &tmp);

		free(url);
	} while (current_page <= pages);

	return n;
}

static CURLcode get_json(const char *url)
{
	CURLcode res;

	if (chunk.size != 0)
		free(chunk.memory);

	chunk.memory = malloc(1);
	chunk.size = 0;

	curl_easy_setopt(curl, CURLOPT_URL, url);
	res = curl_easy_perform(curl);
	return res;
}

static void cleanup_curl()
{
	curl_slist_free_all(headers);
	//free(chunk.memory);
	//chunk.memory = NULL;
	curl_easy_cleanup(curl);
	//curl = NULL;
	curl_global_cleanup();
//      libdistexec_uri_free(&baseuri);
}

static int init_curl()
{

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	if (!curl)
		LIBDISTEXEC_ABORT(-1, "Couldn't initialize cURL");

	chunk.memory = NULL;
	chunk.size = 0;

	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, "Content-Type: application/json");

	char *useragent = "libdistexec/0.0.1";

	curl_easy_setopt(curl, CURLOPT_USERAGENT, useragent);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_USERNAME, cfg.username);
	curl_easy_setopt(curl, CURLOPT_PASSWORD, cfg.password);

	if (strcmp(cfg.ssl_verify, "false") == 0) {
		// if not verifying, even accept self-signed certificates for now
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	}
	// send all data to this function
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
	// we pass our 'chunk' struct to the callback function
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

	return 0;
}

static int collect(const char *filter)
{

	LOG_DEBUG("url: %s, username: %s, pass: %s - %s", cfg.url, cfg.username, cfg.password, cfg.ssl_verify);
	CHECK_CONFIG;
	int rc = 0;

	if (init_curl() != 0)
		LIBDISTEXEC_ABORT(-1, "init_curl failed");

	if (libdistexec_uri_parse(cfg.url, &baseuri) != 0)
		LIBDISTEXEC_ABORT(-1, "Could not parse %s", cfg.url);

	if (NULL != filter)
		libdistexec_uri_add_get_param(&baseuri, "search", filter);

	// always collect n nodes per request
	libdistexec_uri_add_get_param(&baseuri, "per_page", cfg.per_page);

	// copy baseurl and add page argument
	libdistexec_uri_t *fp_uri = libdistexec_uri_copy(&baseuri);
	libdistexec_uri_add_get_param(fp_uri, "page", "1");

	char *fp_uri_str = libdistexec_uri_build(fp_uri);
	libdistexec_uri_free(fp_uri);

	LOG_DEBUG
	    ("url: %s, username: %s, password: %s, per-page: %s, verify: %s",
	     cfg.url, cfg.username, cfg.password, cfg.per_page, cfg.ssl_verify);

	if ((rc = get_json(fp_uri_str)) != CURLE_OK)
		LIBDISTEXEC_ABORT(rc, "Couldn't fetch JSON!");
	else if ((rc = get_hosts()) < 0)
		LIBDISTEXEC_ABORT(rc, "Could not parse json");

	free(fp_uri_str);
	cleanup_curl();

	return rc;
}

EXPORT int unload()
{
	return 0;
}
