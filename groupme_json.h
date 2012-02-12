/*
 * libgroupme
 *
 * libgroupme is the property of its developers.  See the COPYRIGHT file
 * for more details.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GROUPME_JSON_H
#define GROUPME_JSON_H

#ifdef GROUPME_JSON_TEST
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define getch() getchar()
#define gchar char
#define g_strndup strndup
#define g_ascii_strncasecmp strncmp
#define purple_debug_misc(x, ...) printf(__VA_ARGS__)
#define purple_debug_info(x, ...) printf(__VA_ARGS__)
#define purple_debug_warning(x, ...) printf(__VA_ARGS__)
#else
#include <glib.h>
#include <glib/gi18n.h>
#include <string.h>
#endif // GROUPME_JSON_TEST

const gchar *
json_whitespace_end(const gchar *json);

const gchar *
json_named_const_seek(const gchar *json);

int
json_named_const_equals(const gchar *json, const gchar *compare);

const gchar *
json_number_seek(const gchar *json);

gint64
json_number_value_int(const gchar *json);

gdouble
json_number_value_float(const gchar *json);

const gchar *
json_number_end(const gchar *json);

const gchar *
json_string_seek(const gchar *json);

gchar *
json_string_dup(const gchar *json);

gint64
json_string_to_int(const gchar *json);

gdouble
json_string_to_float(const gchar *json);

int
json_string_equals(const gchar *json, const gchar *compare);

const gchar *
json_string_end(const gchar *json);
  
const gchar *
json_array_seek(const gchar *json);

const gchar *
json_array_contents(const gchar *json);

const gchar *
json_array_end(const gchar *json);

const gchar *
json_object_seek(const gchar *json);

const gchar *
json_object_contents(const gchar *json);

const gchar *
json_object_end(const gchar *json);

const gchar *
json_object_field(const gchar *json, const gchar *key);

const gchar *
json_object_pair_value(const gchar *json, const gchar *key);

int
json_object_pair_value_named_const_equals(const gchar *json, 
					  const gchar *key,
					  const gchar *compare);

gint64
json_object_pair_value_int(const gchar *json, const gchar *key);

gdouble
json_object_pair_value_float(const gchar *json, const gchar *key);

int
json_object_pair_value_string_equals(const gchar *json, 
				     const gchar *key,
				     const gchar *compare);

gchar *
json_object_pair_value_string_dup(const gchar *json, const gchar *key);

const gchar *
json_token_seek(const gchar *json);

int
json_token_ends_scope(const gchar *json);

const gchar *
json_token_end(const gchar *json);


#endif // GROUPME_JSON_H
