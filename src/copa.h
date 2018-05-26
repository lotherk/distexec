
/* copa.h: commandline parser
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

/**
 * @brief	Add brief description!
 *
 * More detailed description
 *
 * @date	06/10/2017
 * @file	copa.h
 * @author	Konrad Lother
 */

#ifndef COPA_H
#define COPA_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COPA_BANNER_BUFFER 4096
#define COPA_MAX_OPTS 128

#define CMD_FLAG '-'

typedef struct copa_opt_t copa_opt_t;
typedef struct copa_parser_t copa_parser_t;

struct copa_opt_t {
	copa_parser_t *parser;
	char arg_short;
	char *arg_long;
	char *description;
	char *values[1024];
	size_t values_s;
	int has_param;
	int given;
	void (*callback)(copa_opt_t*);
};

struct copa_parser_t {
	copa_opt_t *opts[COPA_MAX_OPTS];
	size_t opts_s;
	char *arguments[COPA_MAX_OPTS];
	size_t arguments_s;
	char *unknown[COPA_MAX_OPTS];
	size_t unknown_s;
	int config_parser;
};

copa_parser_t *copa_parser();
int copa_free(copa_parser_t *p);
void copa_callback(copa_parser_t *p);

int copa_parse_config_file(copa_parser_t *p, const char *filepath);

int copa_parse(copa_parser_t *p, char *argv[], int argc);
copa_opt_t *copa_opt(copa_parser_t *p, char s, char *l, char *desc, void (*callback)(copa_opt_t*));

#ifdef __cplusplus
}
#endif
#endif
