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

//#define GROUPME_JSON_TEST
#ifndef GROUPME_JSON_TEST
#include <glib.h>
#include <glib/gi18n.h>
#include <string.h>
#include "debug.h"
#endif // GROUPME_JSON_TEST

#include <stdio.h>
#include "groupme_json.h"

int
json_is_whitespace(const gchar *json)
{
  return json &&
         ((*json == ' ') || 
	  (*json == '\t') ||
	  (*json == '\r') ||
	  (*json == '\n') ||
	  0);
}

const gchar *
json_whitespace_end(const gchar *json)
{
  while (json_is_whitespace(json)) {
    json++;
  }
  return json;
}

int
json_is_named_const(const gchar *json)
{
  return (json &&
	  (((*json >= 'a') && (*json <= 'z')) ||
	   ((*json >= 'A') && (*json <= 'Z')) ||
	   (*json == '_')));
}

const gchar *
json_named_const_seek(const gchar *json)
{
  while (!json_token_ends_scope(json) &&
	 !json_is_named_const(json)) {
    json = json_token_seek(json);
  }
  if (!json_is_named_const(json)) {
    return NULL;
  }
  return json;
}

const gchar *
json_named_const_end(const gchar *json)
{
  while (((*json >= '0') && (*json <= '9')) ||
	 ((*json >= 'a') && (*json <= 'z')) ||
	 ((*json >= 'A') && (*json <= 'Z')) ||
	 (*json == '_')) {
    json++;
  }
  return json;
}

int
json_named_const_equals(const gchar *json, const gchar *compare)
{
  const gchar *json_end;
  int json_len;
  int compare_len;

  if (!json_is_named_const(json)) {
    return 0;
  }

  json_end = json_named_const_end(json);
  json_len = json_end - json;
  compare_len = strlen(compare);
  if (json_len != compare_len) {
    return 0;
  }

  if (g_ascii_strncasecmp(json, compare, json_len)) {
    return 0;
  }

  return 1;
}

int
json_is_number(const gchar *json)
{
  return (json &&
	  (((*json >= '0') && (*json <= '9')) ||
	   (*json == '.') || 
	   (*json == '-') || (*json == '+') ||
	   (*json == 'e') || (*json == 'E')));
}

const gchar *
json_number_seek(const gchar *json)
{
  while (!json_token_ends_scope(json) &&
	 !json_is_number(json)) {
    json = json_token_seek(json);
  }
  if (!json_is_number(json)) {
    return NULL;
  }
  return json;
}

const gchar *
json_number_end(const gchar *json)
{
  while (((*json >= '0') && (*json <= '9')) ||
	 (*json == '.') || 
	 (*json == '-') || (*json == '+') ||
	 (*json == 'e') || (*json == 'E')) {
    json++;
  }
  return json;
}

gint64
json_number_value_int(const gchar *json)
{
  if (json_is_number(json)) {
    return g_ascii_strtoll(json, NULL, 10);
  }
  return 0;
}

gdouble
json_number_value_float(const gchar *json)
{
  if (json_is_number(json)) {
    return g_ascii_strtod(json, NULL);
  }
  return (gdouble)0;
}

int
json_is_string(const gchar *json)
{
  return json && (*json == '\"');
}

const gchar *
json_string_seek(const gchar *json)
{
  while (!json_token_ends_scope(json) &&
	 !json_is_string(json)) {
    json = json_token_seek(json);
  }
  if (!json_is_string(json)) {
    return NULL;
  }
  return json;
}

gchar *
json_string_dup(const gchar *json_start)
{
  const gchar *json_end;
  gchar *json_out;
  gchar *json;
  gunichar unicode_char;
  gchar unicode_char_str[6];
  gint unicode_char_len;

  if (!json_is_string(json_start)) {
    purple_debug_misc("groupme", 
		       "json_string_dup: bad %s\n",
		       json_start);
    return g_strndup("", 0);
  }

  json_end = json_string_end(json_start);
  json_out = g_strndup(json_start + 1, 
		       json_end - json_start - 2);
  json = json_out;
  while ((json = strstr(json, "\\u")))
  {
    sscanf(json, "\\u%4x", &unicode_char);
    unicode_char_len = g_unichar_to_utf8(unicode_char, unicode_char_str);
    g_memmove(json, unicode_char_str, unicode_char_len);
    g_stpcpy(json + unicode_char_len, json + 6);
  }
  json = g_strcompress(json_out);
  g_free(json_out);
  json_out = json;

  return json_out;
}

const gchar *
json_get_string_contents(const gchar *json)
{
  return (json+1);
}

gint64
json_string_to_int(const gchar *json)
{
  json = json_get_string_contents(json);
  if (json_is_number(json)) {
    return g_ascii_strtoll(json, NULL, 10);
  }
  return 0;
}

gdouble
json_string_to_float(const gchar *json)
{
  json = json_get_string_contents(json);
  if (json_is_number(json)) {
    return g_ascii_strtod(json, NULL);
  }
  return (gdouble)0;
}

int
json_string_equals(const gchar *json, const gchar *compare)
{
  const gchar *json_end;
  int json_len;
  int compare_len;

  if (!json_is_string(json)) {
    return 0;
  }

  json_end = json_string_end(json);
  json_len = json_end - json - 2;
  compare_len = strlen(compare);
  if (json_len != compare_len) {
    return 0;
  }

  json = json_get_string_contents(json);
  if (g_ascii_strncasecmp(json, compare, json_len)) {
    return 0;
  }

  return 1;
}

const gchar *
json_string_end(const gchar *json)
{
  int slash = 0;

  if (!json_is_string(json)) {
    purple_debug_misc("groupme", 
		       "json_string_end: bad %s\n",
		       json);
    return json;
  }
 
  while (++json) {
    switch (*json) {
    case '\\':
      if (slash) 
	slash = 0;
      else
	slash = 1;
      break;
    case '\"':
      if (slash)
	slash = 0;
      else
	return json+1;
    default:
      slash = 0;
      break;
    }
  }

  // shouldn't happen
  return NULL;
}

int
json_is_array(const gchar *json)
{
  return json && (*json == '[');
}

const gchar *
json_array_seek(const gchar *json)
{
  while (!json_token_ends_scope(json) &&
	 !json_is_array(json)) {
    json = json_token_seek(json);
  }
  if (!json_is_array(json)) {
    return NULL;
  }
  return json;
}

const gchar *
json_array_contents(const gchar *json)
{
  if (*json != '[') {
    purple_debug_misc("groupme", 
		       "json_array_contents: bad %s\n",
		       json);
    return json;
  }

  return json+1;
}

const gchar *
json_array_end(const gchar *json)
{
  if (!json_is_array(json)) {
    purple_debug_misc("groupme", 
		       "json_array_end: bad %s\n",
		       json);
    return json;
  }

  json++;
  while (*json != ']') {
    json = json_token_seek(json);
  }
  
  return json+1;
}

int
json_is_object(const gchar *json)
{
  return json && (*json == '{');
}

const gchar *
json_object_seek(const gchar *json)
{
  while (!json_token_ends_scope(json) &&
	 !json_is_object(json)) {
    json = json_token_seek(json);
  }
  if (!json_is_object(json)) {
    return NULL;
  }
  return json;
}

const gchar *
json_object_contents(const gchar *json)
{
  if (!json_is_object(json)) {
    purple_debug_misc("groupme", 
		       "json_find_object_contents: bad %s\n",
		       json);
    return json;
  }

  return json+1;
}

const gchar *
json_object_end(const gchar *json)
{
  if (*json != '{') {
    purple_debug_misc("groupme", 
		       "json_find_object_end: bad %s\n",
		       json);
    return json;
  }

  json++;
  while (*json != '}') {
    json = json_token_seek(json);
  }

  return json+1;
}

const gchar *
json_object_pair(const gchar *json, const gchar *key)
{
  if (!json_is_object(json)) {
    purple_debug_misc("groupme", 
		       "json_find_object_field: bad %s\n",
		       json);
    return json;
  }
  
  ++json;
  while (*json != '}') {
    if ((*json == '\"') &&
	(g_ascii_strncasecmp(json+1, key, strlen(key))==0)) {
      return json;
    }
    if (*json == ':') {
      // eat the value
      json = json_token_seek(json);
    }
    json = json_token_seek(json);
  }
  return NULL;
}

const gchar *
json_object_pair_value(const gchar *json, const gchar *key)
{
  if (!json_is_object(json)) {
    purple_debug_misc("groupme", 
		       "json_find_object_pair_value: bad %s\n",
		       json);
    return json;
  }
  
  // find the key
  json = json_object_pair(json, key);
  if (!json) return NULL;

  // pass the key string
  json = json_token_seek(json);
  while ((*json != ':')  && 
	 (*json != ',') && 
	 (*json != '}')) {
    json = json_token_seek(json);
  }
  if (*json != ':')
    return NULL;

  json = json_token_seek(json);
  return json;
}

int
json_object_pair_value_named_const_equals(const gchar *json, const gchar *key, const gchar *compare)
{
  json = json_object_pair_value(json, key);
  return json_named_const_equals(json, compare);
}

gint64
json_object_pair_value_int(const gchar *json, const gchar *key)
{
  json = json_object_pair_value(json, key);
  return json_number_value_int(json);
}

gdouble
json_object_pair_value_float(const gchar *json, const gchar *key)
{
  json = json_object_pair_value(json, key);
  return json_number_value_float(json);
}

int
json_object_pair_value_string_equals(const gchar *json, 
				     const gchar *key,
				     const gchar *compare)
{
  json = json_object_pair_value(json, key);
  return json_string_equals(json, compare);
}

gchar *
json_object_pair_value_string_dup(const gchar *json, const gchar *key)
{
  json = json_object_pair_value(json, key);
  return json_string_dup(json);
}

const gchar *
json_token_end(const gchar *json)
{
  if (json_is_object(json))
    return json_object_end(json);

  if (json_is_array(json))
    return json_array_end(json);

  if (json_is_string(json))
    return json_string_end(json);

  if (json_is_number(json))
    return json_number_end(json);

  if (json_is_named_const(json))
    return json_named_const_end(json);

  return json+1;
}

const gchar *
json_token_seek(const gchar *json)
{
  json = json_token_end(json);
  return json_whitespace_end(json);
}

int
json_token_ends_scope(const gchar *json)
{
  return (!*json || 
	  (*json == '}') ||
	  (*json == ']'));
}

#if GROUPME_JSON_TEST
void
main()
{
  const gchar *data = 
    "{\"result\": [{\"_id\": \"32042309724780\", \"ct\": 329048209328, \"name\": \"Kandarp\", \"text\": \"This message sent from pidgin!\", \"uid\": \"328973249837\"},{\"_id\": \"32042309724780\", \"ct\": 329048209328, \"name\": \"Buggy\", \"text\": \"This message sent from a dog!\", \"uid\": \"328973249837\"}]}";

  const gchar *result;
  const gchar *msg;
  const gchar *name;
  const gchar *text;
  gchar *pod_id;
  
  pod_id = (gchar *)"buggaboo";
  purple_debug_misc("groupme", "poll_pod_cb(%s)\n",
		    pod_id);
 
  // adding faux loop so I can "break" to the end
  do {
    if (!data) {
      purple_debug_misc("groupme", "bad poll response \n");
      break;
    }

    purple_debug_info("groupme", "poll_pod_cb: %s\n", data);
    getch();
    
    // parse result
    result = json_object_seek(data);
    if (!result) {
      purple_debug_warning("groupme", "no root object \n");
      break;
    }
    result = json_object_pair_value(result, "result");
    if (!result) {
      purple_debug_warning("groupme", "no result field \n");
      break;
    }
    result = json_array_seek(result);
    if (!result) {
      purple_debug_warning("groupme", "result not-array \n");
      break;
    }
    msg = json_object_seek(json_array_contents(result));
    while (msg) {
      printf("msg: %s\n", msg);
      // get name
      name = json_object_pair_value(msg, "name");
      printf("name: %s\n", name);
      if (!name) {
	purple_debug_warning("groupme", "msg: no name \n");
	break;
      }
      name = json_string_dup(name, json_string_end(name));
      // get text
      text = json_object_pair_value(msg, "text");
      printf("text: %s\n", name);
      if (!text) {
	purple_debug_warning("groupme", "msg: no text \n");
	break;
      }
      text = json_string_dup(text, json_string_end(text));
      purple_debug_info("groupme", "msg: %s: %s\n",
			name, text);

      msg = json_object_seek(json_token_seek(msg));
      getch();
    }
  } while (0);
}
#endif // GROUPME_JSON_TEST
