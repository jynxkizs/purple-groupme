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

#ifndef GROUPME_PROTOCOL_H
#define GROUPME_PROTOCOL_H

#include "libgroupme.h"

void
PurpleGroupMeCheckVersion(GroupMeAccount *account);

void
GroupMeSetBuddyIcon(GroupMeAccount *account,
		   GroupMePod *pod,
		   gchar *data, gsize data_len);

void
GroupMeConnect(PurpleAccount *account);

void
GroupMeDisconnect(GroupMeAccount *account);

void
GroupMeCreatePod(GroupMeAccount *account);

void
GroupMeCheckNewPods(GroupMeAccount *account);

void
GroupMePodSetNick(GroupMeAccount *account,
		 GroupMePod *pod,
		 const gchar *nick);

void
GroupMePodSetName(GroupMeAccount *account,
		 GroupMePod *pod,
		 const gchar *name);
/*
void
GroupMePodAddMembers(GroupMeAccount *account, 
		    GroupMePod *pod, 
		    gchar *argsString);

void
GroupMePodRemoveMembers(GroupMeAccount *account,
		    GroupMePod *pod, 
		    gchar *argsString);

void
GroupMePodRemoveMember(GroupMeAccount *account,
		      GroupMePod *pod,
		      const gchar *uid);

void
GroupMePodLeave(GroupMeAccount *account,
	       GroupMePod *pod);
*/

void
GroupMePodDelete(GroupMeAccount *account,
		 GroupMePod *pod);

void
GroupMeHintMsg(GroupMeAccount *account,
	      GroupMePod *pod,
	      const gchar *msg);

void
GroupMeSendMessage(GroupMeAccount *account,
		  const char *podId,
		  const char *msg);

#endif /* GROUPME_PROTOCOL_H */
