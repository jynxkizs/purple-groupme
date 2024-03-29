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
#define BLIST_GROUP_NAME "GroupMe Chats"
#define GROUPME_MIN_MSG_LENGTH   1
#define GROUPME_MAX_MSG_LENGTH 450

// global variables
gboolean gCheckedVersion = FALSE;

// Forward Declarations
void GH_WritePodSettings(gpointer key, gpointer value, gpointer user_data);
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
void GroupMeCheckNewPods(GroupMeAccount *account);
void GroupMeUpdatePods(GroupMeAccount *account);
//void GroupMeGetPodDetails(GroupMeAccount *account, GroupMePod *pod);
void GroupMeSeedPod(GroupMeAccount *account, GroupMePod *pod);
void GroupMePodImage(GroupMeAccount *account, GroupMePod *pod);
void GroupMeRetryPollNewUpdates(GroupMeAccount *account, GroupMePod *pod);
void GroupMePollNewUpdates(GroupMeAccount *account, GroupMePod *pod);
void GroupMeUpdateDetails(GroupMeAccount *account, GroupMePod *pod, GroupMeUpdate *update);
void GroupMeUpdatePhoto(GroupMeAccount *account, GroupMePod *pod, GroupMeUpdate *update);
void GroupMePodAddMember(GroupMeAccount *account, GroupMePod *pod, gchar *member);
void GroupMeSendNextMessage(GroupMeAccount *account, GroupMePod *pod);
gboolean GroupMeSeedAccountTO(gpointer data);
gboolean GroupMeRetryPollNewUpdatesTO(gpointer data);
gboolean GroupMeCatchupPodTO(gpointer data);
void GroupMeContactHostCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMeSignInPage0CB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMeSignInPageCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMeLoginCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMeSeedAccountCB(GroupMeAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void GroupMeMaybeAddNewPod(GroupMeAccount *account, GroupMePod *newPod);
//void GroupMeGetPodDetailsCB(GroupMeAccount *account, gchar const *requestUrl, gchar *data, gsize dataLen, gpointer userData);
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
  while ((update = GroupMePodNextUpdate(pod))) {
    msgText = GroupMeUpdateDisplayText(account,
				      pod,
				      update);

    msgTime = update->timestamp;

    isFromLocalUser = purple_strequal(account->uid, 
				      update->uid);
    msgFlags = PURPLE_MESSAGE_RECV;
    msgFlags |= isFromLocalUser?PURPLE_MESSAGE_NICK:0;
    msgFlags |= update->isDelayed?PURPLE_MESSAGE_DELAYED:0;
    msgFlags |= update->isAdmin?PURPLE_MESSAGE_SYSTEM:0;
    GroupMeLogMisc("groupme", "msg:\n\t(%ld) %s (%s):%s\n",
		   update->timestamp, update->name, update->uid, update->text);

    purple_conv_im_write(conv->u.im, 
			 update->name, 
			 msgText,
			 msgFlags,
			 msgTime);

    if (update->isAdmin) {
      updatePodDetails = TRUE;
    }

    g_free(msgText);
  }

  if (updatePodDetails) {
    // GroupMeGetPodDetails(account, pod); is unimplemented
    GroupMeUpdatePods(account);
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

  // write out pod settins
  g_hash_table_foreach(account->pods,
		       GH_WritePodSettings,
		       account);

  // send logout to server
  host = GroupMeAccountHost(account);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET, 
		     host, "/signout", 
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
  g_free(account->token);
  g_free(account->utf8);
  g_free(account->authenticity_token);

  // free the account
  GroupMeAccountFree(account);
}

void
GH_WritePodSettings(gpointer key, 
		    gpointer value, 
		    gpointer user_data)
{
  GroupMePodWriteLastUpdateIndex((GroupMeAccount *)user_data,
				 (GroupMePod *)value);
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
  
  host = GroupMeAccountHost(account);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_GET, 
		      host, "/", 
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

  GroupMeLogInfo("groupme", "ContactHostCB\n");
  //GroupMeLogMisc("groupme", "%s", html);

  host = GroupMeAccountHost(account);
  groupme_post_or_get(account, 
		      GROUPME_METHOD_GET,
		      host, "/signin", 
		      NULL, GroupMeSignInPage0CB, 
		      NULL, TRUE);  
}

void
GroupMeSignInPage0CB(GroupMeAccount *account, 
		    gchar const *requestUrl, 
		    gchar *html, gsize htmlLen, 
		    gpointer userData)
{
  const gchar *host;

  GroupMeLogInfo("groupme", "SignInPage0CB\n");
  //GroupMeLogMisc("groupme", "%s", html);

  host = GroupMeAccountHost(account);
  groupme_post_or_get(account, 
		      GROUPME_METHOD_GET | 
		      GROUPME_METHOD_SSL, 
		      host, "/signin", 
		      NULL, GroupMeSignInPageCB, 
		      NULL, TRUE);  
}

void
GroupMeSignInPageCB(GroupMeAccount *account, 
		    gchar const *requestUrl, 
		    gchar *html, gsize htmlLen, 
		    gpointer userData)
{
  const gchar *host;
  gchar *postData;
  gchar *utf8;
  gchar *authToken;
  gchar *encodedUser;
  gchar *encodedPass;
  
  GroupMeLogInfo("groupme", "SignInPageCB\n");

  groupme_html_dup_utf8(html, &utf8);
  if (!utf8) {
      GroupMeLogError("groupme", "utf8 not found\n");
      purple_connection_error(account->pc, 
			      _("utf8 not found"));
      return;
  }
  account->utf8 = g_strdup(purple_url_encode(utf8));
  g_free(utf8);

  groupme_html_dup_auth_token(html, &authToken);
  if (!authToken) {
      GroupMeLogError("groupme", "authenticity_token not found\n");
      purple_connection_error(account->pc, 
			      _("authenticity_token not found"));
      return;
  }
  account->authenticity_token = g_strdup(purple_url_encode(authToken));
  g_free(authToken);

  // we're connected to the host website now
  purple_connection_update_progress(account->pc, 
				    _("Logging in..."), 
				    2, 4);
  
  // form post data
  encodedUser = g_strdup(purple_url_encode(purple_account_get_username(account->account)));
  encodedPass = g_strdup(purple_url_encode(purple_account_get_password(account->account)));
  postData = g_strdup_printf("utf8=%%E2%%9C%%93"
			     "&authenticity_token=%s"
			     "&session%%5Bphone_number%%5D=%s"
			     "&session%%5Bpassword%%5D=%s"
			     "&session%%5Bremember_me%%5D=0", 
			     //account->utf8,
			     account->authenticity_token,
			     encodedUser, 
			     encodedPass);
  g_free(encodedUser);
  g_free(encodedPass);

  // post the login data to log in
  host = GroupMeAccountHost(account);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST | 
		     GROUPME_METHOD_SSL,
		     host, "/session", 
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
				    _("Fetching chats..."),
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

void
GroupMeUpdatePods(GroupMeAccount *account)
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
		     GROUPME_METHOD_GET | 
		     GROUPME_METHOD_SSL, 
		     host, "/groups", 
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
  const gchar *json;
  gchar *token;
  gchar *chats_json;

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
  
  // pull the host's account token from the cookie
  token = g_hash_table_lookup(account->cookie_table, 
			      "token");
  if (!token) {
    GroupMeLogError("groupme", "token not found\n");
    error = _("Login Failure.  Check email address / password.");
    purple_connection_error_reason(
      account->pc, 
      PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED,
      error);
    return;
  }
  account->token = g_strdup(token);

  // we've got our account info!
  purple_connection_set_state(account->pc, 
			      PURPLE_CONNECTED);
  GroupMeLogInfo("groupme", "Connected!\n");
  purple_connection_notice(account->pc, 
			   "Connected to GroupMe!");

  // setup buddies for pods
  groupme_html_dup_chats_json(html, &chats_json);
  if (chats_json) {
    for (json = json_object_seek(json_array_contents(chats_json));
	 json != NULL;
	 json = json_object_seek(json_token_seek(json))) {

      newPod = GroupMePodFromJson(json, &json);
      GroupMeMaybeAddNewPod(account, newPod);
    }
    g_free(chats_json);
  } else {
    // only got html?
    html = purple_strcasestr(html, "id=\'previews\'");
    while ((newPod = GroupMePodFromHtml(html, &html))) {
      GroupMeMaybeAddNewPod(account, newPod);
    }
  }
}

void
GroupMeMaybeAddNewPod(GroupMeAccount *account, GroupMePod *newPod)
{
  GroupMePod *oldPod;

  if (!newPod) {
    GroupMeLogError("groupme", "bad pod data\n");
    return;
  }

  GroupMeLogInfo("groupme", 
		 "FoundPod %s (%s)\n", 
		 newPod->title,
		 newPod->id);

  if (GroupMeAccountHasPod(account, newPod->id)) {
    GroupMeLogInfo("groupme", "pod already exists, skipping.\n");
    // check if we need to update the title
    oldPod = GroupMeAccountGetPod(account, newPod->id);
    if (g_ascii_strncasecmp(newPod->title, oldPod->title, strlen(newPod->title))) {
      GroupMePodSetTitle(oldPod, newPod->title);
      GroupMeUpdateBuddy(account, oldPod);
    }
    GroupMePodFree(newPod);
    return;
  }

  GroupMeLogInfo("groupme", "adding pod.\n");
  GroupMeAddBuddy(account, newPod);
  GroupMeAccountAddPod(account, newPod);
  GroupMeDebugMsg(account, newPod, "Connected to Pod...");
  GroupMePodGeneratePhotoPath(account, newPod);
  GroupMePodImage(account, newPod);
  GroupMeSeedPod(account, newPod);
  //GroupMePollNewUpdates(account, newPod);
}

/*
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

  GroupMeUpdateBuddy(account, pod);
}
*/

void
GroupMeSeedPod(GroupMeAccount *account, GroupMePod *pod)
{ 
  RequestData *newRequest;
  const gchar *host;
  gchar *url;

  GroupMeLogInfo("groupme", 
		"SeedPod(%s)\n",
		pod->title);

  // fetch updates for this pod
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  host = GroupMeAccountV2Host(account);
  if (pod->lastUpdateId == -1) {
    url = g_strdup_printf("/groups/%s/messages?token=%s", 
			  purple_url_encode(pod->id), 
			  account->token);
  } else {
    url = g_strdup_printf("/groups/%s/messages?token=%s&since_id=%d", 
			  purple_url_encode(pod->id), 
			  account->token,
			  pod->lastUpdateId);
  }
  GroupMeLogInfo("groupme", "SeedPodUrl: %s\n", url);
  pod->pollOutstanding = TRUE;
  groupme_post_or_get(account, 
		      GROUPME_METHOD_GET | 
		      GROUPME_METHOD_SSL, 
		      host, url,
		      NULL, GroupMePollNewUpdatesCB,
		      newRequest, TRUE);
  g_free(url);
}

void
GroupMeRetryPollNewUpdates(GroupMeAccount *account,
			   GroupMePod *pod)
{
  RequestData *newRequest;
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;

  GroupMeLogMisc("groupme",
		 "Scheduling retry poll for updates (%d sec)...\n",
		 pod->retryPollPodPeriod);

  if (pod->retryPollPodTimeout) {
    purple_timeout_remove(pod->retryPollPodTimeout);
    pod->retryPollPodTimeout = 0;
  }

  pod->retryPollPodTimeout = 
    purple_timeout_add_seconds(pod->retryPollPodPeriod,
			       GroupMeRetryPollNewUpdatesTO,
			       (gpointer)newRequest);
}

gboolean 
GroupMeRetryPollNewUpdatesTO(gpointer data)
{ 
  RequestData *request;
  request = (RequestData *)data;

  GroupMeLogMisc("groupme",
		 "Retrying poll for updates...\n");

  request->pod->retryPollPodTimeout = 0;

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

  if (pod->pollOutstanding) {
    return;
  }

  //fetch updates for this pod
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  host = GroupMeAccountV2Host(account);
  if (pod->lastUpdateId == -1) {
    url = g_strdup_printf("/groups/%s/messages?token=%s", 
			  purple_url_encode(pod->id), 
			  account->token);
  } else {
    url = g_strdup_printf("/groups/%s/messages?token=%s&since_id=%d", 
			  purple_url_encode(pod->id), 
			  account->token,
			  pod->lastUpdateId);
  }
  GroupMeLogInfo("groupme", "PollNewUpdatesUrl: %s%s\n", host, url);
  pod->pollOutstanding = TRUE;
  groupme_post_or_get(account, 
		      GROUPME_METHOD_GET | 
		      GROUPME_METHOD_SSL, 
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
  const gchar *meta;
  const gchar *messages;
  //gboolean initialize;
  GList  *updates;

  request = (RequestData *)userData;
  pod = request->pod;
  pod->pollOutstanding = FALSE;
  //initialize = (pod->updates == NULL);

  g_free(request);

  GroupMeLogInfo("groupme", 
		"PollNewUpdatesCB(%s)\n",
		pod->title);
  GroupMeLogMisc("groupme", "PollNewUpdatesResponse:\n%s\n", data);

  // extend retry poll period
  if (pod->retryPollPodPeriod < 15) {
    pod->retryPollPodPeriod += 1;
  }

  // look for json result
  if (!data) {
    GroupMeLogWarn("groupme", "bad poll response, no data");
    GroupMeRetryPollNewUpdates(account, pod);
    return;
  }
  
  root = json_object_seek(data);
  if (!root) {
    GroupMeLogMisc("groupme", "no updates (no root json object)\n");
    GroupMeRetryPollNewUpdates(account, pod);
    return;
  }

  result = json_object_pair_value(root, "response");
  if (!result) {
    GroupMeLogWarn("groupme", "no response field\n");

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

  if (json_named_const_equals(result, "null")) {
    GroupMeLogWarn("groupme", "response field is null\n");

    // check if user has been removed from pod
    meta = json_object_pair_value(root, "meta");
    if (meta && (json_object_pair_value_int(meta, "code") == 404)) {
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

  result = json_object_seek(result);
  if (!result) {
    GroupMeLogWarn("groupme", "response not-object\n");
    GroupMeRetryPollNewUpdates(account, pod);
    return;
  }

  messages = json_object_pair_value(result, "messages");
  if (!messages) {
    GroupMeLogWarn("groupme", "no messages field\n");

    // unhandled response, retry
    GroupMeRetryPollNewUpdates(account, pod);
    return;
  }

  messages = json_array_seek(messages);
  if (!messages) {
    GroupMeLogWarn("groupme", "messages not-array\n");
    GroupMeRetryPollNewUpdates(account, pod);
    return;
  }

  // parse all of the updates
  updates = NULL;
  for (json = json_object_seek(json_array_contents(messages));
       json != NULL;
       json = json_object_seek(json_token_seek(json))) {
    
    newUpdate = GroupMeUpdateFromJson(json);
    if (newUpdate->index <= pod->lastUpdateId) {
      GroupMeUpdateFree(newUpdate);
      continue;
    }

    updates = g_list_prepend(updates, 
			     (gpointer)newUpdate);
    if (newUpdate->hasPhoto) {
      GroupMeUpdatePhoto(account, pod, newUpdate);
    }
  }

  // add updates (now in proper (reversed) order)
  while (updates) {
    newUpdate = (GroupMeUpdate *)updates->data;
    updates = g_list_remove(updates, (gpointer)newUpdate);
    GroupMePodAppendUpdate(pod, newUpdate);
  }    

  // display any new updates we may have gotten
  GroupMeDisplayNewUpdates(account, pod);

  // poll again for new updates
  pod->retryPollPodPeriod = 0;
  GroupMeRetryPollNewUpdates(account, pod);
}

void
GroupMePodImage(GroupMeAccount *account,
	       GroupMePod *pod)
{
  RequestData *request;
  const gchar *fullUrl;
  gchar *tmp;
  gchar *host;
  gchar *url;
  gint  method;
  
  purple_debug_info("groupme", 
		    "PodImage(%s, %s)\n",
		    pod->id, 
		    pod->imageUrl);

  if (!pod->imageUrl) {
    GroupMeLogMisc("groupme", 
		   "no imageUrl");
    return;
  }

  // extract host and relative url from ImageUrl
  fullUrl = pod->imageUrl;

  method = 0;
  if (strstr(fullUrl, "https://")) {
    method = GROUPME_METHOD_SSL;
  }

  tmp = strstr(fullUrl, "//");
  if (!tmp) {
    GroupMeLogWarn("groupme", 
		   "could not find host in fullUrl: %s\n",
		   fullUrl);
    return;
  }
  host = g_strdup(tmp+2);
  tmp = strstr(host, "/");
  if (!tmp) {
    GroupMeLogWarn("groupme", 
		   "could not find relativeUrl in fullUrl: %s\n",
		   fullUrl);
    g_free(host);
    return;
  }  
  // copy relative url
  url = g_strdup(tmp);
  // remove relative url from host
  *tmp = 0;

  GroupMeDebugMsg(account, pod, "Fetching pod image.");
  GroupMeLogInfo("groupme",
		 "fetching pod image: %s %s %s\n",
		 (method?"https://":"http://"),
		 host,
		 url);
  // fetch the image data
  request = g_new0(RequestData, 1);
  request->account = account;
  request->pod = pod;
  groupme_post_or_get(account, 
		      GROUPME_METHOD_GET |
		      method, 
		      host, url,
		      NULL, GroupMePodImageCB, 
		      request, TRUE);

  g_free(host);
  g_free(url);
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

  request = (RequestData *)userData;
  GroupMeLogInfo("groupme", 
		"UpdateDetailsCB(%s, %d)\n",
		request->pod->id, 
		request->index);

  // read details
  GroupMeUpdateDetailsFromHtml(request->update,
			      html);

  // process details
  if (request->update->photoUrl) {
    GroupMeUpdatePhoto(request->account, 
		       request->pod,
		       request->update);
  }
  g_free(request);
}

void 
GroupMeUpdatePhoto(GroupMeAccount *account, 
		   GroupMePod *pod,
		   GroupMeUpdate *update)
{
  RequestData *request;
  const gchar *fullUrl;
  gchar *tmp;
  gchar *host;
  gchar *url;
  gint  method;

  GroupMeLogInfo("groupme", 
		"UpdatePhoto(%s, %d)\n",
		pod->id, 
		update->index);

  if (!update->photoUrl) {
    GroupMeLogMisc("groupme", 
		   "no photoUrl");
    return;
  }

  // extract host and relative url from PhotoUrl
  fullUrl = update->photoUrl;

  method = 0;
  if (strstr(fullUrl, "https://")) {
    method = GROUPME_METHOD_SSL;
  }

  tmp = strstr(fullUrl, "//");
  if (!tmp) {
    GroupMeLogWarn("groupme", 
		   "could not find host in fullUrl: %s\n",
		   fullUrl);
    return;
  }
  host = g_strdup(tmp+2);
  tmp = strstr(host, "/");
  if (!tmp) {
    GroupMeLogWarn("groupme", 
		   "could not find relativeUrl in fullUrl: %s\n",
		   fullUrl);
    g_free(host);
    return;
  }  
  // copy relative url
  url = g_strdup(tmp);
  // remove relative url from host
  *tmp = 0;

  GroupMeDebugMsg(account, pod, "Fetching photo.");
  GroupMeLogInfo("groupme",
		 "fetching photo: %s %s %s\n",
		 (method?"https://":"http://"),
		 host,
		 url);
  // fetch the image data
  request = g_new0(RequestData, 1);
  request->account = account;
  request->pod = pod;
  request->update = update;
  request->index = update->index;
  groupme_post_or_get(account, 
		      GROUPME_METHOD_GET |
		      method, 
		      host, url,
		      NULL, GroupMeUpdatePhotoCB, 
		      request, TRUE);

  g_free(host);
  g_free(url);
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
  postData = g_strdup_printf("token=%s&what=%s&emails=%s",
			     account->token, 
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
GroupMePodSetNick(GroupMeAccount *account,
		  GroupMePod *pod,
		  const gchar *nick)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *unescapedName;
  gchar *encodedName;
  
  GroupMeLogInfo("groupme", 
		"PodSetNick(%s, %s)\n",
		pod->id, nick);

  // form post data
  unescapedName = purple_unescape_html(nick);
  encodedName = g_strdup(purple_url_encode(unescapedName));
  postData = g_strdup_printf("utf8=%%E2%%9C%%93"
			     "&_method=put"
			     "&authenticity_token=%s"
			     "&nickname=%s", 
			     //encodedUtf8,
			     account->authenticity_token,
			     encodedName
			     );
  g_free(encodedName);
  g_free(unescapedName);
  
  // send request (fire & forget)
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/groups/%s",
			purple_url_encode(pod->id)
			);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST | 
		     GROUPME_METHOD_SSL, 
		     host, url, 
		     postData,
		     NULL, NULL,
		     TRUE);
  g_free(url);
  g_free(postData);
}

void
GroupMePodSetName(GroupMeAccount *account,
		  GroupMePod *pod,
		  const gchar *name)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *unescapedName;
  gchar *encodedName;
  
  GroupMeLogInfo("groupme", 
		"PodSetName(%s, %s)\n",
		pod->id, name);

  // form post data
  unescapedName = purple_unescape_html(name);
  encodedName = g_strdup(purple_url_encode(unescapedName));
  postData = g_strdup_printf("utf8=%%E2%%9C%%93"
			     "&_method=put"
			     "&authenticity_token=%s"
			     "&group%%5Btopic%%5D=%s",
			     //encodedUtf8,
			     account->authenticity_token,
			     encodedName
			     );
  g_free(encodedName);
  g_free(unescapedName);
  
  // send request (fire & forget)
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/groups/%s",
			purple_url_encode(pod->id)
			);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST | 
		     GROUPME_METHOD_SSL, 
		     host, url, 
		     postData,
		     NULL, NULL,
		     TRUE);
  g_free(url);
  g_free(postData);
}

void
GroupMePodDelete(GroupMeAccount *account,
		 GroupMePod *pod)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  
  GroupMeLogInfo("groupme", 
		"PodDelete(%s)\n",
		pod->id);

  // form post data
  postData = g_strdup_printf("_method=delete"
			     "&authenticity_token=%s",
			     account->authenticity_token);

  // send request (fire & forget)
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/groups/%s/",
			purple_url_encode(pod->id)
			);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST | 
		     GROUPME_METHOD_SSL, 
		     host, url, 
		     postData,
		     NULL, NULL,
		     TRUE);
  g_free(url);
  g_free(postData);
}

void
GroupMeSendMessage(GroupMeAccount *account,
		  const char *podId,
		  const char *msg)
{
  GroupMePod *pod;
  gboolean onlyMessage;
  gint len;
  gchar *msgPart;

  pod = GroupMeAccountGetPod(account, podId);

  // if the new message is the only one in 
  // the queue, we will call SendNextMessage
  onlyMessage = (g_slist_length(pod->toSend) == 0);

  // add this message to the pod's outbox
  len = strlen(msg);
  while (len > GROUPME_MAX_MSG_LENGTH) {
    msgPart = g_strdup(msg);
    msgPart[GROUPME_MAX_MSG_LENGTH] = (gchar)0;
    pod->toSend = g_slist_append(pod->toSend, 
			       (gpointer)msgPart);
    msg += GROUPME_MAX_MSG_LENGTH;
    len = strlen(msg);
  }
  if (len >= GROUPME_MIN_MSG_LENGTH) {
    msgPart = g_strdup(msg);
    pod->toSend = g_slist_append(pod->toSend, 
			       (gpointer)msgPart);
  }

  // if the new messages are the only ones in 
  // the queue, call SendNextMessage
  if (onlyMessage) {
    GroupMeSendNextMessage(account, pod);
  }
}

void
GroupMeSendNextMessage(GroupMeAccount *account,
		       GroupMePod *pod)
{
  const gchar *host;
  gchar *url;
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
  postData = g_strdup_printf("utf8=%%E2%%9C%%93"
			     "&authenticity_token=%s"
			     "&line%%5Btext%%5D=%s"
			     "&commit=Send", 
			     //encodedUtf8,
			     account->authenticity_token,
			     encodedMsg
			     );
  g_free(encodedMsg);
  g_free(unescapedMsg);
  
  // send request (fire & forget)
  host = GroupMeAccountHost(account);
  url = g_strdup_printf("/groups/%s/lines",
			purple_url_encode(pod->id)
			);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST | 
		     GROUPME_METHOD_SSL, 
		     host, url, 
		     postData,
		     GroupMeSendNextMessageCB, pod,
		     TRUE);
  g_free(url);
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

  // if there are more messages in
  // the queue, call SendNextMessage
  if (g_slist_length(pod->toSend) > 0) {
    GroupMeSendNextMessage(account, pod);
  } else {
    // force fetch messages
    pod->retryPollPodPeriod = 0;
    GroupMeRetryPollNewUpdates(account, pod);
  }
}

/*
void
GroupMePodLeave(GroupMeAccount *account,
		GroupMePod *pod)
{
  GroupMeLogInfo("groupme", 
		"PodLeave(%s)\n",
		pod->id);
  //GroupMePodRemoveMember(account, pod, account->membership_id);
  GroupMeLogWarn("groupme", 
		"PodLeave(%s) - unimplemented\n",
		pod->id);
}

void
GroupMePodRemoveMember(GroupMeAccount *account,
		      GroupMePod *pod,
		      const gchar *membership_id)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *encodedMembershipId;
  
  GroupMeLogInfo("groupme", 
		"PodRemoveMember(%s, %s)\n",
		pod->id, uid);

  // form post data
  postData = g_strdup_printf("_method=delete"
			     "&authenticity_token=%s",
			     account->authenticity_token);

  // send request (fire & forget)
  host = GroupMeAccountHost(account);
  encodedMembershipId = g_strdup(purple_url_encode(membership_id));
  url = g_strdup_printf("/groups/%s/memberships/%s",
			purple_url_encode(pod->id),
			encodedMembershipId
			);
  g_free(encodedMembershipId);
  groupme_post_or_get(account, 
		     GROUPME_METHOD_POST | 
		     GROUPME_METHOD_SSL, 
		     host, url, 
		     postData,
		     NULL, NULL,
		     TRUE);
  g_free(url);
  g_free(postData);
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
  postData = g_strdup_printf("v=3&token=%s&podid=%s&invites=%s", 
			     account->token,
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
*/
