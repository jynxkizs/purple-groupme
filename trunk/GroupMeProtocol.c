/*
 * libbeluga
 *
 * libbeluga is the property of its developers.  See the COPYRIGHT file
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
#include "BelugaAccount.h"
#include "BelugaLog.h"
#include "BelugaPod.h"
#include "BelugaProtocol.h"
#include "BelugaUpdate.h"
#include "beluga_connection.h"
#include "beluga_json.h"
#include "beluga_html.h"

// constants
#define BLIST_GROUP_NAME "Beluga Pods"

// global variables
gboolean gCheckedVersion = FALSE;

// Forward Declarations
PurpleGroup *BelugaGetBuddyGroup();
void BelugaAddBuddy(BelugaAccount *account, BelugaPod *pod);
void BelugaUpdateBuddy(BelugaAccount *account, BelugaPod *pod);
void BelugaRemoveBuddy(BelugaAccount *account, BelugaPod *pod);
void GH_RemoveBuddy(gpointer key, gpointer value, gpointer user_data);
void BelugaSetBuddyIcon(BelugaAccount *account, BelugaPod *pod, gchar *data, gsize data_len);
void BelugaDebugMsg(BelugaAccount *account, BelugaPod *pod, const gchar *msgText);
void BelugaHelpMsg(BelugaAccount *account, BelugaPod *pod, const gchar *msgText);
void BelugaDisplayNewUpdates(BelugaAccount *account, BelugaPod *pod);
//
void PurpleBelugaCheckVersionCB(BelugaAccount *account, gchar const *requestUrl, gchar *json, gsize jsonLen, gpointer userData);
void BelugaContactHost(BelugaAccount *account);
void BelugaSeedAccount(BelugaAccount *account);
void BelugaGetPodDetails(BelugaAccount *account, BelugaPod *pod);
void BelugaSeedPod(BelugaAccount *account, BelugaPod *pod);
void BelugaPodImage(BelugaAccount *account, BelugaPod *pod);
void BelugaRetryPollNewUpdates(BelugaAccount *account, BelugaPod *pod);
void BelugaPollNewUpdates(BelugaAccount *account, BelugaPod *pod);
void BelugaUpdateDetails(BelugaAccount *account, BelugaPod *pod, BelugaUpdate *update);
void BelugaPodAddMember(BelugaAccount *account, BelugaPod *pod, gchar *member);
void BelugaSendNextMessage(BelugaAccount *account, BelugaPod *pod);
gboolean BelugaSeedAccountTO(gpointer data);
gboolean BelugaRetryPollNewUpdatesTO(gpointer data);
gboolean BelugaCatchupPodTO(gpointer data);
void BelugaContactHostCB(BelugaAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void BelugaLoginCB(BelugaAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void BelugaSeedAccountCB(BelugaAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void BelugaGetPodDetailsCB(BelugaAccount *account, gchar const *requestUrl, gchar *data, gsize dataLen, gpointer userData);
void BelugaSeedPodCB(BelugaAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void BelugaPodImageCB(BelugaAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void BelugaPollNewUpdatesCB(BelugaAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void BelugaUpdateDetailsCB(BelugaAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void BelugaUpdatePhotoCB(BelugaAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);
void BelugaCreatePodCB(BelugaAccount *account, gchar const *requestUrl, gchar *data, gsize dataLen, gpointer userData);
void BelugaSendNextMessageCB(BelugaAccount *account, gchar const *requestUrl, gchar *html, gsize htmlLen, gpointer userData);

// Request Callback Data
typedef struct RequestData_t {
  BelugaAccount *account;
  BelugaPod *pod;
  BelugaUpdate *update;
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
BelugaGetBuddyGroup()
{
  PurpleGroup *buddyGroup;

  // check if group already exists
  buddyGroup = purple_find_group(BLIST_GROUP_NAME);
  if (buddyGroup) 
    return buddyGroup;

  if (!purple_get_blist()) {
    return NULL;
  }

  // create a Beluga group
  BelugaLogInfo("beluga", "Creating Beluga group...\n");
  buddyGroup = purple_group_new(BLIST_GROUP_NAME);
  purple_blist_add_group(buddyGroup, NULL);
  purple_blist_node_set_flags(&buddyGroup->node, 
   			      PURPLE_BLIST_NODE_FLAG_NO_SAVE);
  return buddyGroup;
}

void
BelugaAddBuddy(BelugaAccount *account,
	       BelugaPod *pod)
{
  PurpleBuddy *buddy;
  const char *available;

  // check if buddy already exists
  buddy = purple_find_buddy(account->account, 
			    pod->id);
  if (!buddy) {
    BelugaLogInfo("beluga", 
		  "creating buddy %s (%s)\n",
		  pod->title,
		  pod->id);
    // create buddy
    buddy = purple_buddy_new(account->account, 
			     pod->id, 
			     pod->title);
    buddy->proto_data = (void *)pod;
    purple_blist_add_buddy(buddy, NULL, 
			   BelugaGetBuddyGroup(), NULL);
    purple_blist_node_set_flags(&buddy->node,
				PURPLE_BLIST_NODE_FLAG_NO_SAVE);
  }

  // set status as available
  available = purple_primitive_get_id_from_type(PURPLE_STATUS_AVAILABLE);
  if (!available) {
    purple_debug_fatal("beluga", 
  		       "No id for PURPLE_STATUS_AVAILABLE!");
    return;
  }
  purple_prpl_got_user_status(account->account, 
  			      pod->id,
  			      available, 
			      NULL);
}

void
BelugaUpdateBuddy(BelugaAccount *account,
		  BelugaPod *pod)
{
  PurpleBuddy *buddy;

  BelugaLogInfo("beluga", 
		"updating buddy %s (%s)\n",
		pod->title,
		pod->id);

  // check if buddy already exists
  buddy = purple_find_buddy(account->account, 
			    pod->id);
  if (!buddy) {
    BelugaAddBuddy(account, pod);
    return;
  }

  // update alias
  purple_blist_alias_buddy(buddy, pod->title);

  // update status
  // pod->location
  // pod->time
}

void
BelugaRemoveBuddy(BelugaAccount *account,
		  BelugaPod *pod)
{
  PurpleConversation *conv;
  PurpleBuddy *buddy;

  BelugaLogInfo("beluga",
		"BelugaRemoveBuddy(%s)",
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
    BelugaLogInfo("beluga", 
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
  BelugaRemoveBuddy((BelugaAccount *)user_data,
		    (BelugaPod *)value);
}

void
BelugaSetBuddyIcon(BelugaAccount *account,
		   BelugaPod *pod,
		   gchar *data, gsize data_len)
{
  data = g_memdup(data, data_len);
  purple_buddy_icon_new(account->account, 
			pod->id, 
			data, data_len, 
			NULL);
}

void
BelugaDebugMsg(BelugaAccount *account,
	       BelugaPod *pod,
	       const gchar *msgText)
{
  PurpleConversation *conv;
  PurpleMessageFlags msgFlags;  

  if (!BelugaAccountDebug(account)) {
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
		       "purple-beluga", 
		       msgText,
		       msgFlags,
		       time(NULL));
}

void
BelugaHintMsg(BelugaAccount *account,
	      BelugaPod *pod,
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
		       "purple-beluga", 
		       msgText,
		       msgFlags,
		       time(NULL));
}

void
BelugaDisplayNewUpdates(BelugaAccount *account,
			BelugaPod *pod)
{
  PurpleConversation *conv;
  RequestData *newRequest;
  BelugaUpdate *update;
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
  while (BelugaPodHasUpdate(pod, pod->nextUpdateDisplayed)) {
    update = BelugaPodGetUpdate(pod, pod->nextUpdateDisplayed);
    msgText = BelugaUpdateDisplayText(account,
				      pod,
				      update);

    msgTime = update->timestamp;

    isFromLocalUser = purple_strequal(account->uid, 
				      update->uid);
    msgFlags = PURPLE_MESSAGE_RECV;
    msgFlags |= isFromLocalUser?PURPLE_MESSAGE_NICK:0;
    msgFlags |= update->isDelayed?PURPLE_MESSAGE_DELAYED:0;
    BelugaLogMisc("beluga", "msg: %s (%s):\n\t%s\n",
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
    BelugaGetPodDetails(account, pod);
  }

  if (pod->nextUpdateDisplayed < pod->lastUpdateReceived) {
    BelugaDebugMsg(account, pod, "Recieved updates out of order.");
    // we have dropped updates or received them out of order  
    // wait 3 seconds, and if we are still missing updates
    // we will fetch them actively
    if (!pod->catchupPodTimeout) {
      BelugaDebugMsg(account, pod, "Waiting for missing updates.");
      newRequest = g_new0(RequestData, 1);
      newRequest->account = account;
      newRequest->pod = pod;
      pod->catchupPodTimeout = 
	purple_timeout_add_seconds(3,
				   BelugaCatchupPodTO,
				   (gpointer)newRequest);
    }
  }
}

void
PurpleBelugaCheckVersionAutomatically(BelugaAccount *account)
{
  BelugaLogInfo("beluga", "CheckVersionAutomatically\n");
 
  if (gCheckedVersion == TRUE) {
    return;
  }
  if (BelugaAccountCheckVersion(account) != TRUE) {
    return;
  }

  // get version data
  gCheckedVersion = TRUE;
  beluga_post_or_get(account, 
		     BELUGA_METHOD_GET, 
		     "purple-beluga.googlecode.com", 
		     "/svn/trunk/VERSION",
		     NULL, PurpleBelugaCheckVersionCB, 
		     (gpointer)TRUE, FALSE);
}

void
PurpleBelugaCheckVersion(BelugaAccount *account)
{
  BelugaLogInfo("beluga", "CheckVersion\n");
 
  // get version data
  gCheckedVersion = TRUE;
  beluga_post_or_get(account, 
		     BELUGA_METHOD_GET, 
		     "purple-beluga.googlecode.com", 
		     "/svn/trunk/VERSION",
		     NULL, PurpleBelugaCheckVersionCB, 
		     (gpointer)FALSE, FALSE);
}

void
PurpleBelugaCheckVersionCB(BelugaAccount *account, 
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

  BelugaLogInfo("beluga", "CheckVersionCB\n");
  automaticCheck = (gboolean)userData;

  json = data;
  json = json_object_seek(json);
  if (!json) {
    BelugaLogWarn("beluga", "no root object\n");
    return;
  }

  json = json_object_pair_value(json, "versions");
  if (!json) {
    BelugaLogWarn("beluga", "no versions field\n");
    return;
  }

  json = json_array_seek(json);
  if (!json) {
    BelugaLogWarn("beluga", "version not-array\n");
    return;
  }

  // parse all of the updates
  current = TRUE;
  text = g_strdup_printf("Download <a href=\"http://code.google.com/p/purple-beluga/downloads/list\">here.</a>");
  BelugaLogInfo("beluga", "current version %d.%d.%d\n", 
		BELUGA_PLUGIN_VERSION_MAJOR,
		BELUGA_PLUGIN_VERSION_MINOR,
		BELUGA_PLUGIN_VERSION_REVISION);
  allowBeta = BelugaAccountAllowBetaVersions(account);
  for (json = json_object_seek(json_array_contents(json));
       json != NULL;
       json = json_object_seek(json_token_seek(json))) {
    major = json_object_pair_value_int(json, "major");
    minor = json_object_pair_value_int(json, "minor");
    revision = json_object_pair_value_int(json, "revision");
    isBeta = json_object_pair_value_string_equals(json, "status", "beta");
    BelugaLogInfo("beluga", "checking version %d.%d.%d\n", 
		  major, minor, revision);
    if ((!isBeta || allowBeta) &&
	((major > BELUGA_PLUGIN_VERSION_MAJOR) ||
	 ((major == BELUGA_PLUGIN_VERSION_MAJOR) &&
	  ((minor > BELUGA_PLUGIN_VERSION_MINOR) ||
	   ((minor == BELUGA_PLUGIN_VERSION_MINOR) &&
	    ((revision > BELUGA_PLUGIN_VERSION_REVISION))))))) {
      BelugaLogMisc("beluga", "new!\n");
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
	BelugaLogMisc("beluga", "change: %s\n", change);
	tmp = g_strdup_printf("%s<br/>%s", text, change);
	g_free(change);
	g_free(text);
	text = tmp;
      }	
    }
  }
  
  // display popup informing user of update
  if (current == FALSE) {
    purple_notify_formatted(belugaPlugin, 
			    "purple-beluga Update",
			    "A new version of purple-beluga is available.", 
			    (automaticCheck==FALSE)?"":
			    "\n (automatic update check can be disabled in Account Settings)", 
			    text,
			    NULL, NULL);
  } else if (automaticCheck == FALSE) {
    purple_notify_message(belugaPlugin, 
			  PURPLE_NOTIFY_MSG_INFO, 
			  "purple-beluga is up-to-date",
			  "Your purple-beluga plugin is up-to-date.", 
			  NULL, NULL, NULL);
  }

  g_free(text);
}

void
BelugaConnect(PurpleAccount *pa)
{
  PurpleConnection *pc;
  BelugaAccount *account;
  
  BelugaLogInfo("beluga", "Connecting\n");
  pc = purple_account_get_connection(pa);
  purple_connection_set_state(pc, PURPLE_CONNECTING);

  // create a new account
  account = BelugaAccountNew();
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

  // check if there is a new version of purple-beluga
  PurpleBelugaCheckVersionAutomatically(account);

  // start connection process
  BelugaContactHost(account);
}

void
BelugaDisconnect(BelugaAccount *account)
{
  PurpleDnsQueryData *dns_query;
  const gchar *host;

  BelugaLogInfo("beluga", 
		"disabling account %s\n",
		account->name);

  // send logout to server
  host = BelugaAccountHost(account);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_GET, 
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
  BelugaLogMisc("beluga", 
		"destroying %d incomplete connections\n",
		g_slist_length(account->conns));
  while (account->conns != NULL)
    beluga_connection_destroy(account->conns->data);
  
  // cancel outstanding dns queries
  while (account->dns_queries != NULL) {
    dns_query = account->dns_queries->data;
    BelugaLogMisc("beluga", 
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
  BelugaAccountFree(account);
}

void
BelugaContactHost(BelugaAccount *account)
{
  const gchar *host;

  BelugaLogInfo("beluga", "ContactHost\n");
  purple_connection_update_progress(account->pc, 
				    _("Contacting host..."), 
				    1, 4);
 
  // get login page
  host = BelugaAccountHost(account);
  if (host == NULL || host[0] == '\0') {
    purple_connection_error(account->pc, _("Host not set"));
    return;
  }
  
  beluga_post_or_get(account, 
		     BELUGA_METHOD_GET | 
		     BELUGA_METHOD_SSL, 
		     host, "/login", 
		     NULL, BelugaContactHostCB, 
		     NULL, TRUE);  
}

void
BelugaContactHostCB(BelugaAccount *account, 
		    gchar const *requestUrl, 
		    gchar *html, gsize htmlLen, 
		    gpointer userData)
{
  const gchar *host;
  gchar *_xsrf;
  gchar *postData;
  gchar *encodedUser;
  gchar *encodedPass;
  
  BelugaLogInfo("beluga", "ContactHostCB\n");

  // pull the host's Beluga account id from the cookie
  _xsrf = g_hash_table_lookup(account->cookie_table, 
			      "_xsrf");
  if (!_xsrf) {
      BelugaLogError("beluga", "_xsrf not found\n");
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
  host = BelugaAccountHost(account);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_POST | 
		     BELUGA_METHOD_SSL,
		     host, "/login", 
		     postData, BelugaLoginCB, 
		     NULL, TRUE);
  g_free(postData);
}

void
BelugaLoginCB(BelugaAccount *account, 
	      gchar const *requestUrl, 
	      gchar *html, gsize htmlLen, 
	      gpointer userData)
{
  BelugaLogInfo("beluga", "LoginCB\n");
	
  // we're logged into the website now
  purple_connection_update_progress(account->pc, 
				    _("Fetching pods..."),
				    3, 4);
  
  //load the homepage to grab the pod info
  BelugaSeedAccount(account);
  // check again periodically for new pods
  account->seedTimeout = purple_timeout_add_seconds(60,
						    BelugaSeedAccountTO,
						    (gpointer)account);
}

void
BelugaCheckNewPods(BelugaAccount *account)
{
  BelugaSeedAccount(account);
}

gboolean BelugaSeedAccountTO(gpointer data)
{
  BelugaSeedAccount((BelugaAccount *)data);
  return TRUE;
}

void
BelugaSeedAccount(BelugaAccount *account)
{
  const gchar *host;
  BelugaLogInfo("beluga", "BelugaSeedAccount\n");
	
  //load the homepage to grab the pod info
  host = BelugaAccountHost(account);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_GET, 
		     host, "/pods", 
		     NULL, BelugaSeedAccountCB, 
		     NULL, TRUE);
}


void
BelugaSeedAccountCB(BelugaAccount *account, 
		    gchar const *requestUrl, 
		    gchar *data, gsize dataLen, 
		    gpointer userData)
{
  BelugaPod *newPod;
  const gchar *html;
  const gchar *error;

  BelugaLogInfo("beluga", 
		"SeedAccountCB(%s)\n",
		account->name);

  html = data;
  if (!html) {
    BelugaLogError("beluga", "no seedaccount data\n");
    purple_connection_error(account->pc, 
			    _("No account info"));
    return;
  }
  
  // we've got our account info & pods!
  BelugaLogInfo("beluga", "Connected!\n");
  purple_connection_notice(account->pc, 
			   "Connected to Beluga!");

  BelugaAccountFromHtml(account, html);
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
  while ((newPod = BelugaPodFromHtml(html, &html))) {
    BelugaLogInfo("beluga", 
		  "FoundPod %s (%s)\n", 
		  newPod->title,
		  newPod->id);
    if (BelugaAccountHasPod(account, newPod->id)) {
      BelugaLogInfo("beluga", "pod already exists, skipping.\n");
      BelugaPodFree(newPod);
    } else {
      BelugaLogInfo("beluga", "adding pod.\n");
      BelugaAddBuddy(account, newPod);
      BelugaAccountAddPod(account, newPod);
      BelugaDebugMsg(account, newPod, "Connected to Pod...");
      BelugaPodGeneratePhotoPath(account, newPod);
      BelugaPodImage(account, newPod);
      BelugaSeedPod(account, newPod);
      BelugaPollNewUpdates(account, newPod);
    }
  }
}

gboolean BelugaCatchupPodTO(gpointer data)
{
  RequestData *request;
  BelugaAccount *account;
  BelugaPod *pod;

  request = (RequestData *)data;
  account = request->account;
  pod = request->pod;
  g_free(request);
  pod->catchupPodTimeout = 0;

  if (pod->nextUpdateDisplayed < pod->lastUpdateReceived) {
    BelugaDebugMsg(account, pod, "Giving up.");
    BelugaSeedPod(account, pod);
  }

  return FALSE;
}

void
BelugaGetPodDetails(BelugaAccount *account, 
		    BelugaPod *pod)
{
  RequestData *newRequest;
  const gchar *host;
  gchar *url;
  int lastread;
  int n;

  BelugaLogInfo("beluga", 
		"GetPodDetails(%s)\n",
		pod->title);
	
  // how many updates should we fetch to seed the pod
  if (pod->nextUpdateDisplayed == 0) {
    // this is an initial fetch
    BelugaDebugMsg(account, pod, "Fetching recent history for chat.");
    lastread = -1;
    n = BelugaAccountSeedFetchCount(account);
  } else {
    // we are trying to catch up on missing updates
    BelugaDebugMsg(account, pod, "Getting pod details or missing updates.");
    lastread = pod->lastUpdateReceived;
    n = (pod->lastUpdateReceived - pod->nextUpdateDisplayed + 1);
    n += 5; // for good measure
  }

  // fetch updates for this pod
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  host = BelugaAccountHost(account);
  //api/pods/getPod?n=50&v=3&start=128&podid=4de21d23e694aa6c05004dfd&lastread=-1
  url = g_strdup_printf("/api/pods/getPod?v=3&podid=%s&lastread=%d&n=%d", 
			purple_url_encode(pod->id), 
			lastread,
			n);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_GET, 
		     host, url,
		     NULL, BelugaGetPodDetailsCB, 
		     newRequest, TRUE);
  g_free(url);
}

void 
BelugaGetPodDetailsCB(BelugaAccount *account, 
		      gchar const *requestUrl, 
		      gchar *data, gsize dataLen, 
		      gpointer userData)
{
  RequestData *request;
  BelugaPod *pod;
  //BelugaUpdate *newUpdate;
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

  BelugaLogInfo("beluga", 
		"GetPodDetailsCB(%s)\n",
		pod->title);
  //BelugaLogMisc("beluga", "%s", data);

  // look for json result
  json = data;
  if (!json) {
    BelugaLogWarn("beluga", "bad createPod response");
    return;
  }
  
  root = json_object_seek(json);
  if (!root) {
    BelugaLogWarn("beluga", "no root object\n");
    return;
  }

  result = json_object_pair_value(root, "result");
  if (!result) {
    BelugaLogWarn("beluga", "no result field\n");

    // check if user has been removed from pod
    if (json_object_pair_value_string_equals(root, 
					     "error",
					     "user not authorized")) {
      BelugaLogWarn("beluga", "user has been removed from pod\n");
      BelugaRemoveBuddy(account, pod);
      BelugaAccountRemovePod(account, pod);
      //BelugaPodDestroyPhotoPath(account, newPod);
      return;
    }

    return;
  }

  what = json_object_pair_value_string_dup(result, "what");
  if (what[0]) {
    BelugaPodSetTitle(pod, what);
  }
  g_free(what);

  where = json_object_pair_value_string_dup(result, "where");
  if (where[0]) {
    //BelugaPodSetLocation(pod, where);
  }
  g_free(where);

  when = json_object_pair_value_string_dup(result, "when");
  if (when[0]) {
    //BelugaPodSetTime(pod, when);
  }
  g_free(when);

  ihash = json_object_pair_value_string_dup(result, "ihash");
  if (ihash[0] && g_ascii_strncasecmp(ihash, pod->ihash, strlen(ihash))) {
    BelugaPodSetIHash(pod, ihash);
    BelugaPodImage(account, pod);
  }
  g_free(ihash);

  BelugaUpdateBuddy(account, pod);
}

void
BelugaSeedPod(BelugaAccount *account, BelugaPod *pod)
{ 
  RequestData *newRequest;
  const gchar *host;
  gchar *url;
  int seedFetchCount;

  BelugaLogInfo("beluga", 
		"SeedPod(%s)\n",
		pod->title);
	
  // how many updates should we fetch to seed the pod
  if (pod->nextUpdateDisplayed == 0) {
    // this is an initial fetch
    BelugaDebugMsg(account, pod, "Fetching recent history for chat.");
    seedFetchCount = BelugaAccountSeedFetchCount(account);
  } else {
    // we are trying to catch up on missing updates
    BelugaDebugMsg(account, pod, "Re-fetching recent history.");
    seedFetchCount = (pod->lastUpdateReceived - pod->nextUpdateDisplayed + 1);
    seedFetchCount += 10; // for good measure
  }

  // fetch updates for this pod
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  host = BelugaAccountHost(account);
  url = g_strdup_printf("/pod/%s?n=%d", 
			purple_url_encode(pod->id), 
			seedFetchCount);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_GET, 
		     host, url,
		     NULL, BelugaSeedPodCB, 
		     newRequest, TRUE);
  g_free(url);
}

void 
BelugaSeedPodCB(BelugaAccount *account, 
		gchar const *requestUrl, 
		gchar *data, gsize dataLen, 
		gpointer userData)
{
  RequestData *request;
  BelugaPod *pod;
  BelugaUpdate *newUpdate;
  const gchar *html;
  gint lastIndex;

  request = (RequestData *)userData;
  pod = request->pod;
  g_free(request);

  BelugaLogInfo("beluga", 
		"SeedPodCB(%s)\n",
		pod->title);
  //BelugaLogMisc("beluga", "response:\n%s", data);

  html = data;
  if (!html) {
    BelugaLogWarn("beluga", "bad seed response");
    return;
  }

  // parse results
  BelugaDebugMsg(account, pod, "Got recent history results.");
  // maybe should search for "realmessages" flag first
  lastIndex = 0;
  while ((newUpdate = BelugaUpdateFromHtml(html, &html))) {
    if (BelugaPodHasUpdate(pod, newUpdate->index)) {
      BelugaUpdateFree(newUpdate);
    } else {
      newUpdate->isDelayed = TRUE;
      BelugaPodAddUpdate(pod, newUpdate);
      if (newUpdate->hasPhoto) {
	BelugaUpdateDetails(account,
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
    BelugaHintMsg(account, pod, 
		  "type /help to get a list of pod commands");
  }

  // display any new updates we may have gotten
  BelugaDisplayNewUpdates(account, pod);
}

void
BelugaRetryPollNewUpdates(BelugaAccount *account,
		       BelugaPod *pod)
{
  RequestData *newRequest;
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;

  BelugaDebugMsg(account, pod, 
		 "Scheduling retry poll for updates...");

  pod->retryPollPodTimeout = 
    purple_timeout_add_seconds(60,
			       BelugaRetryPollNewUpdatesTO,
			       (gpointer)newRequest);
}

gboolean 
BelugaRetryPollNewUpdatesTO(gpointer data)
{ 
  RequestData *request;
  request = (RequestData *)data;

  BelugaDebugMsg(request->account, request->pod, 
		 "Retrying poll for updates...");

  request->pod->retryPollPodTimeout = 0;
  // Pickup lost messages
  BelugaSeedPod(request->account, request->pod);
  // Pickup new messages
  BelugaPollNewUpdates(request->account, request->pod);

  g_free(request);
  return FALSE;
}

void
BelugaPollNewUpdates(BelugaAccount *account,
		     BelugaPod *pod)
{
  RequestData *newRequest;
  const gchar *host;
  gchar *url;
  
  BelugaLogInfo("beluga", 
		"PollNewUpdates(%s)\n",
		pod->title);
  
  //fetch updates for this pod
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  host = BelugaAccountHost(account);
  url = g_strdup_printf("/api/poll/newUpdates?podid=%s", 
			  purple_url_encode(pod->id));
  beluga_post_or_get(account, 
		     BELUGA_METHOD_GET, 
		     host, url,
		     NULL, BelugaPollNewUpdatesCB, 
		     newRequest, TRUE);
  g_free(url);
}


void 
BelugaPollNewUpdatesCB(BelugaAccount *account, 
		       gchar const *requestUrl, 
		       gchar *data, gsize dataLen, 
		       gpointer userData)
{
  RequestData *request;
  BelugaPod *pod;
  BelugaUpdate *newUpdate;
  const gchar *json;
  const gchar *root;
  const gchar *result;

  request = (RequestData *)userData;
  pod = request->pod;
  g_free(request);

  BelugaLogInfo("beluga", 
		"PollNewUpdatesCB(%s)\n",
		pod->title);
  //BelugaLogMisc("beluga", "response:\n%s", data);

  // look for json result
  if (!data) {
    BelugaLogWarn("beluga", "bad poll response, no data");
    BelugaRetryPollNewUpdates(account, pod);
    return;
  }
  
  root = json_object_seek(data);
  if (!root) {
    BelugaLogWarn("beluga", "no root json object\n");
    BelugaRetryPollNewUpdates(account, pod);
    return;
  }

  result = json_object_pair_value(root, "result");
  if (!result) {
    BelugaLogWarn("beluga", "no result field\n");

    // check if user has been removed from pod
    if (json_object_pair_value_string_equals(root, 
					     "error",
					     "user not authorized")) {
      BelugaLogWarn("beluga", "user has been removed from pod\n");
      BelugaRemoveBuddy(account, pod);
      BelugaAccountRemovePod(account, pod);
      //BelugaPodDestroyPhotoPath(account, newPod);
      return;
    }

    // unhandled response, retry
    BelugaRetryPollNewUpdates(account, pod);
    return;
  }

  json = json_array_seek(result);
  if (!json) {
    BelugaLogWarn("beluga", "result not-array\n");
    BelugaRetryPollNewUpdates(account, pod);
    return;
  }

  // parse all of the updates
  for (json = json_object_seek(json_array_contents(json));
       json != NULL;
       json = json_object_seek(json_token_seek(json))) {
    
    newUpdate = BelugaUpdateFromJson(json);
    if (BelugaPodHasUpdate(pod, newUpdate->index)) {
      BelugaUpdateFree(newUpdate);
    } else {
      BelugaPodAddUpdate(pod, newUpdate);
      if (newUpdate->hasPhoto) {
	BelugaUpdateDetails(account,
			    pod,
			    newUpdate);
      }
      if (newUpdate->index > pod->lastUpdateReceived) {
	pod->lastUpdateReceived = newUpdate->index;
      }
    }
  }
  
  // display any new updates we may have gotten
  BelugaDisplayNewUpdates(account, pod);

  // poll again for new updates
  BelugaPollNewUpdates(account, pod);
}

void
BelugaPodImage(BelugaAccount *account,
	       BelugaPod *pod)
{
  RequestData *newRequest;
  const gchar *host;
  
  purple_debug_info("beluga", 
		    "PodImage(%s, %s)\n",
		    pod->id, 
		    pod->imageUrl);
  
  //fetch details for this update
  //fetch details for this update
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  host = BelugaAccountHost(account);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_GET, 
		     host, pod->imageUrl,
		     NULL, BelugaPodImageCB, 
		     newRequest, TRUE);
}

void 
BelugaPodImageCB(BelugaAccount *account, 
		 gchar const *requestUrl, 
		 gchar *data, gsize dataLen, 
		 gpointer userData)
{
  RequestData *request;

  request = (RequestData *)userData;
  BelugaLogInfo("beluga",
		"PodImageCB(%s)\n",
		request->pod->id);

  // read details
  BelugaPodImageFromPngData(request->account,
			    request->pod,
			    data, dataLen);

  g_free(request);
}

void
BelugaUpdateDetails(BelugaAccount *account,
		    BelugaPod *pod,
		    BelugaUpdate *update)
{
  RequestData *newRequest;
  const gchar *host;
  gchar *url;
  
  BelugaLogInfo("beluga", 
		"UpdateDetails(%s, %d)\n",
		pod->id, update->index);
  
  //fetch details for this update
  newRequest = g_new0(RequestData, 1);
  newRequest->account = account;
  newRequest->pod = pod;
  newRequest->update = update;
  newRequest->index = update->index;
  host = BelugaAccountHost(account);
  url = g_strdup_printf("/details/%s/%d",
			purple_url_encode(pod->id),
			update->index);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_GET, 
		     host, url, 
		     NULL, BelugaUpdateDetailsCB, 
		     newRequest, TRUE);
  g_free(url);
}

void 
BelugaUpdateDetailsCB(BelugaAccount *account, 
		      const gchar *requestUrl,
		      gchar *html, gsize htmlLen, 
		      gpointer userData)
{
  RequestData *request;
  RequestData *newRequest;
  const gchar *host;

  request = (RequestData *)userData;
  BelugaLogInfo("beluga", 
		"UpdateDetailsCB(%s, %d)\n",
		request->pod->id, 
		request->index);

  // read details
  BelugaUpdateDetailsFromHtml(request->update,
			      html);

  // process details
  //fetch photo for this update
  if (request->update->photoUrl) {
    BelugaDebugMsg(account, request->pod, "Fetching photo.");
    host = BelugaAccountHost(request->account);
    newRequest = RequestDataClone(request);  
    beluga_post_or_get(request->account, 
		       BELUGA_METHOD_GET, 
		       host, request->update->photoUrl,
		       NULL, BelugaUpdatePhotoCB, 
		       newRequest, TRUE);
  }

  g_free(request);
}

void 
BelugaUpdatePhotoCB(BelugaAccount *account, 
		    gchar const *requestUrl, 
		    gchar *data, gsize dataLen, 
		    gpointer userData)
{
  RequestData *request;

  request = (RequestData *)userData;
  BelugaLogInfo("beluga",
		"UpdatePhotoCB(%s, %d)\n",
		request->pod->id, 
		request->index);

  // read details
  BelugaDebugMsg(account, request->pod, "Got photo.");
  BelugaUpdatePhotoFromJpegData(request->pod,
				request->update,
				data, dataLen);

  g_free(request);
}

void
BelugaCreatePod(BelugaAccount *account)
{
  const gchar *host;
  gchar *encodedName;
  gchar *encodedEmails;
  gchar *postData;

  BelugaLogMisc("beluga", "CreatePod()\n");

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
  host = BelugaAccountHost(account);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_POST, 
		     host, "/api/pods/createPod", 
		     postData,
		     BelugaCreatePodCB, NULL,
		     TRUE);
  g_free(postData);
}

void 
BelugaCreatePodCB(BelugaAccount *account, 
		  gchar const *requestUrl, 
		  gchar *data, gsize dataLen, 
		  gpointer userData)
{ 
  BelugaPod *newPod; 
  const gchar *json;

  BelugaLogMisc("beluga", "CreatePodCB()\n");
  //BelugaLogMisc("beluga", "response:\n%s", data);

  // look for json result
  json = data;
  if (!json) {
    BelugaLogWarn("beluga", "bad createPod response");
    return;
  }
  
  json = json_object_seek(json);
  if (!json) {
    BelugaLogWarn("beluga", "no root object\n");
    return;
  }

  json = json_object_pair_value(json, "result");
  if (!json) {
    BelugaLogWarn("beluga", "no result field\n");
    return;
  }

  // setup buddy for pod
  newPod = BelugaPodFromJson(json, &json);
  if (!newPod) {
    BelugaLogWarn("beluga", "result is not valid pod json\n");
    return;
  }

  BelugaLogInfo("beluga", 
		"FoundPod %s (%s)\n", 
		newPod->title,
		newPod->id);
  if (BelugaAccountHasPod(account, newPod->id)) {
    BelugaLogInfo("beluga", "pod already exists, skipping.\n");
    BelugaPodFree(newPod);
  } else {
    BelugaLogInfo("beluga", "adding pod.\n");
    BelugaAddBuddy(account, newPod);
    BelugaAccountAddPod(account, newPod);
    BelugaDebugMsg(account, newPod, "Connected to Pod...");
    BelugaPodGeneratePhotoPath(account, newPod);
    BelugaPodImage(account, newPod);
    BelugaSeedPod(account, newPod);
    BelugaPollNewUpdates(account, newPod);
  }
}

void
BelugaPodSetName(BelugaAccount *account,
		  BelugaPod *pod,
		  const gchar *name)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *encodedName;
  
  BelugaLogInfo("beluga", 
		"PodSetName(%s, %s)\n",
		pod->id, name);
  
  host = BelugaAccountHost(account);
  url = g_strdup_printf("/api/pods/modifyPod");
  encodedName = g_strdup(purple_url_encode(name));
  postData = g_strdup_printf("v=3&_xsrf=%s&podid=%s&what=%s",
			     account->_xsrf, 
			     pod->id,
			     encodedName);
  g_free(encodedName);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_POST, 
		     host, url, 
		     postData, NULL, 
		     NULL, FALSE);
  g_free(postData);
  g_free(url);
}

void
BelugaPodSetLocation(BelugaAccount *account,
		     BelugaPod *pod,
		     const gchar *location)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *encodedLocation;
  
  BelugaLogInfo("beluga", 
		"PodSetLocation(%s, %s)\n",
		pod->id, location);
  
  host = BelugaAccountHost(account);
  url = g_strdup_printf("/api/pods/modifyPod");
  encodedLocation = g_strdup(purple_url_encode(location));
  postData = g_strdup_printf("v=3&_xsrf=%s&podid=%s&where=%s",
			     account->_xsrf, 
			     pod->id,
			     encodedLocation);
  g_free(encodedLocation);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_POST, 
		     host, url, 
		     postData, NULL, 
		     NULL, FALSE);
  g_free(postData);
  g_free(url);
}

void
BelugaPodSetAddress(BelugaAccount *account,
		    BelugaPod *pod,
		    const gchar *address)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *encodedAddress;
  
  BelugaLogInfo("beluga", 
		"PodSetAddress(%s, %s)\n",
		pod->id, address);
  
  host = BelugaAccountHost(account);
  url = g_strdup_printf("/api/pods/modifyPod");
  encodedAddress = g_strdup(purple_url_encode(address));
  postData = g_strdup_printf("v=3&_xsrf=%s&podid=%s&addr=%s",
			     account->_xsrf, 
			     pod->id,
			     encodedAddress);
  g_free(encodedAddress);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_POST, 
		     host, url, 
		     postData, NULL, 
		     NULL, FALSE);
  g_free(postData);
  g_free(url);
}

void
BelugaPodAddMembers(BelugaAccount *account, 
		    BelugaPod *pod, 
		    gchar *argsString)
{
  gchar **split;
  int n;

  BelugaLogInfo("beluga", 
		"PodAddMember(%s)\n",
		pod->id);

  split = g_strsplit(argsString, ",", 0);
  for (n=0; split[n]; ++n) {
    BelugaPodAddMember(account, pod, split[n]);
  }
  g_strfreev(split);
}

void
BelugaPodAddMember(BelugaAccount *account, 
		     BelugaPod *pod, 
		     gchar *member)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *invites;
  
  BelugaLogInfo("beluga", 
		"PodInviteEmail(%s, %s)\n",
		pod->id, member);
  
  host = BelugaAccountHost(account);
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
  beluga_post_or_get(account, 
		     BELUGA_METHOD_POST, 
		     host, url, 
		     postData, NULL, 
		     NULL, FALSE);
  g_free(postData);
  g_free(url);  
}

void
BelugaPodRemoveMembers(BelugaAccount *account,
		    BelugaPod *pod, 
		    gchar *argsString)
{
  gchar **split;
  int n;

  BelugaLogInfo("beluga", 
		"PodRemoveMember(%s)\n",
		pod->id);

  split = g_strsplit(argsString, ",", 0);
  for (n=0; split[n]; ++n) {
    BelugaPodRemoveMember(account, pod, split[n]);
  }
  g_strfreev(split);
}

void
BelugaPodRemoveMember(BelugaAccount *account,
		      BelugaPod *pod,
		      const gchar *uid)
{
  const gchar *host;
  gchar *postData;
  gchar *url;
  gchar *userList;
  
  BelugaLogInfo("beluga", 
		"PodRemoveUser(%s, %s)\n",
		pod->id, uid);
  
  host = BelugaAccountHost(account);
  url = g_strdup_printf("/api/pods/modifyPod");
  // POST data:
  // rmusers=["4d6d4f5c78f2f240e0002086"]&podid=4de21d23e694aa6c05004dfd&v=3
  userList = g_strdup_printf("[\"%s\"]", uid);
  postData = g_strdup_printf("v=3&_xsrf=%s&podid=%s&rmusers=%s", 
			     account->_xsrf,
  			     pod->id,
  			     purple_url_encode(userList));
  g_free(userList);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_POST, 
		     host, url, 
		     postData, NULL, 
		     NULL, FALSE);
  g_free(postData);
  g_free(url);
}

void
BelugaPodLeave(BelugaAccount *account,
	       BelugaPod *pod)
{
  BelugaLogInfo("beluga", 
		"PodLeave(%s)\n",
		pod->id);
  BelugaPodRemoveMember(account, pod, account->uid);
}

void
BelugaSendMessage(BelugaAccount *account,
		  const char *podId,
		  const char *msg)
{
  BelugaPod *pod;
  pod = BelugaAccountGetPod(account, podId);

  // add this message to the pod's outbox
  pod->toSend = g_slist_append(pod->toSend, 
			       (gpointer)g_strdup(msg));
  
  // if the new message is the only one in 
  // the queue, call SendNextMessage
  if (g_slist_length(pod->toSend) == 1) {
    BelugaSendNextMessage(account, pod);
  }
}

void
BelugaSendNextMessage(BelugaAccount *account,
		      BelugaPod *pod)
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
  BelugaLogMisc("beluga",
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
  host = BelugaAccountHost(account);
  beluga_post_or_get(account, 
		     BELUGA_METHOD_POST, 
		     host, "/api/updates/createUpdate", 
		     postData,
		     BelugaSendNextMessageCB, pod,
		     TRUE);
  g_free(postData);
}
		  
void 
BelugaSendNextMessageCB(BelugaAccount *account, 
			gchar const *requestUrl, 
			gchar *data, gsize dataLen, 
			gpointer userData)
{
  BelugaPod *pod;
  gchar *msg;
  pod = (BelugaPod *)userData;

  BelugaLogMisc("beluga",
		"SendNextMessageCB(%s)\n",
		pod->id);

  // remove this message from the queue
  msg = (gchar *)pod->toSend->data;
  pod->toSend = g_slist_remove(pod->toSend, (gpointer)msg);
  g_free(msg);

  // try to send another message
  BelugaSendNextMessage(account, pod);
}
