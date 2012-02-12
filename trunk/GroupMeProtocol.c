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

#include "prpl.h"
#include "GroupMeAccount.h"
#include "GroupMeLog.h"
#include "GroupMePod.h"
#include "GroupMeProtocol.h"
#include "GroupMeUpdate.h"
#include "groupme_connection.h"
#include "groupme_json.h"
#include "groupme_html.h"

// constants
#define BLIST_GROUP_NAME "GroupMe Pods"

// global variables
gboolean gCheckedVersion = FALSE;

// Forward Declarations
PurpleGroup *GroupMeGetBuddyGroup();
void GroupMeAddBuddy(GroupMeAccount *account, GroupMePod *pod);
void GroupMeUpdateBuddy(GroupMeAccount *account, GroupMePod *pod);
void GroupMeRemoveBuddy(GroupMeAccount *account, GroupMePod *pod);
void GH_RemoveBuddy(gpointer key, gpointer value, gpointer user_data);
void GroupMeSetBuddyIcon(GroupMeAccount *account, GroupMePod *pod, gchar *data, gsize data_len);
void GroupMeDebugMsg(GroupMeAccount *account, GroupMePod *pod, const gchar *msgText);
void GroupMeHelpMsg(GroupMeAccount *account, GroupMePod *pod, const gchar *msgText);
void GroupMeDisplayNewUpdates(GroupMeAccount *account, GroupMePod *pod);
//
void PurpleGroupMeCheckVersionCB(GroupMeAccount *account, gchar const *requestUrl, gchar *json, gsize jsonLen, gpointer userData);
void GroupMeContactHost(GroupMeAccount *account);
void GroupMeSeedAccount(GroupMeAccount *account);
void GroupMeGetPodDetails(GroupMeAccount *account, GroupMePod *pod);
void GroupMeSeedPod(GroupMeAccount *account, GroupMePod *pod);
void GroupMePodImage(GroupMeAccount *account, GroupMePod *pod);
void GroupMeRetryPollNewUpdates(GroupMeAccount *account, GroupMePod *pod);
void GroupMePollNewUpdates(GroupMeAccount *account, GroupMePod *pod);
void GroupMeUpdateDetails(GroupMeAccount *account, GroupMePod *pod, GroupMeUpdate *update);
void GroupMePodAddMember(GroupMeAccount *account, GroupMePod *pod, gchar *member);
void GroupMeSendNextMessage(GroupMeAccount *account, GroupMePod *pod);
gboolean GroupMeSeedAccountTO(gpointer data);
gboolean GroupMeRetryPollNewUpdatesTO(gpointer data);
gboolean GroupMeCatchupPodTO(gpointer data);
void GroupMeContactHostCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMeLoginCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMeSeedAccountCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMeGetPodDetailsCB(GroupMeAccount *account, gchar const *requestUrl, gchar *data, gsize dataLen, gpointer userData);
void GroupMeSeedPodCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMePodImageCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMePollNewUpdatesCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMeUpdateDetailsCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMeUpdatePhotoCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMeCreatePodCB(GroupMeAccount *account, gchar const *requestUrl, gchar *data, gsize dataLen, gpointer userData);
void GroupMeSendNextMessageCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);

// Request Callback Data
typedef struct RequestData_t {
  GroupMeAccount *account;
  GroupMePod *pod;
  GroupMeUpdate *update;
  gint index;
} RequestData;

RequestData *
RequestDataClone(RequestData *request)
{
  RequestData *newRequest;
  newRequest = g_new0(RequestData, 1);
  newRequest->account = request->account;
  newRequest->pod = request->pod;
  newRequest->update = request->update;
  newRequest->index = request->index;
  return newRequest;
}

PurpleGroup *
GroupMeGetBuddyGroup()
{
  PurpleGroup *buddyGroup;

  // check if group already exists
  buddyGroup = purple_find_group(BLIST_GROUP_NAME);
  if (buddyGroup) 
    return buddyGroup;

  if (!purple_get_blist()) {
    return NULL;
  }

  // create a GroupMe group
  GroupMeLogInfo("groupme", "Creating GroupMe group...\n");
  buddyGroup = purple_group_new(BLIST_GROUP_NAME);
  purple_blist_add_group(buddyGroup, NULL);
  purple_blist_node_set_flags(&buddyGroup->node, 
   			      PURPLE_BLIST_NODE_FLAG_NO_SAVE);
  return buddyGroup;
}

void
GroupMeAddBuddy(GroupMeAccount *account,
	       GroupMePod *pod)
{
  PurpleBuddy *buddy;
  const char *available;

  // check if buddy already exists
  buddy = purple_find_buddy(account->account, 
			    pod->id);
  if (!buddy) {
    GroupMeLogInfo("groupme", 
		  "creating buddy %s (%s)\n",
		  pod->title,
		  pod->id);
    // create buddy
    buddy = purple_buddy_new(account->account, 
			     pod->id, 
			     pod->title);
    buddy->proto_data = (void *)pod;
    purple_blist_add_buddy(buddy, NULL, 
			   GroupMeGetBuddyGroup(), NULL);
    purple_blist_node_set_flags(&buddy->node,
				PURPLE_BLIST_NODE_FLAG_NO_SAVE);
  }

  // set status as available
  available = purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE);
  if (!available) {
    purple_debug_fatal("groupme", 
  		       "No id for PURPLE_STATUS_AVAILABLE!");
    return;
  }
  purple_prpl_got_user_status(account->account, 
  			      pod->id,
  			      available, 
			      NULL);
}

void
GroupMeUpdateBuddy(GroupMeAccount *account,
		  GroupMePod *pod)
{
  PurpleBuddy *buddy;

  GroupMeLogInfo("groupme", 
		"updating buddy %s (%s)\n",
		pod->title,
		pod->id);

  // check if buddy already exists
  buddy = purple_find_buddy(account->account, 
			    pod->id);
  if (!buddy) {
    GroupMeAddBuddy(account, pod);
    return;
  }

  // update alias
  purple_blist_alias_buddy(buddy, pod->title);

  // update status
  // pod->location
  // pod->time
}

void
GroupMeRemoveBuddy(GroupMeAccount *account,
		  GroupMePod *pod)
{
  PurpleConversation *conv;
  PurpleBuddy *buddy;

  GroupMeLogInfo("groupme",
		"GroupMeRemoveBuddy(%s)",
		pod->title);

  // get conversation for buddy if exists
  conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, 
					       pod->id, 
					       account->account);
  if (conv) {
    purple_conversation_destroy(conv);
  }

  // get buddy if buddy exists
  buddy = purple_find_buddy(account->account, 
			    pod->id);
  if (!buddy) {
    GroupMeLogInfo("groupme", 
		  "no buddy %s (%s)\n",
		  pod->title,
		  pod->id);
    return;
  }

  // remove buddy
  buddy->proto_data = (void *)NULL;
  purple_blist_remove_buddy(buddy);
}

void
GH_RemoveBuddy(gpointer key, 
	       gpointer value, 
	       gpointer user_data)
{
  GroupMeRemoveBuddy((GroupMeAccount *)user_data,
		    (GroupMePod *)value);
}

void
GroupMeSetBuddyIcon(GroupMeAccount *account,
		   GroupMePod *pod,
		   gchar *data, gsize data_len)
{
  data = g_memdup(data, data_len);
  purple_buddy_icon_new(account->account, 
			pod->id, 
			data, data_len, 
			NULL);
}

void
GroupMeDebugMsg(GroupMeAccount *account,
	       GroupMePod *pod,
	       const gchar *msgText)
{
  PurpleConversation *conv;
  PurpleMessageFlags msgFlags;  

  if (!GroupMeAccountDebug(account)) {
    return;
  }

  // find conversation for pod
  conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, 
					       pod->id, 
					       account->account);
  if (!conv) {
    conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, 
				   account->account, 
				   pod->id);
    purple_conversation_set_title(conv, pod->title);
  }

  msgFlags = PURPLE_MESSAGE_ERROR; // SYSTEM, NOTIFY, RAW
  purple_conv_im_write(conv->u.im, 
		       "purple-groupme", 
		       msgText,
		       msgFlags,
		       time(NULL));
}

void
GroupMeHintMsg(GroupMeAccount *account,
	      GroupMePod *pod,
	      const gchar *msgText)
{
  PurpleConversation *conv;
  PurpleMessageFlags msgFlags;  

  // find conversation for pod
  conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, 
					       pod->id, 
					       account->account);
  if (!conv) {
    conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, 
				   account->account, 
				   pod->id);
    purple_conversation_set_title(conv, pod->title);
  }

  msgFlags = PURPLE_MESSAGE_SYSTEM | PURPLE_MESSAGE_RAW;
  purple_conv_im_write(conv->u.im, 
		       "purple-groupme", 
		       msgText,
		       msgFlags,
		       time(NULL));
}

void
GroupMeDisplayNewUpdates(GroupMeAccount *account,
			GroupMePod *pod)
{
  PurpleConversation *conv;
  RequestData *newRequest;
  GroupMeUpdate *update;
  gchar *msgText;
  time_t msgTime;
  PurpleMessageFlags msgFlags;  
  gboolean isFromLocalUser;
  gboolean updatePodDetails;

  // find conversation for pod
  conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM, 
					       pod->id, 
					       account->account);
  if (!conv) {
    conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, 
				   account->account, 
				   pod->id);
    purple_conversation_set_title(conv, pod->title);
  }

  updatePodDetails = FALSE;
  while (GroupMePodHasUpdate(pod, pod->nextUpdateDisplayed)) {
    update = GroupMePodGetUpdate(pod, pod->nextUpdateDisplayed);
    msgText = GroupMeUpdateDisplayText(account,
				      pod,
				      update);

    msgTime = update->timestamp;

    isFromLocalUser = purple_strequal(account->uid, 
				      update->uid);
    msgFlags = PURPLE_MESSAGE_RECV;
    msgFlags |= isFromLocalUser?PURPLE_MESSAGE_NICK:0;
    msgFlags |= update->isDelayed?PURPLE_MESSAGE_DELAYED:0;
    GroupMeLogMisc("groupme", "msg: %s (%s):\n\t%s\n",
		  update->name, update->uid, update->text);

    purple_conv_im_write(conv->u.im, 
			 update->name, 
			 msgText,
			 msgFlags,
			 msgTime);

    if (update->isAdmin) {
      updatePodDetails = TRUE;
    }

    pod->nextUpdateDisplayed = update->index + 1;

    g_free(msgText);
  }

  if (updatePodDetails) {
    GroupMeGetPodDetails(account, pod);
  }

  if (pod->nextUpdateDisplayed < pod->lastUpdateReceived) {
    GroupMeDebugMsg(account, pod, "Recieved updates out of order.");
    // we have dropped updates or received them out of order  
    // wait 3 seconds, and if we are still missing updates
    // we will fetch them actively
    if (!pod->catchupPodTimeout) {
      GroupMeDebugMsg(account, pod, "Waiting for missing updates.");
      newRequest = g_new0(RequestData, 1);
      newRequest->account = account;
      newRequest->pod = pod;
      pod->catchupPodTimeout = 
	purple_timeout_add_seconds(3,
				   GroupMeCatchupPodTO,
				   (gpointer)newRequest);
    }
  }
}

void
PurpleGroupMeCheckVersionAutomatically(GroupMeAccount *account)
{
  GroupMeLogInfo("groupme", "CheckVersionAutomatically\n");
 
  if (gCheckedVersion == TRUE) {
    return;
  }
  if (GroupMeAccountCheckVersion(account) != TRUE) {
    return;
  }

  // get version data
  gCheckedVersion = TRUE;
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET, 
		     "purple-groupme.googlecode.com", 
		     "/svn/trunk/VERSION",
		     NULL, PurpleGroupMeCheckVersionCB, 
		     (gpointer)TRUE, FALSE);
}

void
PurpleGroupMeCheckVersion(GroupMeAccount *account)
{
  GroupMeLogInfo("groupme", "CheckVersion\n");
 
  // get version data
  gCheckedVersion = TRUE;
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET, 
		     "purple-groupme.googlecode.com", 
		     "/svn/trunk/VERSION",
		     NULL, PurpleGroupMeCheckVersionCB, 
		     (gpointer)FALSE, FALSE);
}

void
PurpleGroupMeCheckVersionCB(GroupMeAccount *account, 
			   gchar const *requestUrl, 
			   gchar *data, gsize dataLen, 
			   gpointer userData)
{
  const gchar *json;
  const gchar *changeJson;
  gchar *text;
  gchar *tmp;
  gchar *status;
  gchar *change;
  gboolean allowBeta;
  gboolean isBeta;
  gboolean current;
  gint major, minor, revision;
  gboolean automaticCheck;

  GroupMeLogInfo("groupme", "CheckVersionCB\n");
  automaticCheck = (gboolean)userData;

  json = data;
  json = json_object_seek(json);
  if (!json) {
    GroupMeLogWarn("groupme", "no root object\n");
    return;
  }

  json = json_object_pair_value(json, "versions");
  if (!json) {
    GroupMeLogWarn("groupme", "no versions field\n");
    return;
  }

  json = json_array_seek(json);
  if (!json) {
    GroupMeLogWarn("groupme", "version not-array\n");
    return;
  }

  // parse all of the updates
  current = TRUE;
  text = g_strdup_printf("Download <a href=\"http://code.google.com/p/purple-groupme/downloads/list\">here.</a>");
  GroupMeLogInfo("groupme", "current version %d.%d.%d\n", 
		GROUPME_PLUGIN_VERSION_MAJOR,
		GROUPME_PLUGIN_VERSION_MINOR,
		GROUPME_PLUGIN_VERSION_REVISION);
  allowBeta = GroupMeAccountAllowBetaVersions(account);
  for (json = json_object_seek(json_array_contents(json));
       json != NULL;
       json = json_object_seek(json_token_seek(json))) {
    major = json_object_pair_value_int(json, "major");
    minor = json_object_pair_value_int(json, "minor");
    revision = json_object_pair_value_int(json, "revision");
    isBeta = json_object_pair_value_string_equals(json, "status", "beta");
    GroupMeLogInfo("groupme", "checking version %d.%d.%d\n", 
		  major, minor, revision);
    if ((!isBeta || allowBeta) &&
	((major > GROUPME_PLUGIN_VERSION_MAJOR) ||
	 ((major == GROUPME_PLUGIN_VERSION_MAJOR) &&
	  ((minor > GROUPME_PLUGIN_VERSION_MINOR) ||
	   ((minor == GROUPME_PLUGIN_VERSION_MINOR) &&
	    ((revision > GROUPME_PLUGIN_VERSION_REVISION))))))) {
      GroupMeLogMisc("groupme", "new!\n");
      status = json_object_pair_value_string_dup(json, "status");
      current = FALSE;
      tmp = g_strdup_printf("%s<br/><br/><i>Version %d.%d.%d (%s):</i>", text, 
			    major, minor, revision, status);
      g_free(status);
      g_free(text);
      text = tmp;
      changeJson = json_object_pair_value(json, "changes");
      for (changeJson = json_string_seek(json_array_contents(changeJson));
	   changeJson != NULL;
	   changeJson = json_string_seek(json_token_seek(changeJson))) {
	change = json_string_dup(changeJson);
	GroupMeLogMisc("groupme", "change: %s\n", change);
	tmp = g_strdup_printf("%s<br/>%s", text, change);
	g_free(change);
	g_free(text);
	text = tmp;
      }	
    }
  }
  
  // display popup informing user of update
  if (current == FALSE) {
    purple_notify_formatted(groupmePlugin, 
			    "purple-groupme Update",
			    "A new version of purple-groupme is available.", 
			    (automaticCheck==FALSE)?"":
			    "\n (automatic update check can be disabled in Account Settings)", 
			    text,
			    NULL, NULL);
  } else if (automaticCheck == FALSE) {
    purple_notify_message(groupmePlugin, 
			  PURPLE_NOTIFY_MSG_INFO, 
			  "purple-groupme is up-to-date",
			  "Your purple-groupme plugin is up-to-date.", 
			  NULL, NULL, NULL);
  }

  g_free(text);
}

void
GroupMeConnect(PurpleAccount *pa)
{
  PurpleConnection *pc;
  GroupMeAccount *account;
  
  GroupMeLogInfo("groupme", "Connecting\n");
  pc = purple_account_get_connection(pa);
  purple_connection_set_state(pc, PURPLE_CONNECTING);

  // create a new account
  account = GroupMeAccountNew();
  account->account = pa;
  account->pc = pc;
  account->cookie_table = 
    g_hash_table_new_full(g_str_hash, 
			  g_str_equal, 
			  g_free, 
			  g_free);
  account->hostname_ip_cache = 
    g_hash_table_new_full(g_str_hash,
			  g_str_equal,
			  g_free, 
			  g_free);
  pa->gc->proto_data = account;

  // check if there is a new version of purple-groupme
  PurpleGroupMeCheckVersionAutomatically(account);

  // start connection process
  GroupMeContactHost(account);
}

void
GroupMeDisconnect(GroupMeAccount *account)
{
  PurpleDnsQueryData *dns_query;
  const gchar *host;

  GroupMeLogInfo("groupme", 
		"disabling account %s\n",
		account->name);

  // send logout to server
  host = GroupMeAccountHost(account);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET, 
		     host, "/logout", 
		     NULL, NULL, 
		     NULL, FALSE);

  // remove conversations
  g_hash_table_foreach(account->pods,
		       GH_RemoveBuddy,
		       account);

  // remove account
  purple_blist_remove_account(account->account);

  // destroy incomplete connections
  GroupMeLogMisc("groupme", 
		"destroying %d incomplete connections\n",
		g_slist_length(account->conns));
  while (account->conns != NULL)
    groupme_connection_destroy(account->conns->data);
  
  // cancel outstanding dns queries
  while (account->dns_queries != NULL) {
    dns_query = account->dns_queries->data;
    GroupMeLogMisc("groupme", 
		  "canceling dns query for %s\n",
		  purple_dnsquery_get_host(dns_query));
    account->dns_queries = g_slist_remove(account->dns_queries, 
					  dns_query);
    purple_dnsquery_destroy(dns_query);
  }

  // cancel timeouts
  if (account->seedTimeout) {
    purple_timeout_remove(account->seedTimeout);
  }

  // cleanup
  g_hash_table_destroy(account->cookie_table);
  g_hash_table_destroy(account->hostname_ip_cache);
  g_free(account->_xsrf);

  // free the account
  GroupMeAccountFree(account);
}

void
GroupMeContactHost(GroupMeAccount *account)
{
  const gchar *host;

  GroupMeLogInfo("groupme", "ContactHost\n");
  purple_connection_update_progress(account->pc, 
				    _("Contacting host..."), 
				    1, 4);
 
  // get login page
  host = GroupMeAccountHost(account);
  if (host == NULL || host[0] == '\0') {
    purple_connection_error(account->pc, _("Host not set"));
    return;
  }
  
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET | 
		     GROUPME_METHOD_SSL, 
		     host, "/login", 
		     NULL, GroupMeContactHostCB, 
		     NULL, TRUE);  
}

void
GroupMeContactHostCB(GroupMeAccount *account, 
		    gchar const *requestUrl, 
		    gchar *html, gsize htmlLen, 
		    gpointer userData)
{
  const gchar *host;
  gchar *_xsrf;
  gchar *postData;
  gchar *encodedUser;
  gchar *encodedPass;
  
  GroupMeLogInfo("groupme", "ContactHostCB\n");

  // pull the host's GroupMe account id from the cookie
  _xsrf = g_hash_table_lookup(account->cookie_table, 
			      "_xsrf");
  if (!_xsrf) {
      GroupMeLogError("groupme", "_xsrf not found\n");
      purple_connection_error(account->pc, 
			      _("_xsrf not found"));
      return;
  }
  account->_xsrf = g_strdup(_xsrf);

  // we're connected to the host website now
  purple_connection_update_progress(account->pc, 
				    _("Logging in..."), 
				    2, 4);
  
  // form post data
  encodedUser = g_strdup(purple_url_encode(purple_account_get_username(account->account)));
  encodedPass = g_strdup(purple_url_encode(purple_account_get_password(account->account)));
  postData = g_strdup_printf("_xsrf=%s&ident=%s&password=%s", 
			     account->_xsrf,
			     encodedUser, 
			     encodedPass);
  g_free(encodedUser);
  g_free(encodedPass);

  // post the login data to log in
  host = GroupMeAccountHost(account);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST | 
		     GROUPME_METHOD_SSL,
		     host, "/login", 
		     postData, GroupMeLoginCB, 
		     NULL, TRUE);
  g_free(postData);
}

void
GroupMeLoginCB(GroupMeAccount *account, 
	      gchar const *requestUrl, 
	      gchar *html, gsize htmlLen, 
	      gpointer userData)
{
  GroupMeLogInfo("groupme", "LoginCB\n");
	
  // we're logged into the website now
  purple_connection_update_progress(account->pc, 
				    _("Fetching pods..."),
				    3, 4);
  
  //load the homepage to grab the pod info
  GroupMeSeedAccount(account);
  // check again periodically for new pods
  account->seedTimeout = purple_timeout_add_seconds(60,
						    GroupMeSeedAccountTO,
						    (gpointer)account);
}

void
GroupMeCheckNewPods(GroupMeAccount *account)
{
  GroupMeSeedAccount(account);
}

gboolean GroupMeSeedAccountTO(gpointer data)
{
  GroupMeSeedAccount((GroupMeAccount *)data);
  return TRUE;
}

void
GroupMeSeedAccount(GroupMeAccount *account)
{
  const gchar *host;
  GroupMeLogInfo("groupme", "GroupMeSeedAccount\n");
	
  //load the homepage to grab the pod info
  host = GroupMeAccountHost(account);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET, 
		     host, "/pods", 
		     NULL, GroupMeSeedAccountCB, 
		     NULL, TRUE);
}


void
GroupMeSeedAccountCB(GroupMeAccount *account, 
		    gchar const *requestUrl, 
		    gchar *data, gsize dataLen, 
		    gpointer userData)
{
  GroupMePod *newPod;
  const gchar *html;
  const gchar *error;

  GroupMeLogInfo("groupme", 
		"SeedAccountCB(%s)\n",
		account->name);

  html = data;
  if (!html) {
    GroupMeLogError("groupme", "no seedaccount data\n");
    purple_connection_error(account->pc, 
			    _("No account info"));
    return;
  }
  
  // we've got our account info & pods!
  GroupMeLogInfo("groupme", "Connected!\n");
  purple_connection_notice(account->pc, 
			   "Connected to GroupMe!");

  GroupMeAccountFromHtml(account, html);
  if (account->uid) {
    purple_connection_set_state(account->pc, 
				PURPLE_CONNECTED);
  } else {
    error = account->error;
    if (!error) {
      error = _("Login Failure.  Check email address / password.");
    }
    purple_connection_error_reason(
	account->pc, 
	PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED,
	error);
    return;
  }

  // setup buddies for pods
  while ((newPod = GroupMePodFromHtml(html, &html))) {
    GroupMeLogInfo("groupme", 
		  "FoundPod %s (%s)\n", 
		  newPod->title,
		  newPod->id);
    if (GroupMeAccountHasPod(account, newPod->id)) {
      GroupMeLogInfo("groupme", "pod already exists, skipping.\n");
      GroupMePodFree(newPod);
    } else {
      GroupMeLogInfo("groupme", "adding pod.\n");
      GroupMeAddBuddy(account, newPod);
      GroupMeAccountAddPod(account, newPod);
      GroupMeDebugMsg(account, newPod, "Connected to Pod...");
      GroupMePodGeneratePhotoPath(account, newPod);
      GroupMePodImage(account, newPod);
      GroupMeSeedPod(account, newPod);
      GroupMePollNewUpdates(account, newPod);
    }
  }
}

gboolean GroupMeCatchupPodTO(gpointer data)
{
  RequestData *request;
  GroupMeAccount *account;
  GroupMePod *pod;

  request = (RequestData *)data;
  account = request->account;
  pod = request->pod;
  g_free(request);
  pod->catchupPodTimeout = 0;

  if (pod->nextUpdateDisplayed < pod->lastUpdateReceived) {
    GroupMeDebugMsg(account, pod, "Giving up.");
    GroupMeSeedPod(account, pod);
  }

  return FALSE;
}

void
GroupMeGetPodDetails(GroupMeAccount *account, 
		    GroupMePod *pod)
{
  RequestData *newRequest;
  const gchar *host;
  gchar *url;
  int lastread;
  int n;

  GroupMeLogInfo("groupme", 
		"GetPodDetails(%s)\n",
		pod->title);
	
  // how many updates should we fetch to seed the pod
  if (pod->nextUpdateDisplayed == 0) {
    // this is an initial fetch
    GroupMeDebugMsg(account, pod, "Fetching recent history for chat.");
    lastread = -1;
    n = GroupMeAccountSeedFetchCount(account);
  } else {
    // we are trying to catch up on missing updates
    GroupMeDebugMsg(account, pod, "Getting pod details or missing updates.");
    lastread = pod->lastUpdateReceived;
    n = (pod->lastUpdateReceived - pod->nextUpdateDisplayed + 1);
    n += 5; // for good measure
  }

  // fetch updates for this pod
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  host = GroupMeAccountHost(account);
  //api/pods/getPod?n=50&v=3&start=128&podid=4de21d23e694aa6c05004dfd&lastread=-1
  url = g_strdup_printf("/api/pods/getPod?v=3&podid=%s&lastread=%d&n=%d", 
			purple_url_encode(pod->id), 
			lastread,
			n);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET, 
		     host, url,
		     NULL, GroupMeGetPodDetailsCB, 
		     newRequest, TRUE);
  g_free(url);
}

void 
GroupMeGetPodDetailsCB(GroupMeAccount *account, 
		      gchar const *requestUrl, 
		      gchar *data, gsize dataLen, 
		      gpointer userData)
{
  RequestData *request;
  GroupMePod *pod;
  //GroupMeUpdate *newUpdate;
  const gchar *json;
  const gchar *root;
  const gchar *result;
  gchar *what;
  gchar *where;
  gchar *when;
  gchar *ihash;

  request = (RequestData *)userData;
  pod = request->pod;
  g_free(request);

  GroupMeLogInfo("groupme", 
		"GetPodDetailsCB(%s)\n",
		pod->title);
  //GroupMeLogMisc("groupme", "%s", data);

  // look for json result
  json = data;
  if (!json) {
    GroupMeLogWarn("groupme", "bad createPod response");
    return;
  }
  
  root = json_object_seek(json);
  if (!root) {
    GroupMeLogWarn("groupme", "no root object\n");
    return;
  }

  result = json_object_pair_value(root, "result");
  if (!result) {
    GroupMeLogWarn("groupme", "no result field\n");

    // check if user has been removed from pod
    if (json_object_pair_value_string_equals(root, 
					     "error",
					     "user not authorized")) {
      GroupMeLogWarn("groupme", "user has been removed from pod\n");
      GroupMeRemoveBuddy(account, pod);
      GroupMeAccountRemovePod(account, pod);
      //GroupMePodDestroyPhotoPath(account, newPod);
      return;
    }

    return;
  }

  what = json_object_pair_value_string_dup(result, "what");
  if (what[0]) {
    GroupMePodSetTitle(pod, what);
  }
  g_free(what);

  where = json_object_pair_value_string_dup(result, "where");
  if (where[0]) {
    //GroupMePodSetLocation(pod, where);
  }
  g_free(where);

  when = json_object_pair_value_string_dup(result, "when");
  if (when[0]) {
    //GroupMePodSetTime(pod, when);
  }
  g_free(when);

  ihash = json_object_pair_value_string_dup(result, "ihash");
  if (ihash[0] && g_ascii_strncasecmp(ihash, pod->ihash, strlen(ihash))) {
    GroupMePodSetIHash(pod, ihash);
    GroupMePodImage(account, pod);
  }
  g_free(ihash);

  GroupMeUpdateBuddy(account, pod);
}

void
GroupMeSeedPod(GroupMeAccount *account, GroupMePod *pod)
{ 
  RequestData *newRequest;
  const gchar *host;
  gchar *url;
  int seedFetchCount;

  GroupMeLogInfo("groupme", 
		"SeedPod(%s)\n",
		pod->title);
	
  // how many updates should we fetch to seed the pod
  if (pod->nextUpdateDisplayed == 0) {
    // this is an initial fetch
    GroupMeDebugMsg(account, pod, "Fetching recent history for chat.");
    seedFetchCount = GroupMeAccountSeedFetchCount(account);
  } else {
    // we are trying to catch up on missing updates
    GroupMeDebugMsg(account, pod, "Re-fetching recent history.");
    seedFetchCount = (pod->lastUpdateReceived - pod->nextUpdateDisplayed + 1);
    seedFetchCount += 10; // for good measure
  }

  // fetch updates for this pod
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/pod/%s?n=%d", 
			purple_url_encode(pod->id), 
			seedFetchCount);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET, 
		     host, url,
		     NULL, GroupMeSeedPodCB, 
		     newRequest, TRUE);
  g_free(url);
}

void 
GroupMeSeedPodCB(GroupMeAccount *account, 
		gchar const *requestUrl, 
		gchar *data, gsize dataLen, 
		gpointer userData)
{
  RequestData *request;
  GroupMePod *pod;
  GroupMeUpdate *newUpdate;
  const gchar *html;
  gint lastIndex;

  request = (RequestData *)userData;
  pod = request->pod;
  g_free(request);

  GroupMeLogInfo("groupme", 
		"SeedPodCB(%s)\n",
		pod->title);
  //GroupMeLogMisc("groupme", "response:\n%s", data);

  html = data;
  if (!html) {
    GroupMeLogWarn("groupme", "bad seed response");
    return;
  }

  // parse results
  GroupMeDebugMsg(account, pod, "Got recent history results.");
  // maybe should search for "realmessages" flag first
  lastIndex = 0;
  while ((newUpdate = GroupMeUpdateFromHtml(html, &html))) {
    if (GroupMePodHasUpdate(pod, newUpdate->index)) {
      GroupMeUpdateFree(newUpdate);
    } else {
      newUpdate->isDelayed = TRUE;
      GroupMePodAddUpdate(pod, newUpdate);
      if (newUpdate->hasPhoto) {
	GroupMeUpdateDetails(account,
			    pod,
			    newUpdate);
      }
      if (newUpdate->index > pod->lastUpdateReceived) {
	pod->lastUpdateReceived = newUpdate->index;
      }
      lastIndex = newUpdate->index;
    }
  }

  if (pod->nextUpdateDisplayed == 0) {
    pod->nextUpdateDisplayed = lastIndex;
  }

  if (pod->nextUpdateDisplayed == 0) {
    GroupMeHintMsg(account, pod, 
		  "type /help to get a list of pod commands");
  }

  // display any new updates we may have gotten
  GroupMeDisplayNewUpdates(account, pod);
}

void
GroupMeRetryPollNewUpdates(GroupMeAccount *account,
		       GroupMePod *pod)
{
  RequestData *newRequest;
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;

  GroupMeDebugMsg(account, pod, 
		 "Scheduling retry poll for updates...");

  pod->retryPollPodTimeout = 
    purple_timeout_add_seconds(60,
			       GroupMeRetryPollNewUpdatesTO,
			       (gpointer)newRequest);
}

gboolean 
GroupMeRetryPollNewUpdatesTO(gpointer data)
{ 
  RequestData *request;
  request = (RequestData *)data;

  GroupMeDebugMsg(request->account, request->pod, 
		 "Retrying poll for updates...");

  request->pod->retryPollPodTimeout = 0;
  // Pickup lost messages
  GroupMeSeedPod(request->account, request->pod);
  // Pickup new messages
  GroupMePollNewUpdates(request->account, request->pod);

  g_free(request);
  return FALSE;
}

void
GroupMePollNewUpdates(GroupMeAccount *account,
		     GroupMePod *pod)
{
  RequestData *newRequest;
  const gchar *host;
  gchar *url;
  
  GroupMeLogInfo("groupme", 
		"PollNewUpdates(%s)\n",
		pod->title);
  
  //fetch updates for this pod
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/api/poll/newUpdates?podid=%s", 
			  purple_url_encode(pod->id));
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET, 
		     host, url,
		     NULL, GroupMePollNewUpdatesCB, 
		     newRequest, TRUE);
  g_free(url);
}


void 
GroupMePollNewUpdatesCB(GroupMeAccount *account, 
		       gchar const *requestUrl, 
		       gchar *data, gsize dataLen, 
		       gpointer userData)
{
  RequestData *request;
  GroupMePod *pod;
  GroupMeUpdate *newUpdate;
  const gchar *json;
  const gchar *root;
  const gchar *result;

  request = (RequestData *)userData;
  pod = request->pod;
  g_free(request);

  GroupMeLogInfo("groupme", 
		"PollNewUpdatesCB(%s)\n",
		pod->title);
  //GroupMeLogMisc("groupme", "response:\n%s", data);

  // look for json result
  if (!data) {
    GroupMeLogWarn("groupme", "bad poll response, no data");
    GroupMeRetryPollNewUpdates(account, pod);
    return;
  }
  
  root = json_object_seek(data);
  if (!root) {
    GroupMeLogWarn("groupme", "no root json object\n");
    GroupMeRetryPollNewUpdates(account, pod);
    return;
  }

  result = json_object_pair_value(root, "result");
  if (!result) {
    GroupMeLogWarn("groupme", "no result field\n");

    // check if user has been removed from pod
    if (json_object_pair_value_string_equals(root, 
					     "error",
					     "user not authorized")) {
      GroupMeLogWarn("groupme", "user has been removed from pod\n");
      GroupMeRemoveBuddy(account, pod);
      GroupMeAccountRemovePod(account, pod);
      //GroupMePodDestroyPhotoPath(account, newPod);
      return;
    }

    // unhandled response, retry
    GroupMeRetryPollNewUpdates(account, pod);
    return;
  }

  json = json_array_seek(result);
  if (!json) {
    GroupMeLogWarn("groupme", "result not-array\n");
    GroupMeRetryPollNewUpdates(account, pod);
    return;
  }

  // parse all of the updates
  for (json = json_object_seek(json_array_contents(json));
       json != NULL;
       json = json_object_seek(json_token_seek(json))) {
    
    newUpdate = GroupMeUpdateFromJson(json);
    if (GroupMePodHasUpdate(pod, newUpdate->index)) {
      GroupMeUpdateFree(newUpdate);
    } else {
      GroupMePodAddUpdate(pod, newUpdate);
      if (newUpdate->hasPhoto) {
	GroupMeUpdateDetails(account,
			    pod,
			    newUpdate);
      }
      if (newUpdate->index > pod->lastUpdateReceived) {
	pod->lastUpdateReceived = newUpdate->index;
      }
    }
  }
  
  // display any new updates we may have gotten
  GroupMeDisplayNewUpdates(account, pod);

  // poll again for new updates
  GroupMePollNewUpdates(account, pod);
}

void
GroupMePodImage(GroupMeAccount *account,
	       GroupMePod *pod)
{
  RequestData *newRequest;
  const gchar *host;
  
  purple_debug_info("groupme", 
		    "PodImage(%s, %s)\n",
		    pod->id, 
		    pod->imageUrl);
  
  //fetch details for this update
  //fetch details for this update
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  host = GroupMeAccountHost(account);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET, 
		     host, pod->imageUrl,
		     NULL, GroupMePodImageCB, 
		     newRequest, TRUE);
}

void 
GroupMePodImageCB(GroupMeAccount *account, 
		 gchar const *requestUrl, 
		 gchar *data, gsize dataLen, 
		 gpointer userData)
{
  RequestData *request;

  request = (RequestData *)userData;
  GroupMeLogInfo("groupme",
		"PodImageCB(%s)\n",
		request->pod->id);

  // read details
  GroupMePodImageFromPngData(request->account,
			    request->pod,
			    data, dataLen);

  g_free(request);
}

void
GroupMeUpdateDetails(GroupMeAccount *account,
		    GroupMePod *pod,
		    GroupMeUpdate *update)
{
  RequestData *newRequest;
  const gchar *host;
  gchar *url;
  
  GroupMeLogInfo("groupme", 
		"UpdateDetails(%s, %d)\n",
		pod->id, update->index);
  
  //fetch details for this update
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  newRequest->update = update;
  newRequest->index = update->index;
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/details/%s/%d",
			purple_url_encode(pod->id),
			update->index);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET, 
		     host, url, 
		     NULL, GroupMeUpdateDetailsCB, 
		     newRequest, TRUE);
  g_free(url);
}

void 
GroupMeUpdateDetailsCB(GroupMeAccount *account, 
		      const gchar *requestUrl,
		      gchar *html, gsize htmlLen, 
		      gpointer userData)
{
  RequestData *request;
  RequestData *newRequest;
  const gchar *host;

  request = (RequestData *)userData;
  GroupMeLogInfo("groupme", 
		"UpdateDetailsCB(%s, %d)\n",
		request->pod->id, 
		request->index);

  // read details
  GroupMeUpdateDetailsFromHtml(request->update,
			      html);

  // process details
  //fetch photo for this update
  if (request->update->photoUrl) {
    GroupMeDebugMsg(account, request->pod, "Fetching photo.");
    host = GroupMeAccountHost(request->account);
    newRequest = RequestDataClone(request);  
    groupme_post_or_get(request->account, 
		       GROUPME_METHOD_GET, 
		       host, request->update->photoUrl,
		       NULL, GroupMeUpdatePhotoCB, 
		       newRequest, TRUE);
  }

  g_free(request);
}

void 
GroupMeUpdatePhotoCB(GroupMeAccount *account, 
		    gchar const *requestUrl, 
		    gchar *data, gsize dataLen, 
		    gpointer userData)
{
  RequestData *request;

  request = (RequestData *)userData;
  GroupMeLogInfo("groupme",
		"UpdatePhotoCB(%s, %d)\n",
		request->pod->id, 
		request->index);

  // read details
  GroupMeDebugMsg(account, request->pod, "Got photo.");
  GroupMeUpdatePhotoFromJpegData(request->pod,
				request->update,
				data, dataLen);

  g_free(request);
}

void
GroupMeCreatePod(GroupMeAccount *account)
{
  const gchar *host;
  gchar *encodedName;
  gchar *encodedEmails;
  gchar *postData;

  GroupMeLogMisc("groupme", "CreatePod()\n");

  // form post data
  encodedName = g_strdup(purple_url_encode("New Pod"));
  encodedEmails = g_strdup(purple_url_encode("")); //kpatelTrash@gmail.com,"));
  postData = g_strdup_printf("_xsrf=%s&what=%s&emails=%s",
			     account->_xsrf, 
			     encodedName,
			     encodedEmails);
  g_free(encodedName);
  g_free(encodedEmails);
  
  // send request
  host = GroupMeAccountHost(account);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST, 
		     host, "/api/pods/createPod", 
		     postData,
		     GroupMeCreatePodCB, NULL,
		     TRUE);
  g_free(postData);
}

void 
GroupMeCreatePodCB(GroupMeAccount *account, 
		  gchar const *requestUrl, 
		  gchar *data, gsize dataLen, 
		  gpointer userData)
{ 
  GroupMePod *newPod; 
  const gchar *json;

  GroupMeLogMisc("groupme", "CreatePodCB()\n");
  //GroupMeLogMisc("groupme", "response:\n%s", data);

  // look for json result
  json = data;
  if (!json) {
    GroupMeLogWarn("groupme", "bad createPod response");
    return;
  }
  
  json = json_object_seek(json);
  if (!json) {
    GroupMeLogWarn("groupme", "no root object\n");
    return;
  }

  json = json_object_pair_value(json, "result");
  if (!json) {
    GroupMeLogWarn("groupme", "no result field\n");
    return;
  }

  // setup buddy for pod
  newPod = GroupMePodFromJson(json, &json);
  if (!newPod) {
    GroupMeLogWarn("groupme", "result is not valid pod json\n");
    return;
  }

  GroupMeLogInfo("groupme", 
		"FoundPod %s (%s)\n", 
		newPod->title,
		newPod->id);
  if (GroupMeAccountHasPod(account, newPod->id)) {
    GroupMeLogInfo("groupme", "pod already exists, skipping.\n");
    GroupMePodFree(newPod);
  } else {
    GroupMeLogInfo("groupme", "adding pod.\n");
    GroupMeAddBuddy(account, newPod);
    GroupMeAccountAddPod(account, newPod);
    GroupMeDebugMsg(account, newPod, "Connected to Pod...");
    GroupMePodGeneratePhotoPath(account, newPod);
    GroupMePodImage(account, newPod);
    GroupMeSeedPod(account, newPod);
    GroupMePollNewUpdates(account, newPod);
  }
}

void
GroupMePodSetName(GroupMeAccount *account,
		  GroupMePod *pod,
		  const gchar *name)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *encodedName;
  
  GroupMeLogInfo("groupme", 
		"PodSetName(%s, %s)\n",
		pod->id, name);
  
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/api/pods/modifyPod");
  encodedName = g_strdup(purple_url_encode(name));
  postData = g_strdup_printf("v=3&_xsrf=%s&podid=%s&what=%s",
			     account->_xsrf, 
			     pod->id,
			     encodedName);
  g_free(encodedName);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST, 
		     host, url, 
		     postData, NULL, 
		     NULL, FALSE);
  g_free(postData);
  g_free(url);
}

void
GroupMePodSetLocation(GroupMeAccount *account,
		     GroupMePod *pod,
		     const gchar *location)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *encodedLocation;
  
  GroupMeLogInfo("groupme", 
		"PodSetLocation(%s, %s)\n",
		pod->id, location);
  
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/api/pods/modifyPod");
  encodedLocation = g_strdup(purple_url_encode(location));
  postData = g_strdup_printf("v=3&_xsrf=%s&podid=%s&where=%s",
			     account->_xsrf, 
			     pod->id,
			     encodedLocation);
  g_free(encodedLocation);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST, 
		     host, url, 
		     postData, NULL, 
		     NULL, FALSE);
  g_free(postData);
  g_free(url);
}

void
GroupMePodSetAddress(GroupMeAccount *account,
		    GroupMePod *pod,
		    const gchar *address)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *encodedAddress;
  
  GroupMeLogInfo("groupme", 
		"PodSetAddress(%s, %s)\n",
		pod->id, address);
  
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/api/pods/modifyPod");
  encodedAddress = g_strdup(purple_url_encode(address));
  postData = g_strdup_printf("v=3&_xsrf=%s&podid=%s&addr=%s",
			     account->_xsrf, 
			     pod->id,
			     encodedAddress);
  g_free(encodedAddress);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST, 
		     host, url, 
		     postData, NULL, 
		     NULL, FALSE);
  g_free(postData);
  g_free(url);
}

void
GroupMePodAddMembers(GroupMeAccount *account, 
		    GroupMePod *pod, 
		    gchar *argsString)
{
  gchar **split;
  int n;

  GroupMeLogInfo("groupme", 
		"PodAddMember(%s)\n",
		pod->id);

  split = g_strsplit(argsString, ",", 0);
  for (n=0; split[n]; ++n) {
    GroupMePodAddMember(account, pod, split[n]);
  }
  g_strfreev(split);
}

void
GroupMePodAddMember(GroupMeAccount *account, 
		     GroupMePod *pod, 
		     gchar *member)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *invites;
  
  GroupMeLogInfo("groupme", 
		"PodInviteEmail(%s, %s)\n",
		pod->id, member);
  
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/api/pods/modifyPod");
  // POST data:
  // invites=[{"picked":["email"],"idents":["email"]}]
  invites = g_strdup_printf("[{\"picked\":[\"%s\"],"
			    "\"idents\":[\"%s\"]}]", 
			    member,
			    member);
  postData = g_strdup_printf("v=3&_xsrf=%s&podid=%s&invites=%s", 
			     account->_xsrf,
  			     pod->id,
  			     purple_url_encode(invites));
  g_free(invites);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST, 
		     host, url, 
		     postData, NULL, 
		     NULL, FALSE);
  g_free(postData);
  g_free(url);  
}

void
GroupMePodRemoveMembers(GroupMeAccount *account,
		    GroupMePod *pod, 
		    gchar *argsString)
{
  gchar **split;
  int n;

  GroupMeLogInfo("groupme", 
		"PodRemoveMember(%s)\n",
		pod->id);

  split = g_strsplit(argsString, ",", 0);
  for (n=0; split[n]; ++n) {
    GroupMePodRemoveMember(account, pod, split[n]);
  }
  g_strfreev(split);
}

void
GroupMePodRemoveMember(GroupMeAccount *account,
		      GroupMePod *pod,
		      const gchar *uid)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *userList;
  
  GroupMeLogInfo("groupme", 
		"PodRemoveUser(%s, %s)\n",
		pod->id, uid);
  
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/api/pods/modifyPod");
  // POST data:
  // rmusers=["4d6d4f5c78f2f240e0002086"]&podid=4de21d23e694aa6c05004dfd&v=3
  userList = g_strdup_printf("[\"%s\"]", uid);
  postData = g_strdup_printf("v=3&_xsrf=%s&podid=%s&rmusers=%s", 
			     account->_xsrf,
  			     pod->id,
  			     purple_url_encode(userList));
  g_free(userList);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST, 
		     host, url, 
		     postData, NULL, 
		     NULL, FALSE);
  g_free(postData);
  g_free(url);
}

void
GroupMePodLeave(GroupMeAccount *account,
	       GroupMePod *pod)
{
  GroupMeLogInfo("groupme", 
		"PodLeave(%s)\n",
		pod->id);
  GroupMePodRemoveMember(account, pod, account->uid);
}

void
GroupMeSendMessage(GroupMeAccount *account,
		  const char *podId,
		  const char *msg)
{
  GroupMePod *pod;
  pod = GroupMeAccountGetPod(account, podId);

  // add this message to the pod's outbox
  pod->toSend = g_slist_append(pod->toSend, 
			       (gpointer)g_strdup(msg));
  
  // if the new message is the only one in 
  // the queue, call SendNextMessage
  if (g_slist_length(pod->toSend) == 1) {
    GroupMeSendNextMessage(account, pod);
  }
}

void
GroupMeSendNextMessage(GroupMeAccount *account,
		      GroupMePod *pod)
{
  const gchar *host;
  gchar *msg;
  gchar *unescapedMsg;
  gchar *encodedMsg;
  gchar *postData;

  // do we have anything to send?
  if (!pod->toSend) 
    return;

  // get msg from head of outbox
  msg = (gchar *)pod->toSend->data;
  GroupMeLogMisc("groupme",
		"SendNextMessage(%s, %s)\n",
		pod->id, msg);

  // form post data
  unescapedMsg = purple_unescape_html(msg);
  encodedMsg = g_strdup(purple_url_encode(unescapedMsg));
  // source=pidgin doesn't seem to override source 
  postData = g_strdup_printf("_xsrf=%s&podid=%s&text=%s",
			     account->_xsrf, 
			     pod->id, 
			     encodedMsg);
  g_free(encodedMsg);
  g_free(unescapedMsg);
  
  // send request (fire & forget)
  host = GroupMeAccountHost(account);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST, 
		     host, "/api/updates/createUpdate", 
		     postData,
		     GroupMeSendNextMessageCB, pod,
		     TRUE);
  g_free(postData);
}
		  
void 
GroupMeSendNextMessageCB(GroupMeAccount *account, 
			gchar const *requestUrl, 
			gchar *data, gsize dataLen, 
			gpointer userData)
{
  GroupMePod *pod;
  gchar *msg;
  pod = (GroupMePod *)userData;

  GroupMeLogMisc("groupme",
		"SendNextMessageCB(%s)\n",
		pod->id);

  // remove this message from the queue
  msg = (gchar *)pod->toSend->data;
  pod->toSend = g_slist_remove(pod->toSend, (gpointer)msg);
  g_free(msg);

  // try to send another message
  GroupMeSendNextMessage(account, pod);
}
