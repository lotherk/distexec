
/* copa.c: ADD DESCRIPTION HERE
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "copa.h"

static void opt_given(copa_opt_t * o, char *value)
{
	o->given = 1;
	if (NULL != value) {
		printf("add %s to %d\n", value, o->values_s);
		o->values[o->values_s++] = value;
	}
}

void copa_callback(copa_parser_t * p)
{
	int i;
	for (i = 0; i < p->opts_s; i++) {
		copa_opt_t *o = p->opts[i];

		if (o->given) {
			if (NULL != o->callback)
				o->callback(o);
		}
	}

}

copa_parser_t *copa_parser()
{
	copa_parser_t *p = malloc(sizeof(copa_parser_t));

	if (NULL == p) {
		errno = ENOMEM;
		return NULL;
	}
	p->arguments_s = 0;
	p->unknown_s = 0;
	p->opts_s = 0;
	p->config_parser = 0;
	return p;
}

copa_opt_t *copa_opt(copa_parser_t * p, char s, char *l, char *desc,
		     void (*callback) (copa_opt_t *))
{
	copa_opt_t *o = malloc(sizeof(copa_opt_t));

	if (NULL == o) {
		errno = ENOMEM;
		return NULL;
	}

	o->callback = callback;

	o->arg_short = s;
	o->arg_long = l;
	o->description = desc;
	o->given = 0;
	o->values_s = 0;
	o->parser = p;
	p->opts[p->opts_s++] = o;

	return o;
}

int copa_parse(copa_parser_t * p, char *argv[], int argc)
{
	int i;

	for (i = 0; i < argc; i++) {
		char *ptr;

		int flag = 0;

		for (ptr = argv[i]; *ptr != '\0'; ptr++)
			if (*ptr == CMD_FLAG)
				flag++;
			else
				break;

		size_t ptr_len = strlen(ptr);

		if (flag == 1 && ptr_len > 0) {
			// short argument(s)
			int x = 0;
			for (ptr = argv[i]; *ptr != '\0'; ptr++, x++) {
				int n;
				int found = 0;
				for (n = 0; n < p->opts_s; n++) {
					copa_opt_t *o = p->opts[n];

					if (o->arg_short == 0)
						continue;

					if (o->arg_short == *ptr) {
						found = 1;
						if (o->has_param
						    && (ptr_len - x) != 0) {
							fprintf(stderr,
								"error: argument '%c%c' accepts a parameter and can not be chained\n",
								CMD_FLAG, *ptr);
							fflush(stderr);
							exit(EXIT_FAILURE);
						} else if (o->has_param
							   && i + 1 < argc) {
							char *tmp_ptr =
							    argv[i + 1];
							if (*tmp_ptr !=
							    CMD_FLAG) {
								if (i + 1 >=
								    argc)
									opt_given
									    (o,
									     NULL);
								else
									opt_given
									    (o,
									     argv
									     [++i]);
							} else {
								opt_given(o,
									  NULL);
								/*fprintf(stderr, "error: argument '%c%c' requires a parameter\n", CMD_FLAG, *ptr);
								   fflush(stderr);
								   exit(EXIT_FAILURE); */
							}
						} else {
							opt_given(o, NULL);
						}
						break;
					}
				}

				if (!found && *ptr != CMD_FLAG) {
					p->unknown[p->unknown_s++] = argv[i];
				}

			}
			continue;
		} else if (flag == 2 && ptr_len > 0) {
			// long argument
			int n;
			int found = 0;
			for (n = 0; n < p->opts_s; n++) {
				copa_opt_t *o = p->opts[n];

				if (NULL == o->arg_long)
					continue;

				if (strcmp(o->arg_long, ptr) == 0) {
					found = 1;
					if (o->has_param && i + 1 < argc) {
						char *tmp_ptr = argv[i + 1];
						if (*tmp_ptr != CMD_FLAG) {
							if (i + 1 >= argc)
								opt_given(o,
									  NULL);
							else
								opt_given(o,
									  argv
									  [++i]);

						} else {
							opt_given(o, NULL);
							/*fprintf(stderr, "error: argument '%c%c%s' requires a parameter\n", CMD_FLAG, CMD_FLAG, ptr);
							   fflush(stderr);
							   exit(EXIT_FAILURE);
							 */
						}
					} else {
						opt_given(o, NULL);
					}
					break;
				}
			}

			if (!found) {
				p->unknown[p->unknown_s++] = argv[i];
			}
			continue;
		} else if (flag == 2 && ptr_len == 0) {
			// -a -b -c -- whatever
			// just add everything left in argv

			i++;	// skip '--'
			for (; i < argc; i++)
				p->arguments[p->arguments_s++] = argv[i];

			break;
		} else {
			// no flag or invalid, add to left_over
			p->unknown[p->unknown_s++] = argv[i];

		}

		/*
		 * check if there is any argv element starting with '-'
		 * if not, add everything to arguments and return.
		 */

		int n;
		int all_args = 1;
		for (n = i; n < argc; n++) {
			char *tmp = argv[n];
			if (*tmp == '-') {
				all_args = 0;
				break;
			}
		}

		if (all_args) {
			for (n = i; n < argc; n++)
				p->arguments[p->arguments_s++] = argv[n];
			break;
		}

	}
	return 0;
}

int copa_parse_config_file(copa_parser_t * p, const char *filepath)
{
	// read file
	// format:
	//
	// key   value # comment
	//
	// build argv like the shell would
	// pass to cope_parse(p, array, array_size);

	FILE *fp;
	size_t linelen = 1024;
	char line[linelen];
	char **buf = calloc(COPA_MAX_OPTS, sizeof(char *));
	if (NULL == buf)
		return -ENOMEM;

	size_t buf_i = 0;
	fp = fopen(filepath, "r");
	if (NULL == fp)
		return -errno;

	while (fgets(line, linelen, fp) != NULL) {
		char *p = line;

		// drop leading whitespaces
		for (; isspace(*p); *p++) ;

		if (*p == '#')
			continue;

		if (strlen(p) == 0)
			continue;

		enum { KEY_MODE, VAL_MODE } state = KEY_MODE;
		int in_string = 0;
		char keybuf[256] = { '\0' };
		size_t keybuf_i = 0;
		char valbuf[256] = { '\0' };
		size_t valbuf_i = 0;

		int first_val_char = 1;

		char last_char = 0;

		for (; *p != '\0'; *p++) {
			if (*p == '-' && *(p + 1) == '-' && isspace(*(p + 2))) {
				p += 3;
				buf[buf_i++] = strdup("--");
				state = VAL_MODE;
			}

			if (state == KEY_MODE && !isspace(*p)) {
				keybuf[keybuf_i++] = *p;
			}

			if (isspace(*p) && state != VAL_MODE) {
				state = VAL_MODE;
				int key_len = strlen(keybuf) + 3;	// -- + \0 = 3;
				buf[buf_i] = malloc(sizeof(char) * key_len);
				if (NULL == buf[buf_i])
					return -ENOMEM;

				snprintf(buf[buf_i], key_len, "--%s", keybuf);
				buf_i++;
				for (; isspace(*p); *p++) ;
			}

			if (state == VAL_MODE) {
				if (*p == '"' && first_val_char) {
					*p++;
					first_val_char = 0;
					in_string = 1;
				} else if (first_val_char)
					first_val_char = 0;

				if (*p == '"' && last_char != '\\' && in_string) {
					in_string = 0;
					buf[buf_i++] = strdup(valbuf);
					break;
				} else if (*p == '#' || *p == '\n'
					   || *p == '\0') {
					buf[buf_i++] = strdup(valbuf);
					break;
				} else {
					valbuf[valbuf_i++] = *p;
				}

			}

			last_char = *p;
		}
	}
	fclose(fp);
	fp = NULL;

	int i;
	for (i = 0; i < buf_i; i++) {
		printf("%s\n", buf[i]);
	}
	p->config_parser = 1;
	return copa_parse(p, buf, buf_i);
}

int copa_free(copa_parser_t * p)
{
	int i;
	for (i = 0; i < p->opts_s; i++)
		free(p->opts[i]);

/*	if (p->config_parser) {
		for (i = 0; i < p->arguments_s; i++)
			free(p->arguments[i]);
		for (i = 0; i < p->unknown_s; i++)
			free(p->unknown[i]);
	}
*/
	free(p);
	return 0;
}
