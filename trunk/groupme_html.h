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

#ifndef GROUPME_HTML_H
#define GROUPME_HTML_H

// groupme_html
//
// This code extracts strings from html based on arrays
// of substrings which can be searched for to locate 
// the relevant start and end markers for the target data
//

#include <glib.h>
#include <glib/gi18n.h>
#include <string.h>

const gchar *
groupme_html_dup_error(const gchar *html, gchar **error);

const gchar *
groupme_html_dup_srf_param(const gchar *html, gchar **srf_param);

const gchar *
groupme_html_dup_srf_token(const gchar *html, gchar **srf_token);

const gchar *
groupme_html_dup_utf8(const gchar *html, gchar **utf8);

const gchar *
groupme_html_dup_auth_token(const gchar *html, gchar **auth_token);

const gchar *
groupme_html_dup_chats_json(const gchar *html, gchar **chats_json);

const gchar *
groupme_html_dup_pod_id(const gchar *html, gchar **pod_id);

const gchar *
groupme_html_dup_pod_imgurl(const gchar *html, gchar **imgurl);

const gchar *
groupme_html_dup_pod_title(const gchar *html, gchar **pod_title);

const gchar *
groupme_html_get_update_index(const gchar *html, gint *index);

const gchar *
groupme_html_dup_update_user_id(const gchar *html, gchar **id);

const gchar *
groupme_html_dup_update_user_name(const gchar *html, gchar **name);

const gchar *
groupme_html_find_update_image(const gchar *html);

const gchar *
groupme_html_find_update_location(const gchar *html);

const gchar *
groupme_html_get_update_latitude(const gchar *html, gdouble *latitude);

const gchar *
groupme_html_get_update_longitude(const gchar *html, gdouble *longitude);

const gchar *
groupme_html_get_update_time(const gchar *html, time_t *utime);

const gchar *
groupme_html_dup_update_text(const gchar *html, gchar **text);

const gchar *
groupme_html_dup_details_photourl(const gchar *html, gchar **photourl);

const gchar *
groupme_html_dup_details_mapurl(const gchar *html, gchar **mapurl);

#endif // GROUPME_HTML_H
