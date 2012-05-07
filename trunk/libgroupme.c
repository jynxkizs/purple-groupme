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

#include "libgroupme.h"
#include "GroupMeAccount.h"
#include "GroupMePod.h"
#include "GroupMeProtocol.h"
#include "GroupMeUpdate.h"
#include "groupme_connection.h"
#include "cmds.h"

#define GROUPME_PRPL_ID "prpl-battleactionken-groupme"
#define GROUPME_HOMEPAGE "http://code.google.com/p/purple-groupme/"
#define GROUPME_IMAGES_DIR "plugins/purple-groupme/images"
#define GROUPME_POD_IMAGES_DIR "plugins/purple-groupme/podimg"
#define GROUPME_AVAILABLE  "ONLINE"

#define GROUPME_CMD_SET_NICK "nick"
#define GROUPME_CMD_NEW_POD "newgroup"
#define GROUPME_CMD_SET_POD_NAME "setgroupname"
#define GROUPME_CMD_DELETE_POD "destroygroup"
#define GROUPME_CMD_LEAVE_POD "leavegroup"
#define GROUPME_CMD_ADD_MEMBER "addmember"
#define GROUPME_CMD_REMOVE_MEMBER "removemember"

#define GROUPME_CMD_SET_NICK_IE "i.e. /" GROUPME_CMD_SET_NICK \
  " ken"
#define GROUPME_CMD_SET_NICK_HELP GROUPME_CMD_SET_NICK \
  " <i>newnick</i>"				       \
  ": "						       \
  "sets your nickname in this group to <i>newnick</i>" \
  "\n"						       \
  GROUPME_CMD_SET_NICK_IE

#define GROUPME_CMD_NEW_POD_IE "i.e. /" GROUPME_CMD_NEW_POD 
#define GROUPME_CMD_NEW_POD_HELP GROUPME_CMD_NEW_POD \
  ": "						     \
  "creates a new group"				     \
  "\n"						     \
  GROUPME_CMD_POD_IE

#define GROUPME_CMD_SET_POD_NAME_IE "i.e. /" GROUPME_CMD_SET_POD_NAME \
  " Dinner Tonight!"
#define GROUPME_CMD_SET_POD_NAME_HELP GROUPME_CMD_SET_POD_NAME \
  " <i>Name</i>"					     \
  ": "							     \
  "sets this group's name to <i>Name</i>"		     \
  "\n"							     \
  GROUPME_CMD_SET_POD_NAME_IE

#define GROUPME_CMD_DELETE_POD_IE "i.e. /" GROUPME_CMD_DELETE_POD 
#define GROUPME_CMD_DELETE_POD_HELP GROUPME_CMD_DELETE_POD		\
  ": "									\
  "destroy this group (must be owner, others use /leavegroup)"		\
  "\n"									\
  GROUPME_CMD_DELETE_POD_IE

#define GROUPME_CMD_LEAVE_POD_IE "i.e. /" GROUPME_CMD_LEAVE_POD 
#define GROUPME_CMD_LEAVE_POD_HELP GROUPME_CMD_LEAVE_POD		\
  ": "									\
  "leave this group (owner cannot leave, can only /destroygroup)"	\
  "\n"									\
  GROUPME_CMD_LEAVE_POD_IE

#define GROUPME_CMD_ADD_MEMBER_IE "i.e. /" GROUPME_CMD_ADD_MEMBER \
  " jennifer@friend.net"
#define GROUPME_CMD_ADD_MEMBER_HELP GROUPME_CMD_ADD_MEMBER \
  " <i>email</i>"					 \
  ": "							 \
  "adds <i>email</i> to this chat"			 \
  "\n"							 \
  GROUPME_CMD_ADD_MEMBER_IE

#define GROUPME_CMD_REMOVE_MEMBER_IE "i.e. /" GROUPME_CMD_REMOVE_MEMBER \
  " jennifer"
#define GROUPME_CMD_REMOVE_MEMBER_HELP GROUPME_CMD_REMOVE_MEMBER \
  " <i>name</i>"					       \
  ": "							       \
  "removes <i>name</i> from this chat"			       \
  "\n"							       \
  GROUPME_CMD_REMOVE_MEMBER_IE

// Globals
GSList *groupmeCmdIds = NULL;

/******************************************************************************/
/* PRPL functions */
/******************************************************************************/

static const char *
groupme_list_icon(PurpleAccount *account, PurpleBuddy *buddy)
{
  return "groupme";
}


static GList *
groupme_statuses(PurpleAccount *account)
{
  GList *types = NULL;
  PurpleStatusType *status;
  
  purple_debug_info("groupme", "statuses\n");
  
  // GroupMe pods are either online or offline
  status = purple_status_type_new_full(PURPLE_STATUS_AVAILABLE, NULL, NULL, TRUE, TRUE, FALSE);
  types = g_list_append(types, status);
  
  status = purple_status_type_new_full(PURPLE_STATUS_OFFLINE, NULL, NULL, TRUE, TRUE, FALSE);
  types = g_list_append(types, status);
  
  purple_debug_info("groupme", "statuses return\n");
  return types;
}

PurpleCmdRet
groupme_cmd_set_nick_cb(PurpleConversation *pc, 
			const gchar *cmd,
			gchar **args, 
			gchar **error, 
			void *userData)
{
  PurpleAccount *pa;
  PurpleConnection *gc;
  GroupMeAccount *account;
  GroupMePod *pod;
  const gchar *podId;

  purple_debug_info("groupme", "cmd_set_nick_cb\n");

  if (!args || !args[0]) {
    (*error) = g_strdup(GROUPME_CMD_SET_NICK
			": requires a new nick name "
			"(" GROUPME_CMD_SET_NICK_IE ")");
    return PURPLE_CMD_RET_FAILED;
  }

  pa = pc->account;
  gc = pa->gc;
  account = (GroupMeAccount *)gc->proto_data;
  podId = (const gchar *)purple_conversation_get_name(pc);
  pod = GroupMeAccountGetPod(account, podId);
  if (!pod) {
    (*error) = g_strdup(GROUPME_CMD_SET_NICK
			" command issued on non-existent group");
    return PURPLE_CMD_RET_FAILED;
  }
  
  GroupMePodSetNick(account, pod, args[0]);
  return PURPLE_CMD_RET_OK;
}

PurpleCmdRet
groupme_cmd_new_pod_cb(PurpleConversation *pc, 
		      const gchar *cmd,
		      gchar **args, 
		      gchar **error, 
		      void *userData)
{
  PurpleAccount *pa;
  PurpleConnection *gc;
  GroupMeAccount *account;

  purple_debug_info("groupme", "cmd_new_pod_cb\n");

  pa = pc->account;
  gc = pa->gc;
  account = (GroupMeAccount *)gc->proto_data;
  GroupMeCreatePod(account);
  return PURPLE_CMD_RET_OK;
}

PurpleCmdRet
groupme_cmd_set_pod_name_cb(PurpleConversation *pc, 
			   const gchar *cmd,
			   gchar **args, 
			   gchar **error, 
			   void *userData)
{
  PurpleAccount *pa;
  PurpleConnection *gc;
  GroupMeAccount *account;
  GroupMePod *pod;
  const gchar *podId;

  purple_debug_info("groupme", "cmd_set_pod_name_cb\n");

  if (!args || !args[0]) {
    (*error) = g_strdup(GROUPME_CMD_SET_POD_NAME
			": requires a new group name "
			"(" GROUPME_CMD_SET_POD_NAME_IE ")");
    return PURPLE_CMD_RET_FAILED;
  }

  pa = pc->account;
  gc = pa->gc;
  account = (GroupMeAccount *)gc->proto_data;
  podId = (const gchar *)purple_conversation_get_name(pc);
  pod = GroupMeAccountGetPod(account, podId);
  if (!pod) {
    (*error) = g_strdup(GROUPME_CMD_SET_POD_NAME
			" command issued on non-existent chat");
    return PURPLE_CMD_RET_FAILED;
  }
  
  GroupMePodSetName(account, pod, args[0]);
  return PURPLE_CMD_RET_OK;
}

PurpleCmdRet
groupme_cmd_delete_cb(PurpleConversation *pc, 
		      const gchar *cmd,
		      gchar **args, 
		      gchar **error, 
		      void *userData)
{
  PurpleAccount *pa;
  PurpleConnection *gc;
  GroupMeAccount *account;
  GroupMePod *pod;
  const gchar *podId;

  purple_debug_info("groupme", "cmd_delete_cb\n");

  pa = pc->account;
  gc = pa->gc;
  account = (GroupMeAccount *)gc->proto_data;
  podId = (const gchar *)purple_conversation_get_name(pc);
  pod = GroupMeAccountGetPod(account, podId);
  if (!pod) {
    (*error) = g_strdup(GROUPME_CMD_DELETE_POD
			" command issued on non-existent group");
    return PURPLE_CMD_RET_FAILED;
  }
  
  GroupMePodDelete(account, pod);
  return PURPLE_CMD_RET_OK;
}

/*
PurpleCmdRet
groupme_cmd_leave_cb(PurpleConversation *pc, 
		    const gchar *cmd,
		    gchar **args, 
		    gchar **error, 
		    void *userData)
{
  PurpleAccount *pa;
  PurpleConnection *gc;
  GroupMeAccount *account;
  GroupMePod *pod;
  const gchar *podId;

  purple_debug_info("groupme", "cmd_leave_cb\n");

  pa = pc->account;
  gc = pa->gc;
  account = (GroupMeAccount *)gc->proto_data;
  podId = (const gchar *)purple_conversation_get_name(pc);
  pod = GroupMeAccountGetPod(account, podId);
  if (!pod) {
    (*error) = g_strdup(GROUPME_CMD_LEAVE_POD
			" command issued on non-existent group");
    return PURPLE_CMD_RET_FAILED;
  }
  
  GroupMePodLeave(account, pod);
  return PURPLE_CMD_RET_OK;
}

PurpleCmdRet
groupme_cmd_add_cb(PurpleConversation *pc, 
		  const gchar *cmd,
		  gchar **args, 
		  gchar **error, 
		  void *userData)
{
  PurpleAccount *pa;
  PurpleConnection *gc;
  GroupMeAccount *account;
  GroupMePod *pod;
  const gchar *podId;

  purple_debug_info("groupme", "cmd_add_cb\n");

  if (!args || !args[0]) {
    (*error) = g_strdup(GROUPME_CMD_ADD_MEMBER
			": requires an email address "
			"(" GROUPME_CMD_ADD_MEMBER_IE ")");
    return PURPLE_CMD_RET_FAILED;
  }

  pa = pc->account;
  gc = pa->gc;
  account = (GroupMeAccount *)gc->proto_data;
  podId = (const gchar *)purple_conversation_get_name(pc);
  pod = GroupMeAccountGetPod(account, podId);
  if (!pod) {
    (*error) = g_strdup(GROUPME_CMD_ADD_MEMBER
			" command issued on non-existent chat");
    return PURPLE_CMD_RET_FAILED;
  }

  GroupMePodAddMembers(account, pod, args[0]);
  return PURPLE_CMD_RET_OK;
}


PurpleCmdRet
groupme_cmd_remove_cb(PurpleConversation *pc, 
		     const gchar *cmd,
		     gchar **args, 
		     gchar **error, 
		     void *userData)
{
  PurpleAccount *pa;
  PurpleConnection *gc;
  GroupMeAccount *account;
  GroupMePod *pod;
  const gchar *podId;

  purple_debug_info("groupme", "cmd_remove_cb\n");

  if (!args || !args[0]) {
    (*error) = g_strdup(GROUPME_CMD_REMOVE_MEMBER
			": requires a member "
			"(" GROUPME_CMD_REMOVE_MEMBER_IE ")");
    return PURPLE_CMD_RET_FAILED;
  }

  pa = pc->account;
  gc = pa->gc;
  account = (GroupMeAccount *)gc->proto_data;
  podId = (const gchar *)purple_conversation_get_name(pc);
  pod = GroupMeAccountGetPod(account, podId);
  if (!pod) {
    (*error) = g_strdup(GROUPME_CMD_REMOVE_MEMBER
			" command issued on non-existent chat");
    return PURPLE_CMD_RET_FAILED;
  }
  
  GroupMePodRemoveMembers(account, pod, args[0]);
  return PURPLE_CMD_RET_OK;
}
*/

static void 
groupme_blist_node_menu_edit_pod_cb(PurpleBlistNode *node, 
				   gpointer userData) 
{
  GroupMeAccount *account;
  GroupMePod *pod;

  purple_debug_info("groupme", "blist_node_menu_edit_pod_cb\n");

  if (!PURPLE_BLIST_NODE_IS_BUDDY(node)) {
    purple_debug_error("groupme", "blist_node is not Buddy!\n");
    return;
  }

  account = (GroupMeAccount *)userData;
  pod = (GroupMePod *)(PURPLE_BUDDY(node)->proto_data);
  GroupMeHintMsg(account, pod, 
		"type /help to get a list of group commands");
  GroupMeHintMsg(account, pod, 
		"type /help <i>commandname</i> for help "
		"with a specific command.");

  purple_notify_message(groupmePlugin, 
			PURPLE_NOTIFY_MSG_INFO, 
			"Edit Group Help",
                        "Editing groups is accomplished via\n"
			" /commands within the IM window."
			"",
			"    " GROUPME_CMD_SET_NICK_IE "\n" 
			"    " GROUPME_CMD_SET_POD_NAME_IE "\n" 
			"    " GROUPME_CMD_DELETE_POD_IE "\n"
			"", 
			NULL, 
			NULL);
}

static void 
groupme_blist_node_menu_delete_cb(PurpleBlistNode *node, 
				  gpointer userData) 
{
  GroupMeAccount *account;
  GroupMePod *pod;

  purple_debug_info("groupme", "blist_node_menu_delete_cb\n");

  if (!PURPLE_BLIST_NODE_IS_BUDDY(node)) {
    purple_debug_error("groupme", "blist_node is not Buddy!\n");
    return;
  }

  account = (GroupMeAccount *)userData;
  pod = (GroupMePod *)(PURPLE_BUDDY(node)->proto_data);
  GroupMePodDelete(account, pod);
}

/*
static void 
groupme_blist_node_menu_leave_cb(PurpleBlistNode *node, 
				gpointer userData) 
{
  GroupMeAccount *account;
  GroupMePod *pod;

  purple_debug_info("groupme", "blist_node_menu_leave_cb\n");

  if (!PURPLE_BLIST_NODE_IS_BUDDY(node)) {
    purple_debug_error("groupme", "blist_node is not Buddy!\n");
    return;
  }

  account = (GroupMeAccount *)userData;
  pod = (GroupMePod *)(PURPLE_BUDDY(node)->proto_data);
  GroupMePodLeave(account, pod);
}
*/

static GList *
groupme_blist_node_menu(PurpleBlistNode *node)
{
  PurpleAccount *account;
  GList *actions;
  PurpleMenuAction *action;
  
  purple_debug_info("groupme", "blist_node_menu\n");

  if (!PURPLE_BLIST_NODE_IS_BUDDY(node)) {
    purple_debug_error("groupme", "blist_node is not Buddy!\n");
    return NULL;
  }

  actions = NULL;
  account = purple_buddy_get_account(PURPLE_BUDDY(node));

  // Edit Pod Action
  action = purple_menu_action_new("Edit Group", 
				  PURPLE_CALLBACK(groupme_blist_node_menu_edit_pod_cb),
				  (gpointer)account->gc->proto_data,
				  NULL);  //children
  actions = g_list_append(actions, action);

  // Delete Action
  action = purple_menu_action_new("Destroy Group",
				  PURPLE_CALLBACK(groupme_blist_node_menu_delete_cb),
				  (gpointer)account->gc->proto_data,
				  NULL);  //children
  actions = g_list_append(actions, action);

  /* Leave Action - not implemented, need membership_id
  action = purple_menu_action_new("Leave Group", 
				  PURPLE_CALLBACK(groupme_blist_node_menu_leave_cb),
				  (gpointer)account->gc->proto_data,
				  NULL);  //children
  actions = g_list_append(actions, action);
  */

  return actions;
}

static void
groupme_action_check_version_cb(PurplePluginAction *action)
{
  PurpleConnection *pc;
  GroupMeAccount *account;

  purple_debug_info("groupme", "action_check_version_cb\n");
  pc = (PurpleConnection *)action->context;
  account = (GroupMeAccount *)pc->proto_data;
  PurpleGroupMeCheckVersion(account);
}

static void
groupme_action_home_page_cb(PurplePluginAction *action)
{
  purple_debug_info("groupme", "action_home_page_cb\n");
  purple_notify_uri(groupmePlugin, GROUPME_HOMEPAGE);
}

/*
static void
groupme_action_create_pod_cb(PurplePluginAction *action)
{
  PurpleConnection *pc;
  GroupMeAccount *account;

  purple_debug_info("groupme", "action_create_pod_cb\n");
  pc = (PurpleConnection *)action->context;
  account = (GroupMeAccount *)pc->proto_data;
  GroupMeCreatePod(account);
}
*/

static void
groupme_action_check_new_pods_cb(PurplePluginAction *action)
{
  PurpleConnection *pc;
  GroupMeAccount *account;

  purple_debug_info("groupme", "action_check_new_pods_cb\n");
  pc = (PurpleConnection *)action->context;
  account = (GroupMeAccount *)pc->proto_data;
  GroupMeCheckNewPods(account);
}

static GList *
plugin_actions(PurplePlugin *plugin, gpointer context)
{
  GList *list = NULL;
  PurplePluginAction *action = NULL;

  purple_debug_info("groupme", "plugin_actions\n");

  action = purple_plugin_action_new("Check for New Groups (NOW!)", 
				    groupme_action_check_new_pods_cb);
  action->context = context;
  list = g_list_append(list, action);

  action = purple_plugin_action_new("Check for Software Updates", 
				    groupme_action_check_version_cb);
  action->context = context;
  list = g_list_append(list, action);

  action = purple_plugin_action_new("purple-groupme Home Page", 
				    groupme_action_home_page_cb);
  action->context = context;
  list = g_list_append(list, action);

  /* new group unimplemented
  action = purple_plugin_action_new("Create a New Group", 
				    groupme_action_create_pod_cb);
  action->context = context;
  list = g_list_append(list, action);
  */

  return list;
}

static void 
groupme_login(PurpleAccount *account)
{
  purple_debug_info("groupme", "login\n");
  GroupMeConnect(account);
}

static void 
groupme_close(PurpleConnection *pc)
{
  GroupMeAccount *account;

  purple_debug_info("groupme", "close\n");
  account = pc->proto_data;
  GroupMeDisconnect(account);
}

static int
groupme_send_msg(PurpleConnection *pc, 
		const char *who, const char *msg,
		PurpleMessageFlags flags)
{
  GroupMeAccount *account;
  
  purple_debug_info("groupme", "send_msg(%s <= %s\n",
		    who, msg);
  account = pc->proto_data;
  GroupMeSendMessage(account, who, msg);
  if (flags & PURPLE_MESSAGE_INVISIBLE) {
    return 0;
  }
  return 0;
}

#if PURPLE_MAJOR_VERSION >= 2 && PURPLE_MINOR_VERSION >= 5
static GHashTable *
groupme_get_account_text_table(PurpleAccount *account)
{
  GHashTable *table;
  
  purple_debug_info("groupme", "account_text_table\n");
  table = g_hash_table_new(g_str_hash, g_str_equal);
  
  g_hash_table_insert(table, "login_label", (gpointer)_("Phone # or Email Address..."));
  
  return table;
}
#endif

/******************************************************************************/
/* Plugin functions */
/******************************************************************************/

static gboolean plugin_load(PurplePlugin *plugin)
{
  PurpleCmdId id;

  purple_debug_info("groupme", "load\n");
  groupmePlugin = plugin;

  // register commands
  id = purple_cmd_register(GROUPME_CMD_SET_NICK,
			   "s",  // args
			   PURPLE_CMD_P_PRPL, 
			   PURPLE_CMD_FLAG_IM | 
			   PURPLE_CMD_FLAG_PRPL_ONLY | 
			   PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS |
			   0,
			   GROUPME_PRPL_ID, 
			   (PurpleCmdFunc)groupme_cmd_set_nick_cb, 
			   GROUPME_CMD_SET_NICK_HELP,
			   (void *)NULL); // userData
  groupmeCmdIds = g_slist_append(groupmeCmdIds, (gpointer)id);

  id = purple_cmd_register(GROUPME_CMD_SET_POD_NAME,
			   "s",  // args
			   PURPLE_CMD_P_PRPL, 
			   PURPLE_CMD_FLAG_IM | 
			   PURPLE_CMD_FLAG_PRPL_ONLY | 
			   PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS |
			   0,
			   GROUPME_PRPL_ID, 
			   (PurpleCmdFunc)groupme_cmd_set_pod_name_cb, 
			   GROUPME_CMD_SET_POD_NAME_HELP,
			   (void *)NULL); // userData
  groupmeCmdIds = g_slist_append(groupmeCmdIds, (gpointer)id);

  id = purple_cmd_register(GROUPME_CMD_DELETE_POD,
			   "",  // args
			   PURPLE_CMD_P_PRPL, 
			   PURPLE_CMD_FLAG_IM | 
			   PURPLE_CMD_FLAG_PRPL_ONLY | 
			   PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS |
			   0,
			   GROUPME_PRPL_ID, 
			   (PurpleCmdFunc)groupme_cmd_delete_cb, 
			   GROUPME_CMD_DELETE_POD_HELP,
			   (void *)NULL); // userData
  groupmeCmdIds = g_slist_append(groupmeCmdIds, (gpointer)id);

  /* leave pod unimplemented
  id = purple_cmd_register(GROUPME_CMD_LEAVE_POD,
			   "",  // args
			   PURPLE_CMD_P_PRPL, 
			   PURPLE_CMD_FLAG_IM | 
			   PURPLE_CMD_FLAG_PRPL_ONLY | 
			   PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS |
			   0,
			   GROUPME_PRPL_ID, 
			   (PurpleCmdFunc)groupme_cmd_leave_cb, 
			   GROUPME_CMD_LEAVE_POD_HELP,
			   (void *)NULL); // userData
  groupmeCmdIds = g_slist_append(groupmeCmdIds, (gpointer)id);
  */

  /* new pod unimplemented 
  id = purple_cmd_register(GROUPME_CMD_NEW_POD,
			   "",  // args
			   PURPLE_CMD_P_PRPL, 
			   PURPLE_CMD_FLAG_IM | 
			   PURPLE_CMD_FLAG_PRPL_ONLY | 
			   PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS |
			   0,
			   GROUPME_PRPL_ID, 
			   (PurpleCmdFunc)groupme_cmd_new_pod_cb, 
			   GROUPME_CMD_NEW_POD_HELP,
			   (void *)NULL); // userData
  groupmeCmdIds = g_slist_append(groupmeCmdIds, (gpointer)id);
  */

  /* add member unimplemented
  id = purple_cmd_register(GROUPME_CMD_ADD_MEMBER,
			   "s",  // args
			   PURPLE_CMD_P_PRPL, 
			   PURPLE_CMD_FLAG_IM | 
			   PURPLE_CMD_FLAG_PRPL_ONLY | 
			   PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS |
			   0,
			   GROUPME_PRPL_ID, 
			   (PurpleCmdFunc)groupme_cmd_add_cb, 
			   GROUPME_CMD_ADD_MEMBER_HELP,
			   (void *)NULL); // userData
  groupmeCmdIds = g_slist_append(groupmeCmdIds, (gpointer)id);
  */

  /* remove member unimplemented */
  /* id = purple_cmd_register(GROUPME_CMD_REMOVE_MEMBER, */
  /* 			   "",  /\* args *\/ */
  /* 			   PURPLE_CMD_P_PRPL,  */
  /* 			   PURPLE_CMD_FLAG_IM |  */
  /* 			   PURPLE_CMD_FLAG_PRPL_ONLY |  */
  /* 			   PURPLE_CMD_FLAG_ALLOW_WRONG_ARGS | */
  /* 			   0, */
  /* 			   GROUPME_PRPL_ID,  */
  /* 			   (PurpleCmdFunc)groupme_cmd_remove_cb,  */
  /* 			   GROUPME_CMD_REMOVE_MEMBER_HELP, */
  /* 			   (void *)NULL); /\* userData *\/ */
  /* groupmeCmdIds = g_slist_append(groupmeCmdIds, (gpointer)id); */

  return TRUE;
}

static gboolean plugin_unload(PurplePlugin *plugin)
{
  purple_debug_info("groupme", "unload\n");

  while (groupmeCmdIds) {
    purple_cmd_unregister((PurpleCmdId)groupmeCmdIds->data);
    groupmeCmdIds = g_slist_remove(groupmeCmdIds, 
				  (gpointer)groupmeCmdIds->data);
  }

  return TRUE;
}

static void plugin_init(PurplePlugin *plugin)
{
  PurpleAccountOption *option;
  PurplePluginInfo *info = plugin->info;
  PurplePluginProtocolInfo *prpl_info = info->extra_info;
  
  purple_debug_info("groupme", "init\n");

return;
  option = purple_account_option_string_new("Host", "host", "groupme.com");
  prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
  
  option = purple_account_option_string_new("Display Photo Link as", "photo", "(Photo)");
  prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
  
  option = purple_account_option_string_new("Display Location Link as", "location", "(*)");
  prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
  
  option = purple_account_option_bool_new("Automatically Check for Software Updates", "checkVersion", TRUE);
  prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

  option = purple_account_option_bool_new("Notify me of Beta Versions", "allowBetaVersions", FALSE);
  prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);

  option = purple_account_option_bool_new("Display Debug Messages", "debugMsgs", FALSE);
  prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
/*
  option = purple_account_option_string_new("debug 1", "debug1", "");
  prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
  
  option = purple_account_option_string_new("debug 2", "debug2", "");
  prpl_info->protocol_options = g_list_append(prpl_info->protocol_options, option);
*/
}

static PurplePluginProtocolInfo prpl_info = {
  /* options */
  0,
  
  NULL,                   /* user_splits */
  NULL,                   /* protocol_options */
  NO_BUDDY_ICONS          /* icon_spec */
  /*{"jpg", 0, 0, 50, 50, -1, PURPLE_ICON_SCALE_SEND}*/, /* icon_spec */
  groupme_list_icon,       /* list_icon */
  NULL,                   /* list_emblems */
  NULL,                   /* status_text */
  NULL,                   /* tooltip_text */
  groupme_statuses,        /* status_types */
  groupme_blist_node_menu, /* blist_node_menu */
  NULL,                   /* chat_info */
  NULL,                   /* chat_info_defaults */
  groupme_login,           /* login */
  groupme_close,           /* close */
  groupme_send_msg,        /* send_im */
  NULL,                   /* set_info */
  NULL,                   /* send_typing */
  NULL,                   /* get_info */
  NULL,                   /* set_status */
  NULL,                   /* set_idle */
  NULL,                   /* change_passwd */
  NULL,                   /* add_buddy */
  NULL,                   /* add_buddies */
  NULL,                   /* remove_buddy */
  NULL,                   /* remove_buddies */
  NULL,                   /* add_permit */
  NULL,                   /* add_deny */
  NULL,                   /* rem_permit */
  NULL,                   /* rem_deny */
  NULL,                   /* set_permit_deny */
  NULL,                   /* join_chat */
  NULL,                   /* reject chat invite */
  NULL,                   /* get_chat_name */
  NULL,                   /* chat_invite */
  NULL,                   /* chat_leave */
  NULL,                   /* chat_whisper */
  NULL,                   /* chat_send */
  NULL,                   /* keepalive */
  NULL,                   /* register_user */
  NULL,                   /* get_cb_info */
  NULL,                   /* get_cb_away */
  NULL,                   /* alias_buddy */
  NULL,                   /* group_buddy */
  NULL,                   /* rename_group */
  NULL,                   /* buddy_free */
  NULL,                   /* convo_closed */
  purple_normalize_nocase,/* normalize */
  NULL,                   /* set_buddy_icon */
  NULL,                   /* remove_group */
  NULL,                   /* get_cb_real_name */
  NULL,                   /* set_chat_topic */
  NULL,                   /* find_blist_chat */
  NULL,                   /* roomlist_get_list */
  NULL,                   /* roomlist_cancel */
  NULL,                   /* roomlist_expand_category */
  NULL,                   /* can_receive_file */
  NULL,                   /* send_file */
  NULL,                   /* new_xfer */
  NULL,                   /* offline_message */
  NULL,                   /* whiteboard_prpl_ops */
  NULL,                   /* send_raw */
  NULL,                   /* roomlist_room_serialize */
  NULL,                   /* unregister_user */
  NULL,                   /* send_attention */
  NULL,                   /* attention_types */
#if PURPLE_MAJOR_VERSION >= 2 && PURPLE_MINOR_VERSION >= 5
  sizeof(PurplePluginProtocolInfo), /* struct_size */
  groupme_get_account_text_table, /* get_account_text_table */
#else
  (gpointer) sizeof(PurplePluginProtocolInfo)
#endif
};

static PurplePluginInfo plugin_info = {
  PURPLE_PLUGIN_MAGIC,
  PURPLE_MAJOR_VERSION,
  PURPLE_MINOR_VERSION,
  PURPLE_PLUGIN_PROTOCOL, /* type */
  NULL, /* ui_requirement */
  0, /* flags */
  NULL, /* dependencies */
  PURPLE_PRIORITY_DEFAULT, /* priority */
  GROUPME_PRPL_ID, /* id */
  "GroupMe", /* name */
  GROUPME_PLUGIN_VERSION, /* version */
  N_("GroupMe Protocol Plugin"), /* summary */
  N_("GroupMe Protocol Plugin"), /* description */
  "Ken Patel <kpatelPro@gmail.com>", /* author */
  GROUPME_HOMEPAGE, /* homepage */
  plugin_load, /* load */
  plugin_unload, /* unload */
  NULL, /* destroy */
  NULL, /* ui_info */
  &prpl_info, /* extra_info */
  NULL, /* prefs_info */
  plugin_actions, /* actions */
  
  /* padding */
  NULL,
  NULL,
  NULL,
  NULL
};

PURPLE_INIT_PLUGIN(groupme, plugin_init, plugin_info);
