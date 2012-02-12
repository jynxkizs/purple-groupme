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

#ifndef GROUPME_UPDATE_H
#define GROUPME_UPDATE_H

#include "libgroupme.h"

struct _GroupMeUpdate {
  gint index;
  gchar *name;
  gchar *text;
  gchar *uid;
  time_t timestamp;
  gdouble lat;
  gdouble lon;
  gboolean hasPhoto;
  gboolean isAdmin;
  gboolean isDelayed;
  gchar *photoUrl;
};

GroupMeUpdate *
GroupMeUpdateNew();

GroupMeUpdate *
GroupMeUpdateCopy(GroupMeUpdate *update);

void
GroupMeUpdateFree(GroupMeUpdate *update);

GroupMeUpdate *
GroupMeUpdateFromHtml(const gchar *html, const gchar **htmlEnd);

GroupMeUpdate *
GroupMeUpdateFromJson(const gchar *json);

gchar *
GroupMeUpdateToJson(const GroupMeUpdate *update);

void 
GroupMeUpdateDetailsFromHtml(GroupMeUpdate *update,
			    const gchar *html);

void 
GroupMeUpdatePhotoFromJpegData(GroupMePod *pod,
			      GroupMeUpdate *update,
			      gchar *data, gsize dataLen);

gchar *
GroupMeUpdateDisplayText(GroupMeAccount *account,
			GroupMePod *pod,
			GroupMeUpdate *update);

#endif /* GROUPME_UPDATE_H */
