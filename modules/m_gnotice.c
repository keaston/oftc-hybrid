/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  m_gnotice.c: Sends a NOTICE to all opers on the network
 *
 *  Copyright (C) 2002 Stuart Walsh
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 */

#include "stdinc.h"
#include "client.h"
#include "ircd.h"
#include "numeric.h"
#include "conf.h"
#include "s_serv.h"
#include "send.h"
#include "parse.h"
#include "modules.h"
#include "send.h"
#include "channel.h"
#include "channel_mode.h"
#include "hash.h"
#include "packet.h"
#include "irc_string.h"

static void
ms_gnotice(struct Client *client_p, struct Client *source_p, int parc,
           char *parv[])
{
  char *message;
  int flags, level;

  message = parv[3];
  level = atoi(parv[2]);
  flags = atoi(parv[1]);

  if (EmptyString(message))
  {
    sendto_one(source_p, form_str(ERR_NEEDMOREPARAMS),
               me.name, parv[0], "GNOTICE");
    return;
  }
  
  sendto_realops_remote(source_p, flags, level, message);

  sendto_server(client_p, CAP_TS6, NOCAPS, ":%s GNOTICE %d %d :%s", ID(source_p), 
                flags, level, message);
  sendto_server(client_p, NOCAPS, CAP_TS6, ":%s GNOTICE %d %d :%s", 
                source_p->name, flags, level, message);
}

struct Message gnotice_msgtab =
{
  "GNOTICE", 0, 0, 3, MAXPARA, MFLG_SLOW, 0L,
  { m_ignore, m_ignore, ms_gnotice, m_ignore, m_ignore, m_ignore }
};

void
module_init()
{
  mod_add_cmd(&gnotice_msgtab);
}

void
module_exit()
{
  mod_del_cmd(&gnotice_msgtab);
}

IRCD_EXPORT struct module module_entry =
{
  { NULL, NULL, NULL },
  NULL,
  "$Revision$",
  NULL,
  module_init,
  module_exit,
  0
};
