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

#ifndef BELUGA_PROTOCOL_H
#define BELUGA_PROTOCOL_H

#include "libbeluga.h"

void
PurpleBelugaCheckVersion(BelugaAccount *account);

void
BelugaSetBuddyIcon(BelugaAccount *account,
		   BelugaPod *pod,
		   gchar *data, gsize data_len);

void
BelugaConnect(PurpleAccount *account);

void
BelugaDisconnect(BelugaAccount *account);

void
BelugaCreatePod(BelugaAccount *account);

void
BelugaCheckNewPods(BelugaAccount *account);

void
BelugaPodSetName(BelugaAccount *account,
		 BelugaPod *pod,
		 const gchar *name);

void
BelugaPodSetLocation(BelugaAccount *account,
		     BelugaPod *pod,
		     const gchar *location);

void
BelugaPodSetAddress(BelugaAccount *account,
		    BelugaPod *pod,
		    const gchar *address);

void
BelugaPodAddMembers(BelugaAccount *account, 
		    BelugaPod *pod, 
		    gchar *argsString);

void
BelugaPodRemoveMembers(BelugaAccount *account,
		    BelugaPod *pod, 
		    gchar *argsString);

void
BelugaPodRemoveMember(BelugaAccount *account,
		      BelugaPod *pod,
		      const gchar *uid);

void
BelugaPodLeave(BelugaAccount *account,
	       BelugaPod *pod);

void
BelugaHintMsg(BelugaAccount *account,
	      BelugaPod *pod,
	      const gchar *msg);

void
BelugaSendMessage(BelugaAccount *account,
		  const char *podId,
		  const char *msg);

#endif /* BELUGA_PROTOCOL_H */
