/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  m_svinfo.c: Sends TS information for clock & compatibility checks.
 *
 *  Copyright (C) 2002 by the past and present ircd coders, and others.
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
 *  $Id$
 */

#include "stdinc.h"
#include "client.h"
#include "irc_string.h"
#include "ircd.h"
#include "numeric.h"
#include "send.h"
#include "conf.h"
#include "log.h"
#include "parse.h"
#include "modules.h"


/*
 * ms_svinfo - SVINFO message handler
 *      parv[0] = sender prefix
 *      parv[1] = TS_CURRENT for the server
 *      parv[2] = TS_MIN for the server
 *      parv[3] = server is standalone or connected to non-TS only
 *      parv[4] = server's idea of UTC time
 */
static void
ms_svinfo(struct Client *client_p, struct Client *source_p,
          int parc, char *parv[])
{
  time_t deltat;
  time_t theirtime;
  char timestamp[IRCD_BUFSIZE / 2];
  char theirtimestamp[IRCD_BUFSIZE / 2];

  if (MyConnect(source_p) && IsUnknown(source_p))
  {
    exit_client(source_p, source_p, "Need SERVER before SVINFO");
    return;
  }

  if (!IsServer(source_p) || !MyConnect(source_p) || parc < 5)
    return;

  if (TS_CURRENT < atoi(parv[2]) || atoi(parv[1]) < TS_MIN)
  {
    /*
     * a server with the wrong TS version connected; since we're
     * TS_ONLY we can't fall back to the non-TS protocol so
     * we drop the link  -orabidoo
     */
    sendto_realops_flags(UMODE_ALL, L_ADMIN,
                         "Link %s dropped, wrong TS protocol version (%s,%s)",
                         get_client_name(source_p, SHOW_IP), parv[1], parv[2]);
    sendto_realops_flags(UMODE_ALL, L_OPER,
                         "Link %s dropped, wrong TS protocol version (%s,%s)",
                         get_client_name(source_p, MASK_IP), parv[1], parv[2]);
    exit_client(source_p, source_p, "Incompatible TS version");
    return;
  }

  theirtime = atol(parv[4]);
  deltat = abs(theirtime - CurrentTime);

  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S Z",
           gmtime(&CurrentTime));
  strftime(theirtimestamp, sizeof(theirtimestamp), "%Y-%m-%d %H:%M:%S Z",
           gmtime(&theirtime));

  if (deltat > ConfigFileEntry.ts_max_delta)
  {
    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "Link %s dropped, excessive TS delta (my TS=%lu (%s), their TS=%lu (%s), delta=%d)",
                         get_client_name(source_p, SHOW_IP),
                         (unsigned long) CurrentTime,
                         timestamp,
                         (unsigned long) theirtime,
                         theirtimestamp,
                         (int) deltat);
    ilog(LOG_TYPE_IRCD,
         "Link %s dropped, excessive TS delta (my TS=%lu (%s), their TS=%lu (%s), delta=%d)",
         get_client_name(source_p, SHOW_IP),
         (unsigned long) CurrentTime,
         timestamp,
         (unsigned long) theirtime,
         theirtimestamp,
         (int) deltat);
    exit_client(source_p, source_p, "Excessive TS delta");
    return;
  }

  if (deltat > ConfigFileEntry.ts_warn_delta)
  {
    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "Link %s notable TS delta (my TS=%lu (%s), their TS=%lu (%s), delta=%d)",
                         source_p->name,
                         (unsigned long) CurrentTime,
                         timestamp,
                         (unsigned long) theirtime,
                         theirtimestamp,
                         (int) deltat);
  }
}

static struct Message svinfo_msgtab =
{
  "SVINFO", 0, 0, 4, MAXPARA, MFLG_SLOW, 0,
  {m_unregistered, m_ignore, ms_svinfo, m_ignore, m_ignore, m_ignore}
};

static void
module_init()
{
  mod_add_cmd(&svinfo_msgtab);
}

static void
module_exit()
{
  mod_del_cmd(&svinfo_msgtab);
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
