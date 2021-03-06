/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  m_dline.c: Bans a user.
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
#include "channel.h"
#include "client.h"
#include "irc_string.h"
#include "sprintf_irc.h"
#include "ircd.h"
#include "hostmask.h"
#include "numeric.h"
#include "fdlist.h"
#include "s_bsd.h"
#include "conf.h"
#include "log.h"
#include "s_misc.h"
#include "send.h"
#include "hash.h"
#include "s_serv.h"
#include "parse.h"
#include "modules.h"


static int remove_tdline_match(const char *);

/* static int remove_tdline_match(const char *host, const char *user)
 * Input: An ip to undline.
 * Output: returns YES on success, NO if no tdline removed.
 * Side effects: Any matching tdlines are removed.
 */
static int
remove_tdline_match(const char *host)
{
  struct sockaddr_storage iphost, *piphost;
  struct AccessItem *aconf;
  int t;

  if ((t = parse_netmask(host, &iphost, NULL)) != HM_HOST)
  {
    if (t == HM_IPV6)
      t = AF_INET6;
    else
      t = AF_INET;

    piphost = &iphost;
  }
  else
  {
    t = 0;
    piphost = NULL;
  }

  if ((aconf = find_conf_by_address(host, piphost, CONF_DLINE | 1, t, NULL, NULL,
                                    0, NULL)))
  {
    if (IsConfTemporary(aconf))
    {
      delete_one_address_conf(host, aconf);
      return 1;
    }
  }

  return 0;
}

/* mo_dline()
 *
 * inputs  - pointer to server
 *    - pointer to client
 *    - parameter count
 *    - parameter list
 * output  -
 * side effects - D line is added
 *
 */
static void
mo_dline(struct Client *client_p, struct Client *source_p,
         int parc, char *parv[])
{
  char def_reason[] = CONF_NOREASON;
  char *dlhost = NULL, *oper_reason = NULL, *reason = NULL;
  char *target_server = NULL;
  const char *creason;
  const struct Client *target_p = NULL;
  struct sockaddr_storage daddr;
  struct AccessItem *aconf = NULL;
  time_t tkline_time = 0;
  int bits, t;
  char hostip[HOSTIPLEN + 1];

  if (!HasOFlag(source_p, OPER_FLAG_DLINE))
  {
    sendto_one(source_p, form_str(ERR_NOPRIVS),
               me.name, source_p->name, "dline");
    return;
  }

  if (parse_aline("DLINE", source_p,  parc, parv, AWILD, &dlhost,
                  NULL, &tkline_time, &target_server, &reason) < 0)
    return;

  if (target_server != NULL)
  {
    if (HasID(source_p))
    {
      sendto_server(NULL, CAP_DLN | CAP_TS6, NOCAPS,
                    ":%s DLINE %s %lu %s :%s",
                    source_p->id, target_server, (unsigned long)tkline_time,
                    dlhost, reason);
      sendto_server(NULL, CAP_DLN, CAP_TS6,
                    ":%s DLINE %s %lu %s :%s",
                    source_p->name, target_server, (unsigned long)tkline_time,
                    dlhost, reason);
    }
    else
      sendto_server(NULL, CAP_DLN, NOCAPS,
                    ":%s DLINE %s %lu %s :%s",
                    source_p->name, target_server, (unsigned long)tkline_time,
                    dlhost, reason);

    /* Allow ON to apply local kline as well if it matches */
    if (!match(target_server, me.name))
      return;
  }
  else
    cluster_a_line(source_p, "DLINE", CAP_DLN, SHARED_DLINE,
                   "%d %s :%s", tkline_time, dlhost, reason);

  if ((t = parse_netmask(dlhost, NULL, &bits)) == HM_HOST)
  {
    if ((target_p = find_chasing(client_p, source_p, dlhost, NULL)) == NULL)
      return;

    if (!MyConnect(target_p))
    {
      sendto_one(source_p,
                 ":%s NOTICE %s :Can't DLINE nick on another server",
                 me.name, source_p->name);
      return;
    }

    if (IsExemptKline(target_p))
    {
      sendto_one(source_p,
                 ":%s NOTICE %s :%s is E-lined", me.name,
                 source_p->name, target_p->name);
      return;
    }

    ip_to_string(&target_p->ip, hostip, sizeof(hostip));
    dlhost = hostip;
    t = parse_netmask(dlhost, NULL, &bits);
    assert(t == HM_IPV4 || t == HM_IPV6);
  }

  if (bits < 8)
  {
    sendto_one(source_p,
               ":%s NOTICE %s :For safety, bitmasks less than 8 require conf access.",
               me.name, source_p->name);
    return;
  }

  if (t == HM_IPV6)
    t = AF_INET6;
  else
    t = AF_INET;

  parse_netmask(dlhost, &daddr, NULL);

  if ((aconf = find_dline_conf(&daddr, t)) != NULL)
  {
    creason = aconf->reason ? aconf->reason : def_reason;

    if (IsConfExemptKline(aconf))
      sendto_one(source_p,
                 ":%s NOTICE %s :[%s] is (E)d-lined by [%s] - %s",
                 me.name, source_p->name, dlhost, aconf->host, creason);
    else
      sendto_one(source_p,
                 ":%s NOTICE %s :[%s] already D-lined by [%s] - %s",
                 me.name, source_p->name, dlhost, aconf->host, creason);

    return;
  }

  /* Look for an oper reason */
  if ((oper_reason = strchr(reason, '|')) != NULL)
    * oper_reason++ = '\0';

  if (!valid_comment(source_p, reason, 1))
    return;

  apply_conf_ban(source_p, DLINE_TYPE, NULL, dlhost, reason, oper_reason,
                 tkline_time);
}

static void
ms_dline(struct Client *client_p, struct Client *source_p,
         int parc, char *parv[])
{
  char def_reason[] = CONF_NOREASON;
  char *dlhost, *oper_reason, *reason;
  const char *creason;
  const struct Client *target_p = NULL;
  struct sockaddr_storage daddr;
  struct AccessItem *aconf = NULL;
  time_t tkline_time = 0;
  int bits, t;
  char hostip[HOSTIPLEN + 1];

  if (parc != 5 || EmptyString(parv[4]))
    return;

  /* parv[0]  parv[1]        parv[2]      parv[3]  parv[4] */
  /* oper     target_server  tkline_time  host     reason  */
  sendto_match_servs(source_p, parv[1], CAP_DLN,
                     "DLINE %s %s %s :%s",
                     parv[1], parv[2], parv[3], parv[4]);

  if (!match(parv[1], me.name))
    return;

  tkline_time = valid_tkline(parv[2], TK_SECONDS);
  dlhost = parv[3];
  reason = parv[4];

  if (HasFlag(source_p, FLAGS_SERVICE)
      || find_matching_name_conf(ULINE_TYPE, source_p->servptr->name,
                                 source_p->username, source_p->host,
                                 SHARED_DLINE))
  {
    if (!IsClient(source_p))
      return;

    if ((t = parse_netmask(dlhost, NULL, &bits)) == HM_HOST)
    {
      if ((target_p = find_chasing(client_p, source_p, dlhost, NULL)) == NULL)
        return;

      if (!MyConnect(target_p))
      {
        sendto_one(source_p,
                   ":%s NOTICE %s :Can't DLINE nick on another server",
                   me.name, source_p->name);
        return;
      }

      if (IsExemptKline(target_p))
      {
        sendto_one(source_p,
                   ":%s NOTICE %s :%s is E-lined", me.name,
                   source_p->name, target_p->name);
        return;
      }

      ip_to_string(&target_p->ip, hostip, sizeof(hostip));

      dlhost = hostip;
      t = parse_netmask(dlhost, NULL, &bits);
      assert(t == HM_IPV4 || t == HM_IPV6);
    }

    if (bits < 8)
    {
      sendto_one(source_p,
                 ":%s NOTICE %s :For safety, bitmasks less than 8 require conf access.",
                 me.name, source_p->name);
      return;
    }

    if (t == HM_IPV6)
      t = AF_INET6;
    else
      t = AF_INET;

    parse_netmask(dlhost, &daddr, NULL);

    if ((aconf = find_dline_conf(&daddr, t)) != NULL)
    {
      creason = aconf->reason ? aconf->reason : def_reason;

      if (IsConfExemptKline(aconf))
        sendto_one(source_p,
                   ":%s NOTICE %s :[%s] is (E)d-lined by [%s] - %s",
                   me.name, source_p->name, dlhost, aconf->host, creason);
      else
        sendto_one(source_p,
                   ":%s NOTICE %s :[%s] already D-lined by [%s] - %s",
                   me.name, source_p->name, dlhost, aconf->host, creason);

      return;
    }

    /* Look for an oper reason */
    if ((oper_reason = strchr(reason, '|')) != NULL)
      * oper_reason++ = '\0';

    if (!valid_comment(source_p, reason, 1))
      return;

    apply_conf_ban(source_p, DLINE_TYPE, NULL, dlhost, reason, oper_reason,
                   tkline_time);
  }
}

/*
** m_undline
** added May 28th 2000 by Toby Verrall <toot@melnet.co.uk>
** based totally on m_unkline
** added to hybrid-7 7/11/2000 --is
**
**      parv[0] = sender nick
**      parv[1] = dline to remove
*/
static void
mo_undline(struct Client *client_p, struct Client *source_p,
           int parc, char *parv[])
{
  char *addr = NULL, *user = NULL;
  char *target_server = NULL;

  if (!HasOFlag(source_p, OPER_FLAG_UNDLINE))
  {
    sendto_one(source_p, form_str(ERR_NOPRIVS),
               me.name, source_p->name, "undline");
    return;
  }

  if (parc < 2 || EmptyString(parv[1]))
  {
    sendto_one(source_p, form_str(ERR_NEEDMOREPARAMS),
               me.name, source_p->name, "UNDLINE");
    return;
  }

  if (parse_aline("UNDLINE", source_p, parc, parv, 0, &user,
                  &addr, NULL, &target_server, NULL) < 0)
    return;

  if (target_server != NULL)
  {
    sendto_match_servs(source_p, target_server, CAP_UNDLN,
                       "UNDLINE %s %s", target_server, addr);

    /* Allow ON to apply local unkline as well if it matches */
    if (!match(target_server, me.name))
      return;
  }
  else
    cluster_a_line(source_p, "UNDLINE", CAP_UNDLN, SHARED_UNDLINE,
                   "%s", addr);

  if (remove_tdline_match(addr))
  {
    sendto_one(source_p,
               ":%s NOTICE %s :Un-Dlined [%s] from temporary D-Lines",
               me.name, source_p->name, addr);
    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "%s has removed the temporary D-Line for: [%s]",
                         get_oper_name(source_p), addr);
    ilog(LOG_TYPE_DLINE, "%s removed temporary D-Line for [%s]",
         source_p->name, addr);
    return;
  }

  if (remove_conf_line(DLINE_TYPE, source_p, addr, NULL) > 0)
  {
    sendto_one(source_p, ":%s NOTICE %s :D-Line for [%s] is removed",
               me.name, source_p->name, addr);
    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "%s has removed the D-Line for: [%s]",
                         get_oper_name(source_p), addr);
    ilog(LOG_TYPE_DLINE, "%s removed D-Line for [%s]",
         get_oper_name(source_p), addr);
  }
  else
    sendto_one(source_p, ":%s NOTICE %s :No D-Line for [%s] found",
               me.name, source_p->name, addr);
}

static void
me_undline(struct Client *client_p, struct Client *source_p,
           int parc, char *parv[])
{
  const char *addr = NULL;

  if (parc != 3 || EmptyString(parv[2]))
    return;

  addr = parv[2];

  if (!IsClient(source_p) || !match(parv[1], me.name))
    return;

  if (HasFlag(source_p, FLAGS_SERVICE) || find_matching_name_conf(ULINE_TYPE,
      source_p->servptr->name,
      source_p->username, source_p->host,
      SHARED_UNDLINE))
  {
    if (remove_tdline_match(addr))
    {
      sendto_one(source_p,
                 ":%s NOTICE %s :Un-Dlined [%s] from temporary D-Lines",
                 me.name, source_p->name, addr);
      sendto_realops_flags(UMODE_ALL, L_ALL,
                           "%s has removed the temporary D-Line for: [%s]",
                           get_oper_name(source_p), addr);
      ilog(LOG_TYPE_DLINE, "%s removed temporary D-Line for [%s]",
           source_p->name, addr);
      return;
    }

    if (remove_conf_line(DLINE_TYPE, source_p, addr, NULL) > 0)
    {
      sendto_one(source_p, ":%s NOTICE %s :D-Line for [%s] is removed",
                 me.name, source_p->name, addr);
      sendto_realops_flags(UMODE_ALL, L_ALL,
                           "%s has removed the D-Line for: [%s]",
                           get_oper_name(source_p), addr);
      ilog(LOG_TYPE_DLINE, "%s removed D-Line for [%s]",
           get_oper_name(source_p), addr);
    }
    else
      sendto_one(source_p, ":%s NOTICE %s :No D-Line for [%s] found",
                 me.name, source_p->name, addr);
  }
}

static void
ms_undline(struct Client *client_p, struct Client *source_p,
           int parc, char *parv[])
{
  if (parc != 3 || EmptyString(parv[2]))
    return;

  sendto_match_servs(source_p, parv[1], CAP_UNDLN,
                     "UNDLINE %s %s",
                     parv[1], parv[2]);

  me_undline(client_p, source_p, parc, parv);
}

static struct Message dline_msgtab =
{
  "DLINE", 0, 0, 2, MAXPARA, MFLG_SLOW, 0,
  {m_unregistered, m_not_oper, ms_dline, m_ignore, mo_dline, m_ignore}
};

static struct Message undline_msgtab =
{
  "UNDLINE", 0, 0, 2, MAXPARA, MFLG_SLOW, 0,
  {m_unregistered, m_not_oper, ms_undline, m_ignore, mo_undline, m_ignore}
};

static void
module_init()
{
  mod_add_cmd(&dline_msgtab);
  mod_add_cmd(&undline_msgtab);
  add_capability("DLN", CAP_DLN, 1);
  add_capability("UNDLN", CAP_UNDLN, 1);
}

static void
module_exit()
{
  mod_del_cmd(&dline_msgtab);
  mod_del_cmd(&undline_msgtab);
  delete_capability("UNDLN");
  delete_capability("DLN");
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
