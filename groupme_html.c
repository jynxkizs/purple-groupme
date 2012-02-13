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

//
// groupme_html
//
// This code extracts strings from html based on arrays
// of substrings which can be searched for to locate 
// the relevant start and end markers for the target data
//


#include <stdio.h>
#include "debug.h"
#include "util.h"
#include "groupme_html.h"

// SIGNIN PAGE
/*
<!DOCTYPE html>
<html>
<head>
<meta content="IE=Edge" http-equiv="X-UA-Compatible">
<title>
GroupMe | Log In
</title>
<meta name="csrf-param" content="authenticity_token">
<meta name="csrf-token" content="33agPkul2A2sieklIjWy1/NIFAFGomkFST/ve27jV74=">
...
<input name="utf8" type="hidden" value="&#10003;">
<input name="authenticity_token" type="hidden" value="6SHVIJ7Elk36BJj+24MXxiuTxIMGJsKb9tP1qcNcGa8=">
...
 */
// marker definitions
const gchar *srf_param_markers[] = {
  "name=\"csrf-param\"", "content=\"", NULL, "\""
};

const gchar *srf_token_markers[] = {
  "name=\"csrf-token\"", "content=\"", NULL, "\""
};

// marker definitions
const gchar *utf8_markers[] = {
  "name=\"utf8\"", "value=\"", NULL, "\""
};

const gchar *auth_token_markers[] = {
  "name=\"authenticity_token\"", "value=\"", NULL, "\""
};

// GROUPS PAGE
/*
...
<ul class='groups'>
<li class='loading'>
Loading your chats
<img alt="Spinner" src="/images/spinner.gif?1328805355" />
<script>
if (typeof window.inbox === 'undefined') window.inbox = new Inbox;
inbox.items.reset([{"avatar_url":"http:\/\/i.groupme.com\/4589bfd03506012f8aba12313920995b","label":"Happy Panda","time":1329083801,"meta":{"creator":"Kandarp Patel","creator_id":"238592","group_id":"1804895","membership_id":"4307862","phone_number":"+1 8028270842","membership_state":"active","message_id":"274469703"},"type":"group","description":"Darren: like blend of pvz and army of darkness"},{"avatar_url":"http:\/\/i.groupme.com\/8407e6c034ee012f4f4012313d140eb2","label":"Lunch @ Bea Bea's","time":1329022300,"meta":{"creator":"Gyedo Jeon","creator_id":"238483","group_id":"70443","membership_id":"329644","phone_number":"+1 8182731852","membership_state":"active","message_id":"273690711"},"type":"group","description":"Gyedo Jeon: I think she was not sure, either. :)"}])
</script>
</li>
</ul>
...
<p>Send a message to one of your GroupMe contacts. It's free and totally private.</p>
<a href='#' id='create_direct_message'>Send a Direct Message</a>
<script>
$$relationships = [{"avatar_url":"http:\/\/i.groupme.com\/a7d42a303685012fa81112313b03216d.avatar","direct_message_capable":true,"phone_number":"+1 8189133741","user_id":"238483","value":"Gyedo Jeon"},{"avatar_url":"http:\/\/i.groupme.com\/6c14b6c03506012f4f4012313d140eb2.avatar","direct_message_capable":true,"phone_number":"+1 8186364751","user_id":"238552","value":"Darren Ranalli"},{"avatar_url":"http:\/\/i.groupme.com\/144a7a503504012f4f4012313d140eb2.avatar","direct_message_capable":true,"phone_number":"+1 9546631193","user_id":"4018442","value":"Juan Carlos Serrallonga"},{"avatar_url":"\/images\/application\/groups\/avatar1.jpg","direct_message_capable":false,"phone_number":"+1 8189156650","user_id":"238568","value":"Dickinson Lo"},{"avatar_url":"\/images\/application\/groups\/avatar4.jpg","direct_message_capable":false,"phone_number":"+1 4124780083","user_id":"238579","value":"Samik Bhowal"}]
</script>
*/
// marker definitions
const gchar *error_markers[] = {
  "class=\"c1 error\"", ">", NULL, "<"
};

const gchar *uid_markers[] = {
  "class=\"userimg_small\"", "src=", "/", "/", "/", NULL, "/"
};

const gchar *chats_json_markers[] = {
  "class=\'groups\'", "inbox.items.reset(", NULL, "</script>"
};

/* GROUPS PAGE (alternate)
<li>
<a href="/groups/70443" title="view group"><img alt="8407e6c034ee012f4f4012313d140eb2" class="avatar" src="http://i.groupme.com/8407e6c034ee012f4f4012313d140eb2.avatar" />
<div class='text short'>
<h2>
<strong>Lunch @ Bea Bea's</strong>
*/
const gchar *pod_id_markers[] = {
  "<li>", "href=\"/groups/", NULL, "\""
};

const gchar *pod_imgurl_markers[] = {
  "<img", "src=\"", NULL, "\""
};

const gchar *pod_title_markers[] = {
  "<strong>", NULL, "</strong>"
};

//TODO:
//const gchar *POD_TIME_CLASS = "class=\"podtime\"";

// UPDATES PAGE
/*
  update format:
  <a name="130">
  <div class="update" style="display:none" id="template"> 
  <div class ="upic"> 
  <img class="userimg" width="50" height="50" src="/static/img/default_user_tile_80.png?v=aa6db"/> 
  </div><!-- end upic --> 
  <div class="ucontent"> 
  <div class="utitle"> 
  <noscript><a href="/details/4db7a12178f2f2377a0b2ad1/template"></noscript> 
  <div class="uname"> 
  <a class="nojs" href="/details/4db7a12178f2f2377a0b2ad1/template"></a> 
  </div> 
  <div class="uinfo"> 
  <span class="pin"> 
  <img class="uimg" width="15" height="15" src="/static/img/imageicon.png?v=8900a"/> 
  <img class="upin" width="15" height="15" src="/static/img/redpin.png?v=9eb8d"
  lat="1.0" lon=1.0" /> 
  </span> 
  <span class="utime" ct="0">just now</span> 
  </div> 
  </div> <!-- end utitle --> 
  <div class="ustatus"> 
  <span class="utext"></span> 
  </div><!-- end ustatus --> 
  </div><!-- end ucontent --> 
  </div> <!-- end update --> 
*/
const gchar *update_index_markers[] = {
  "<a name=\"", NULL, "\""
};

//const gchar *update_user_pic_markers[] = {
//  "\"upic\"", "<img ", "src=\"", NULL, "\""
//};

const gchar *update_user_id_markers[] = {
  "\"upic\"", "<img ", "src=\"/userimg/", "/", NULL, "/"
};

const gchar *update_user_name_markers[] = {
  "\"uname\"", "<a ", ">", NULL, "</a>"
};

const gchar *update_image_markers[] = {
  "\"uimg\"", NULL, ">"
};

const gchar *update_location_markers[] = {
  "\"upin\"", NULL, ">"
};

const gchar *update_latitude_markers[] = {
  "lat=\"", NULL, "\""
};

const gchar *update_longitude_markers[] = {
  "lon=", NULL, "\""
};

const gchar *update_time_markers[] = {
  "\"utime\"", "ct=\"", NULL, "\""
};

const gchar *update_text_markers[] = {
  "\"utext\"", ">", NULL, "</span>"
};


// DETAILS PAGE
/* details format:
   <div style="width:100%"><img class="detailimg rounded" src="/img/600/4db7a12178f2f2377a0b2ad1/1305868579316278"/></div> 
   <noscript> 
   <img src="http://maps.google.com/maps/api/staticmap?center=34.119494,-118.339062&size=500x300&zoom=15&markers=color%3Agreen%7Clabel%3AK%7C34.119494%2C-118.339062&sensor=false">
   </noscript> 
*/
const gchar *details_photourl_markers[] = {
  "\"detailimg", "src=\"", NULL, "\""
};

const gchar *details_mapurl_markers[] = {
  "http://maps.google.com/maps/api/staticmap", NULL, "\">"
};



// IMPLEMENTATIONS



gchar *
html_decode_dup(const gchar *html, int count)
{
  // TODO
  // decode html escaped characters 
  return g_strndup(html, count);
}

int
html_find_morsel(const gchar *html,
		 const gchar *markers[],
		 const gchar **start_out, 
		 const gchar **end_out)
{
  const gchar *start;
  const gchar *end;
  int m;

  //purple_debug_info("groupme", "html_find_morsel\n");
	
  if (!html) {
    purple_debug_info("groupme", "No html data!\n");
    return 0;
  }

  if (start_out) *start_out = NULL;
  if (end_out) *end_out = NULL;

  // find the start of the token
  m = 0;
  start = html;
  while (markers[m]) {
    start = purple_strcasestr(start, markers[m]);
    if (!start) return 0;
    purple_debug_misc("groupme", "Found marker: %s\n", markers[m]);
    start += strlen(markers[m]);
    ++m;
  }

  // find the end token
  end = purple_strcasestr(start, markers[m+1]);
  if (!end) return 0;

  // we found the morsel, return it!
  if (start_out) *start_out = start;
  if (end_out) *end_out = end;
  return 1;
}

const gchar *
html_dup_morsel(const gchar *html,
		const gchar *markers[],
		gchar **morsel)
{
  const gchar *start;
  const gchar *end;
  int success;

  //purple_debug_info("groupme", "html_dup_morsel\n");

  if (morsel) {
    *morsel = NULL;
  }

  if (!html) {
    purple_debug_info("groupme", "No html data!\n");
    return NULL;
  }

  success = html_find_morsel(html, markers, &start, &end);
  if (!success) {
    return NULL;
  }
  
  if (morsel) {
    *morsel = html_decode_dup(start, end - start);
  }

  return end;
}

const gchar *
groupme_html_dup_error(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, error_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\terror:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_dup_srf_param(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, srf_param_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tsrf_param:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_dup_srf_token(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, srf_token_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tsrf_token:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_dup_utf8(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, utf8_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tutf8:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_dup_auth_token(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, auth_token_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tauthenticity_token:%s\n", *morsel);
  }

  return token_end;
}


const gchar *
groupme_html_dup_chats_json(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, chats_json_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tchats_json:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_dup_uid(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, uid_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tuid:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_dup_pod_id(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, pod_id_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tpod id:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_dup_pod_imgurl(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, pod_imgurl_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tpod imgurl:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_dup_pod_title(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, pod_title_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tpod title:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_get_update_index(const gchar *html, gint *index)
{
  gchar *morsel;
  const gchar *token_end;

  morsel = NULL;
  token_end = html_dup_morsel(html, update_index_markers, &morsel);
  if (morsel) {
    *index = (gint)g_ascii_strtoll(morsel, NULL, 10);
    purple_debug_info("groupme", "\tupdate index:%s\n", morsel);
    g_free(morsel);
  }

  return token_end;
}

const gchar *
groupme_html_dup_update_user_id(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, update_user_id_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tupdate_user_id:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_dup_update_user_name(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, update_user_name_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tupdate_user_name:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_find_update_image(const gchar *html)
{
  const gchar *token_start;
  html_find_morsel(html, update_image_markers, &token_start, NULL);
  return token_start;
}

const gchar *
groupme_html_find_update_location(const gchar *html)
{
  const gchar *token_start;
  html_find_morsel(html, update_location_markers, &token_start, NULL);
  return token_start;
}

const gchar *
groupme_html_get_update_latitude(const gchar *html, gdouble *latitude)
{
  gchar *morsel;
  const gchar *token_end;

  morsel = NULL;
  token_end = html_dup_morsel(html, update_latitude_markers, &morsel);
  if (morsel) {
    *latitude = (gdouble)(g_ascii_strtod(morsel, NULL));
    purple_debug_info("groupme", "\tupdate latitude:%s\n", morsel);
    g_free(morsel);
  }

  return token_end;
} 

const gchar *
groupme_html_get_update_longitude(const gchar *html, gdouble *longitude)
{
  gchar *morsel;
  const gchar *token_end;

  morsel = NULL;
  token_end = html_dup_morsel(html, update_longitude_markers, &morsel);
  if (morsel) {
    *longitude = (gdouble)(g_ascii_strtod(morsel, NULL));
    purple_debug_info("groupme", "\tupdate longitude:%s\n", morsel);
    g_free(morsel);
  }

  return token_end;
} 

const gchar *
groupme_html_get_update_time(const gchar *html, time_t *utime)
{
  gchar *morsel;
  const gchar *token_end;

  morsel = NULL;
  token_end = html_dup_morsel(html, update_time_markers, &morsel);
  if (morsel) {
    *utime = (time_t)(g_ascii_strtoll(morsel, NULL, 10));
    purple_debug_info("groupme", "\tupdate time:%s\n", morsel);
    g_free(morsel);
  }

  return token_end;
} 

const gchar *
groupme_html_dup_update_text(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, update_text_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tupdate_text:%s\n", *morsel);
  }

  return token_end;
}

const gchar *
groupme_html_dup_details_photourl(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, details_photourl_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tdetails_photourl:%s\n", *morsel);
  }

  return token_end;
}


const gchar *
groupme_html_dup_details_mapurl(const gchar *html, gchar **morsel)
{
  const gchar *token_end;

  token_end = html_dup_morsel(html, details_mapurl_markers, morsel);
  if (*morsel) {
    purple_debug_info("groupme", "\tdetails_mapurl:%s\n", *morsel);
  }

  return token_end;
}
