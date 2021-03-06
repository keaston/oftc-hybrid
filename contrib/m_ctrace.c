/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  m_ctrace.c: Traces a given class
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
#include "list.h"
#include "client.h"
#include "hash.h"
#include "irc_string.h"
#include "ircd.h"
#include "numeric.h"
#include "s_bsd.h"
#include "conf.h"
#include "s_serv.h"
#include "send.h"
#include "parse.h"
#include "modules.h"

static void do_ctrace(struct Client *, int, char *[]);
static void report_this_status(struct Client *, struct Client *);


/*
** mo_ctrace
**      parv[0] = sender prefix
**      parv[1] = classname
*/
static void
mo_ctrace(struct Client *client_p, struct Client *source_p,
          int parc, char *parv[])
{
  if (EmptyString(parv[1]))
  {
    sendto_one(source_p, form_str(ERR_NEEDMOREPARAMS),
               me.name, source_p->name, "CTRACE");
    return;
  }

  do_ctrace(source_p, parc, parv);
}

/*
 * do_ctrace
 */
static void
do_ctrace(struct Client *source_p, int parc, char *parv[])
{
  char *class_looking_for = parv[1];
  const char *class_name = NULL;
  dlink_node *ptr;

  sendto_realops_flags(UMODE_SPY, L_ALL,
                       "CTRACE requested by %s (%s@%s) [%s]",
                       source_p->name, source_p->username,
                       source_p->host, source_p->servptr->name);

  /* report all direct connections */
  DLINK_FOREACH(ptr, local_client_list.head)
  {
    struct Client *target_p = ptr->data;

    class_name = get_client_class(target_p);

    if ((class_name != NULL) && match(class_looking_for, class_name))
      report_this_status(source_p, target_p);
  }

  sendto_one(source_p, form_str(RPL_ENDOFTRACE), me.name,
             source_p->name, class_looking_for);
}

/*
 * report_this_status
 *
 * inputs  - pointer to client to report to
 *     - pointer to client to report about
 * output  - counter of number of hits
 * side effects - NONE
 */
static void
report_this_status(struct Client *source_p, struct Client *target_p)
{
  const char *name = NULL;
  const char *class_name = NULL;

  name = get_client_name(target_p, HIDE_IP);
  class_name = get_client_class(target_p);

  switch (target_p->status)
  {
    case STAT_CLIENT:

      if ((HasUMode(source_p, UMODE_OPER) &&
           (MyClient(source_p) || !HasUMode(target_p, UMODE_INVISIBLE)))
          || HasUMode(target_p, UMODE_OPER))
      {
        if (HasUMode(target_p, UMODE_ADMIN) && !ConfigFileEntry.hide_spoof_ips)
          sendto_one(source_p, form_str(RPL_TRACEOPERATOR),
                     me.name, source_p->name, class_name, name,
                     HasUMode(source_p, UMODE_ADMIN) ? target_p->sockhost : "255.255.255.255",
                     CurrentTime - target_p->localClient->lasttime,
                     idle_time_get(source_p, target_p));
        else if (HasUMode(target_p, UMODE_OPER))
        {
          if (ConfigFileEntry.hide_spoof_ips)
            sendto_one(source_p, form_str(RPL_TRACEOPERATOR),
                       me.name, source_p->name, class_name, name,
                       IsIPSpoof(target_p) ? "255.255.255.255" : target_p->sockhost,
                       CurrentTime - target_p->localClient->lasttime,
                       idle_time_get(source_p, target_p));
          else
            sendto_one(source_p, form_str(RPL_TRACEOPERATOR),
                       me.name, source_p->name, class_name, name,
                       (IsIPSpoof(target_p) ? "255.255.255.255" : target_p->sockhost),
                       CurrentTime - target_p->localClient->lasttime,
                       idle_time_get(source_p, target_p));
        }
        else
        {
          if (ConfigFileEntry.hide_spoof_ips)
            sendto_one(source_p, form_str(RPL_TRACEUSER),
                       me.name, source_p->name, class_name, name,
                       IsIPSpoof(target_p) ? "255.255.255.255" : target_p->sockhost,
                       CurrentTime - target_p->localClient->lasttime,
                       idle_time_get(source_p, target_p));
          else
            sendto_one(source_p, form_str(RPL_TRACEUSER),
                       me.name, source_p->name, class_name, name,
                       (IsIPSpoof(target_p) ? "255.255.255.255" : target_p->sockhost),
                       CurrentTime - target_p->localClient->lasttime,
                       idle_time_get(source_p, target_p));
        }
      }

      break;

    case STAT_SERVER:
      if (!HasUMode(source_p, UMODE_ADMIN))
        name = get_client_name(target_p, MASK_IP);

      sendto_one(source_p, form_str(RPL_TRACESERVER),
                 me.name, source_p->name, class_name, 0,
                 0, name, *(target_p->serv->by) ?
                 target_p->serv->by : "*", "*",
                 me.name, CurrentTime - target_p->localClient->lasttime);
      break;

    default: /* ...we actually shouldn't come here... --msa */
      sendto_one(source_p, form_str(RPL_TRACENEWTYPE), me.name,
                 source_p->name, name);
      break;
  }
}

static struct Message ctrace_msgtab =
{
  "CTRACE", 0, 0, 2, MAXPARA, MFLG_SLOW, 0,
  {m_unregistered, m_not_oper, m_ignore, m_ignore, mo_ctrace, m_ignore}
};

static void
module_init()
{
  mod_add_cmd(&ctrace_msgtab);
}

static void
module_exit()
{
  mod_del_cmd(&ctrace_msgtab);
}

struct module module_entry =
{
  .node    = { NULL, NULL, NULL },
  .name    = NULL,
  .version = "$Revision$",
  .handle  = NULL,
  .modinit = module_init,
  .modexit = module_exit,
  .flags   = 0
};
