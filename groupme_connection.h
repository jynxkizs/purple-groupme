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

#ifndef GROUPME_CONNECTION_H
#define GROUPME_CONNECTION_H

#include "libgroupme.h"

typedef void (*GroupMeProxyCallbackFunc)(GroupMeAccount *na, 
					gchar const *url,
					gchar *data, 
					gsize data_len, 
					gpointer user_data);

/*
 * This is a bitmask.
 */
typedef enum
{
	GROUPME_METHOD_GET  = 0x0001,
	GROUPME_METHOD_POST = 0x0002,
	GROUPME_METHOD_SSL  = 0x0004
} GroupMeMethod;

typedef struct _GroupMeConnection GroupMeConnection;
struct _GroupMeConnection {
	GroupMeAccount *na;
	GroupMeMethod method;
	gchar *hostname;
        gchar *url;
	GString *request;
	GroupMeProxyCallbackFunc callback;
	gpointer user_data;
	char *rx_buf;
	size_t rx_len;
	PurpleProxyConnectData *connect_data;
	PurpleSslConnection *ssl_conn;
	int fd;
	guint input_watcher;
	gboolean connection_keepalive;
	time_t request_time;
};

void groupme_connection_destroy(GroupMeConnection *okcconn);
void groupme_post_or_get(GroupMeAccount *oca, GroupMeMethod method,
		const gchar *host, const gchar *url, const gchar *postdata,
		GroupMeProxyCallbackFunc callback_func, gpointer user_data,
		gboolean keepalive);

#endif /* GROUPME_CONNECTION_H */
