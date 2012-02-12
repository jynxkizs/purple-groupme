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
#include "groupme_json.h"
#include "groupme_html.h"
#include "util.h"

#ifdef _WIN32
#define FILEURI "file://file://"    // workaround for winpidgin bug
#else
#define FILEURI "file://"
#endif

GroupMeUpdate *
GroupMeUpdateNew()
{
  return g_new0(GroupMeUpdate, 1);
}

GroupMeUpdate *
GroupMeUpdateClone(GroupMeUpdate *update)
{
  GroupMeUpdate *newUpdate;
  newUpdate = g_new0(GroupMeUpdate, 1);
  newUpdate->index = update->index;
  newUpdate->name = g_strdup(update->name);
  newUpdate->text = g_strdup(update->text);
  newUpdate->uid = g_strdup(update->uid);
  newUpdate->timestamp = update->timestamp;
  newUpdate->lat = update->lat;
  newUpdate->lon = update->lon;
  newUpdate->hasPhoto = update->hasPhoto;
  newUpdate->photoUrl = NULL;
  return newUpdate;
}

void
GroupMeUpdateFree(GroupMeUpdate *update)
{
  g_free(update->name);
  g_free(update->text);
  g_free(update->uid);
  g_free(update->photoUrl);
  g_free(update);
}

// TODO: rewrite using purple_markup_find_tag
void
GroupMeUpdateStripHtmlTags(gchar *text)
{
  const gchar *tmp;
  const gchar *endTag;
  gchar *leading;
  gchar *trailing;

  trailing = leading = text;
  while (*leading) {
    if (*leading == '<') {
      endTag = leading;
      while (*endTag && (*(endTag++) != '>')) {
	// advance
      }
      // treat anchor tags specially, extracting the uri
      tmp = purple_strcasestr(leading, "href");
      if (tmp && (tmp < endTag)) {
	tmp = purple_strcasestr(tmp, "\"");
	if (tmp && (tmp < endTag)) {
	  while (*(tmp+1) && (*(tmp+1) != '\"')) {
	    *(trailing++) = *(++tmp);
	  }
	  tmp = purple_strcasestr(endTag, "</a>");
	  endTag += (tmp - endTag);
	}
      }
      leading += (endTag - leading);
    } else {
      *(trailing++) = *(leading++);
    }
  }
  *(trailing++) = *(leading++);
}

GroupMeUpdate *
GroupMeUpdateFromHtml(const gchar *html, const gchar **htmlEnd)
{
  GroupMeUpdate *newUpdate;
  const gchar *uimg;
  const gchar *uloc;
  gchar *tmpText;

  newUpdate = GroupMeUpdateNew();

  // for efficiency, we will get them in html order.
  // otherwise remove the 'html =' from each line and 

  // get index
  html = groupme_html_get_update_index(html, &newUpdate->index);

  // get uid
  groupme_html_dup_update_user_id(html, &newUpdate->uid);
  if (!newUpdate->uid) {
    GroupMeLogWarn("groupme", "update: no uid\n");
  }
  if (purple_strcasestr(newUpdate->uid, "default")) {
    // could not pull uid from image tag
    g_free(newUpdate->uid);
    newUpdate->uid = g_strndup("", 0);
  }

  // check for photo
  uimg = groupme_html_find_update_image(html);

  // check for location
  uloc = groupme_html_find_update_location(html);

  // get name
  html = groupme_html_dup_update_user_name(html, &newUpdate->name);
  if (!newUpdate->name) {
    GroupMeLogWarn("groupme", "update: no name\n");
  }

  // get ct (time)
  html = groupme_html_get_update_time(html, &newUpdate->timestamp);
  if (!newUpdate->timestamp) {
    GroupMeLogWarn("groupme", "update: no time\n");
  }

  // get text
  html = groupme_html_dup_update_text(html, &newUpdate->text);
  if (!newUpdate->text) {
    GroupMeLogWarn("groupme", "update: no text\n");
  } else {
    GroupMeUpdateStripHtmlTags(newUpdate->text);
    tmpText = purple_unescape_text(newUpdate->text);
    g_free(newUpdate->text);
    newUpdate->text = tmpText;
  }

  if (uimg && (uimg < html)) {
    newUpdate->hasPhoto = 1;
  }

  if (uloc && (uloc < html)) {
    groupme_html_get_update_latitude(uloc, &newUpdate->lat);
    groupme_html_get_update_longitude(uloc, &newUpdate->lon);
  }

  if (!newUpdate->uid ||
      !newUpdate->name ||
      !newUpdate->text ||
      !newUpdate->timestamp) {
    GroupMeUpdateFree(newUpdate);
    return NULL;
  }

  *htmlEnd = html;
  return newUpdate;
}

GroupMeUpdate *
GroupMeUpdateFromJson(const gchar *json)
{
  GroupMeUpdate *newUpdate;
  gint index;
  const gchar *name;
  const gchar *text;
  const gchar *uid;
  time_t timestamp;
  gdouble lat;
  gdouble lon;
  gboolean hasPhoto;
  gboolean isAdmin;
  gint64 ctTime;
  
  // get index
  index = json_object_pair_value_int(json, "i");

  // get name
  name = json_object_pair_value(json, "name");
  if (!name) {
    GroupMeLogWarn("groupme", "update: no name\n");
    return NULL;
  }
  
  // get text
  text = json_object_pair_value(json, "text");
  if (!text) {
    GroupMeLogWarn("groupme", "update: no text\n");
    return NULL;
  }

  // get ct (time)
  ctTime = json_object_pair_value_int(json, "ct");
  timestamp = ctTime / 1000000;
  if (!timestamp) {
    GroupMeLogWarn("groupme", "update: no ct time\n");
    return NULL;
  }

  // get uid
  uid = json_object_pair_value(json, "uid");
  if (!uid) {
    GroupMeLogWarn("groupme", "update: no uid\n");
    return NULL;
  }

  // get msg location (optional)
  lat = json_object_pair_value_float(json, "lat");
  lon = json_object_pair_value_float(json, "lon");
  
  // get has image
  hasPhoto = json_object_pair_value_named_const_equals(json, "img", "true");

  // get is admin
  isAdmin = json_object_pair_value_named_const_equals(json, "admin", "true");

  // success, return a new update
  newUpdate = GroupMeUpdateNew();
  newUpdate->index = index;
  newUpdate->name = json_string_dup(name);
  newUpdate->text = json_string_dup(text);
  newUpdate->uid = json_string_dup(uid);
  newUpdate->timestamp = timestamp;
  newUpdate->lat = lat;
  newUpdate->lon = lon;
  newUpdate->hasPhoto = hasPhoto;
  newUpdate->photoUrl = NULL;
  newUpdate->isAdmin = isAdmin;
  return newUpdate;
}

gchar *
GroupMeUpdateToJson(const GroupMeUpdate *update);


void 
GroupMeUpdateDetailsFromHtml(GroupMeUpdate *update,
			    const gchar *html)
{
  GroupMeLogInfo("groupme", 
		"UpdateDetailsFromHtml(%d)\n",
		update->index);

  // get photoUrl, if any
  if (update->photoUrl) {
    g_free(update->photoUrl);
    update->photoUrl = NULL;
  }
  groupme_html_dup_details_photourl(html, &update->photoUrl);
}

void 
GroupMeUpdatePhotoFromJpegData(GroupMePod *pod,
			      GroupMeUpdate *update,
			      gchar *data, gsize dataLen)
{
  gchar *fileName;

  GroupMeLogInfo("groupme", 
		"UpdatePhotoFromJpegData(%s,%d)\n",
		pod->id, update->index);

  fileName = g_strdup_printf("%s/%d.jpg",
			     pod->photosPath,
			     update->index
			     );
  purple_util_write_data_to_file_absolute(fileName, 
					  data, dataLen);
  g_free(fileName);
}

gchar *
GroupMeUpdateDisplayText(GroupMeAccount *account,
			GroupMePod *pod,
			GroupMeUpdate *update)
{
  const gchar *photoLinkText;
  const gchar *locLinkText;
  gchar *displayText;
  gchar *tmpText;
  gint textLength;

  //displayText = g_strdup(update->text);
  textLength = strlen(update->text);
  displayText = purple_markup_escape_text(update->text,
					  textLength);


  if (update->hasPhoto) {
    photoLinkText = GroupMeAccountPhotoLinkText(account);
    if (photoLinkText && photoLinkText[0]) {
      tmpText = g_strdup_printf("%s <a href=\"" 
				FILEURI "%s/%d"
				".jpg\">%s</a>", 
				displayText,
				pod->photosPath,
				update->index,
				photoLinkText);
      g_free(displayText);
      displayText = tmpText;
    }
  }

  if ((update->lat != 0) && (update->lon != 0)) {
    locLinkText = GroupMeAccountLocationLinkText(account);
    if (locLinkText && locLinkText[0]) {
      tmpText = g_strdup_printf("%s <a href=\"http://maps.google.com/maps/api/staticmap?center=%lf,%lf&size=640x480&zoom=12&markers=color%%3Agreen%%7Clabel%%3A%s%%7C%lf%%2C%lf&sensor=false\">%s</a>",
				displayText,
				update->lat,
				update->lon,
				update->name,
				update->lat,
				update->lon,
				locLinkText);
      g_free(displayText);
      displayText = tmpText;
    }
  }

  return displayText;
}