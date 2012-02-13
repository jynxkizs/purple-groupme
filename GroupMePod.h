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

#ifndef GROUPME_POD_H
#define GROUPME_POD_H

#include "libgroupme.h"

struct _GroupMePod {
  PurpleConversation *conv;

  gchar *id;
  gchar *title;
  gchar *imageUrl;

  // gint64 time;
  // gchar *location;
  //GHashTable *members;

  GSList *toSend;
  GList  *updates;
  GList  *nextUpdateDisplayed;
  gint   lastUpdateTime;

  guint retryPollPodTimeout;
  guint catchupPodTimeout;
  gchar *photosPath;
};

GroupMePod *
GroupMePodNew();

void
GroupMePodFree(GroupMePod *pod);

GroupMePod *
GroupMePodFromHtml(const gchar *html, const gchar **htmlEnd);

GroupMePod *
GroupMePodFromJson(const gchar *json, const gchar **jsonEnd);

//GroupMePod *
//GroupMePodMembersFromHtml(gchar *html);

void
GroupMePodSetTitle(GroupMePod *pod,
		  gchar *newTitle);

void
GroupMePodSetImageUrl(GroupMePod *pod,
		      gchar *newImageUrl);

GroupMeUpdate *
GroupMePodGetUpdate(GroupMePod *pod, 
		   gint index);

void
GroupMePodPrependUpdate(GroupMePod *pod, 
			GroupMeUpdate *update);

void
GroupMePodAppendUpdate(GroupMePod *pod, 
		       GroupMeUpdate *update);

GroupMeUpdate *
GroupMePodNextUpdate(GroupMePod *pod);

void
GroupMePodResetUpdateIterator(GroupMePod *pod);

void 
GroupMePodImageFromPngData(GroupMeAccount *account,
			  GroupMePod *pod,
			  gchar *data, gsize dataLen);

void
GroupMePodGeneratePhotoPath(GroupMeAccount *account,
			   GroupMePod *pod);

#endif /* GROUPME_UPDATE_H */
