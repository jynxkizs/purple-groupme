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

#include "GroupMeAccount.h"
#include "GroupMeLog.h"
#include "GroupMePod.h"
#include "GroupMeProtocol.h"
#include "GroupMeUpdate.h"
#include "groupme_html.h"


GroupMeAccount *
GroupMeAccountNew()
{
  GroupMeAccount *newAccount;
  newAccount = g_new0(GroupMeAccount, 1);
  newAccount->pods = g_hash_table_new_full(g_str_hash,
					   g_str_equal,
					   NULL,
					   (GDestroyNotify)GroupMePodFree);
  GroupMeAccountGenerateStoragePath(newAccount);
  return newAccount;
}

void
GroupMeAccountFree(GroupMeAccount *account)
{
  g_hash_table_destroy(account->pods);
  g_free(account->podImagesPath);
  g_free(account->storagePath);
  g_free(account->v2Host);
  g_free(account->uid);
  g_free(account->name);
  g_free(account->error);
  g_free(account);
}

void
GroupMeAccountFromHtml(GroupMeAccount *account, 
		      const gchar *html)
{
  GroupMeLogMisc("groupme", "AccountFromHtml\n");
  //groupme_html_dup_uid(html, &account->uid);
  //groupme_html_dup_error(html, &account->error);
}

gboolean
GroupMeAccountHasPod(GroupMeAccount *account, 
		    const gchar *id)
{
  return (gboolean)g_hash_table_lookup(account->pods, 
				       id);
}

GroupMePod *
GroupMeAccountGetPod(GroupMeAccount *account, 
		    const gchar *id)
{
  return g_hash_table_lookup(account->pods, 
			     id);
}

void
GroupMeAccountAddPod(GroupMeAccount *account,
		    GroupMePod *pod)
{
  g_hash_table_insert(account->pods, 
		      pod->id, 
		      pod);
  GroupMePodReadLastUpdateIndex(account, pod);
}

void
GroupMeAccountRemovePod(GroupMeAccount *account,
		       GroupMePod *pod)
{
  //I am concerned about freeing the pod because it 
  //may have outstanding requests still
  //FIX is to change callbacks to take account ids
  //and pod ids rather than the actual objects
  g_hash_table_steal(account->pods, 
		     pod->id);
  //g_hash_table_remove(account->pods, 
  //		      pod->id);
 
}

void
GroupMeAccountGenerateStoragePath(GroupMeAccount *account)
{
  account->storagePath =  
    g_strdup_printf("%s/plugins/purple-groupme", 
		    purple_user_dir());
  purple_build_dir(account->storagePath, 0700);

  account->podImagesPath =  
    g_strdup_printf("%s/pod-images", 
		    account->storagePath);
  purple_build_dir(account->podImagesPath, 0700);
}

const gchar *
GroupMeAccountHost(GroupMeAccount *account)
{
  return purple_account_get_string(account->account,
				   "host",
				   "groupme.com");
}

const gchar *
GroupMeAccountV2Host(GroupMeAccount *account)
{
  if (!account->v2Host) {
    account->v2Host = g_strdup_printf("v2.%s", 
  		        GroupMeAccountHost(account));
  }
  return account->v2Host;
}

const gchar *
GroupMeAccountLocationLinkText(GroupMeAccount *account)
{
  return purple_account_get_string(account->account,
				   "location",
				   "(*)");
}

const gchar *
GroupMeAccountPhotoLinkText(GroupMeAccount *account)
{
  return purple_account_get_string(account->account,
				   "photo",
				   "(Photo)");
}

gboolean
GroupMeAccountCheckVersion(GroupMeAccount *account)
{
  return purple_account_get_bool(account->account,
				 "checkVersion",
				 TRUE);
}

gboolean
GroupMeAccountAllowBetaVersions(GroupMeAccount *account)
{
  return purple_account_get_bool(account->account,
				 "allowBetaVersions",
				 FALSE);
}

gboolean
GroupMeAccountDebug(GroupMeAccount *account)
{
  return purple_account_get_bool(account->account,
				 "debugMsgs",
				 FALSE);
}

//GroupMePod *
//GroupMePodFromJson(gchar *json);

//GroupMePod *
//GroupMePodMembersFromHtml(gchar *html);

