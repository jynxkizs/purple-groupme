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

#ifndef GROUPME_ACCOUNT_H
#define GROUPME_ACCOUNT_H

#include "libgroupme.h"

struct _GroupMeAccount {
  PurpleAccount *account;
  PurpleConnection *pc;
  
  // connection info
  GSList *conns; /**< A list of all active connections */
  GSList *dns_queries;
  GHashTable *hostname_ip_cache;
  GHashTable *cookie_table;
  gchar *token;
  gchar *v2Host;

  // internals
  gchar *groupme_app;
  gchar *groupme_id;
  guint seedTimeout;

  // account data
  gchar *uid;
  gchar *name;
  gchar *error;
  GHashTable *pods;
  gchar *podImagesPath;
  gchar *storagePath;
};


GroupMeAccount *
GroupMeAccountNew();

void
GroupMeAccountFree(GroupMeAccount *account);

void
GroupMeAccountFromHtml(GroupMeAccount *account, 
		      const gchar *html);

gboolean
GroupMeAccountHasPod(GroupMeAccount *account, 
		    const gchar *id);

GroupMePod *
GroupMeAccountGetPod(GroupMeAccount *account, 
		    const gchar *id);

void
GroupMeAccountAddPod(GroupMeAccount *account,
		    GroupMePod *pod);

void
GroupMeAccountRemovePod(GroupMeAccount *account,
		       GroupMePod *pod);

void
GroupMeAccountGenerateStoragePath(GroupMeAccount *account);

const gchar *
GroupMeAccountHost(GroupMeAccount *account);

const gchar *
GroupMeAccountV2Host(GroupMeAccount *account);

int
GroupMeAccountSeedFetchCount(GroupMeAccount *account);

const gchar *
GroupMeAccountLocationLinkText(GroupMeAccount *account);

const gchar *
GroupMeAccountPhotoLinkText(GroupMeAccount *account);

gboolean
GroupMeAccountCheckVersion(GroupMeAccount *account);

gboolean
GroupMeAccountAllowBetaVersions(GroupMeAccount *account);

gboolean
GroupMeAccountDebug(GroupMeAccount *account);

#endif /* GROUPME_ACCOUNT_H */
