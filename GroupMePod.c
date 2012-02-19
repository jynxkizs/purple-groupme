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
#include "groupme_json.h"

GroupMePod *
GroupMePodNew()
{
  GroupMePod *newPod;
  newPod = g_new0(GroupMePod, 1);
  return newPod;
}

void
GroupMePodFree(GroupMePod *pod)
{
  GroupMeUpdate *update;
  gchar *msg;

  g_free(pod->id);
  g_free(pod->title);
  g_free(pod->imageUrl);
  //g_free(pod->location);
  //g_hash_table_destroy(pod->members);
  while (pod->toSend) {
    msg = (gchar *)pod->toSend->data;
    pod->toSend = g_slist_remove(pod->toSend, (gpointer)msg);
    g_free(msg);
  }
  while (pod->updates) {
    update = (GroupMeUpdate *)pod->updates->data;
    pod->updates = g_list_remove(pod->updates, (gpointer)update);
    GroupMeUpdateFree(update);
  }    
  if (pod->retryPollPodTimeout) {
    purple_timeout_remove(pod->retryPollPodTimeout);
  }
  if (pod->catchupPodTimeout) {
    purple_timeout_remove(pod->catchupPodTimeout);
  }
  g_free(pod->photosPath);
  g_free(pod);
}

GroupMePod *
GroupMePodFromHtml(const gchar *html, const gchar **htmlEnd)
{
  GroupMePod *newPod;
  newPod = GroupMePodNew();

  // find the next pod and get its id
  html = groupme_html_dup_pod_id(html, &newPod->id);
  
  // get the pod imageurl (for buddy icon)
  html = groupme_html_dup_pod_imgurl(html, &newPod->imageUrl);
  
  // get the pod title (for 'alias')
  html = groupme_html_dup_pod_title(html, &newPod->title);

  if (!newPod->id ||
      !newPod->title) {
    GroupMePodFree(newPod);
    return NULL;
  }

  *htmlEnd = html;
  return newPod;
}

GroupMePod *
GroupMePodFromJson(const gchar *json, const gchar **jsonEnd)
{
  GroupMePod *newPod;
  const gchar *meta;
  gchar *tmp;

  // create a new pod
  newPod = GroupMePodNew();

  // find the pod's metadata
  meta = json_object_pair_value(json, "meta");

  // get the pod's id
  newPod->id = json_object_pair_value_string_dup(meta, "group_id");
  
  // get the pod title (for 'alias')
  tmp = json_object_pair_value_string_dup(json, "label");
  GroupMePodSetTitle(newPod, tmp);
  g_free(tmp);

  // get the pod time (for status)
  //

  if (!newPod->id ||
      !newPod->title) {
    GroupMePodFree(newPod);
    return NULL;
  }

  // get the pod img url (for buddy icon)
  tmp = json_object_pair_value_string_dup(json, "avatar_url");
  GroupMePodSetImageUrl(newPod, tmp);
  g_free(tmp);

  if (jsonEnd) {
    *jsonEnd = json_object_end(json);
  }

  return newPod;
}

//GroupMePod *
//GroupMePodMembersFromHtml(gchar *html);

void
GroupMePodSetTitle(GroupMePod *pod,
		  gchar *newTitle)
{
  if (pod->title) {
    g_free(pod->title);
  }
  pod->title = g_strdup(newTitle);
}

void
GroupMePodSetImageUrl(GroupMePod *pod,
		       gchar *newImageUrl)
{
  if (pod->imageUrl) {
    g_free(pod->imageUrl);
    pod->imageUrl = NULL;
  }
  pod->imageUrl = g_strdup(newImageUrl);
}

void
GroupMePodPrependUpdate(GroupMePod *pod, 
		       GroupMeUpdate *update)
{
  pod->updates = g_list_prepend(pod->updates, 
				(gpointer)update);
  if (pod->nextUpdateDisplayed == NULL) {
    pod->nextUpdateDisplayed = g_list_first(pod->updates);
    pod->lastUpdateId = update->index;
  }
}

void
GroupMePodAppendUpdate(GroupMePod *pod, 
		       GroupMeUpdate *update)
{
  pod->updates = g_list_append(pod->updates, 
			       (gpointer)update);
  pod->lastUpdateId = update->index;
  if (pod->nextUpdateDisplayed == NULL) {
    pod->nextUpdateDisplayed = g_list_last(pod->updates);
  }
}

GroupMeUpdate *
GroupMePodNextUpdate(GroupMePod *pod)
{
  GroupMeUpdate *nextUpdate;

  if (!pod->nextUpdateDisplayed) {
    return NULL;
  }

  nextUpdate = pod->nextUpdateDisplayed->data;
  pod->nextUpdateDisplayed = g_list_next(pod->nextUpdateDisplayed);

  return nextUpdate;
}

void
GroupMePodResetUpdateIterator(GroupMePod *pod)
{
  pod->nextUpdateDisplayed = g_list_first(pod->updates);
}

void 
GroupMePodImageFromPngData(GroupMeAccount *account,
			  GroupMePod *pod,
			  gchar *data, gsize dataLen)
{
  gchar *fileName;

  GroupMeLogInfo("groupme", 
		"PodPhotoFromPngData(%s)\n",
		pod->id);

  // save to disk
  fileName = g_strdup_printf("%s/%s.jpeg",
			     account->podImagesPath,
			     pod->id);
  purple_util_write_data_to_file_absolute(fileName, 
					  data, dataLen);
  g_free(fileName);

  // set as buddy icon
  GroupMeSetBuddyIcon(account, pod, data, dataLen);
}

void
GroupMePodGeneratePhotoPath(GroupMeAccount *account,
			   GroupMePod *pod)
{
  pod->photosPath = g_strdup_printf("%s/photos/%s", 
				    account->storagePath,
				    pod->id);
  purple_build_dir(pod->photosPath, 0700);
}

