/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  m_webirc.c: Makes CGI:IRC users appear as coming from their real host
 *
 *  Copyright (C) 1990 Jarkko Oikarinen and University of Oulu, Co Center
 *  Copyright (C) 2002-2006 ircd-ratbox development team
 *  Copyright (C) 1996-2012 Hybrid Development Team
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
#include "ircd.h"
#include "numeric.h"
#include "send.h"
#include "irc_string.h"
#include "hash.h"
#include "parse.h"
#include "modules.h"
#include "conf.h"
#include "hostmask.h"
#include "s_bsd.h"

/*
 * Usage:
 *
 * auth {
 *   user = "webirc@<cgiirc ip>"; # if identd used, put ident username instead
 *   password = "<password>"; # encryption possible
 *   spoof = "webirc."
 *   class = "users";
 *   encrypted = yes; # [Using encryption is highly recommended]
 * };
 *
 * Possible flags:
 *   kline_exempt - k/g lines on the cgiirc ip are ignored
 *
 * dlines are checked on the cgiirc ip (of course).
 * k/d/g/x lines, auth blocks, user limits, etc are checked using the
 * real host/ip.
 *
 * The password should be specified unencrypted in webirc_password in
 * cgiirc.config
 */

static int
invalid_hostname(const char *hostname)
{
  const char *p = hostname;
  unsigned int has_sep = 0;

  assert(p != NULL);

  if (*p == '.' || *p == ':')
    return 1;

  for (; *p; ++p)
  {
    if (!IsHostChar(*p))
      return 1;

    if (*p == '.' || *p == ':')
      ++has_sep;
  }

  return !has_sep;
}

/*
 * mr_webirc
 *      parv[0] = sender prefix
 *      parv[1] = password
 *      parv[2] = fake username (we ignore this)
 *      parv[3] = fake hostname
 *      parv[4] = fake ip
 */
static void
mr_webirc(struct Client *client_p, struct Client *source_p, int parc,
          char *parv[])
{
  struct AccessItem *aconf = NULL;
  struct ConfItem *conf = NULL;
  char original_sockhost[HOSTIPLEN + 1];

  assert(source_p == client_p);

  if (invalid_hostname(parv[4]))
    return;

  aconf = find_address_conf(source_p->host,
                            IsGotId(source_p) ? source_p->username : "webirc",
                            &source_p->ip,
                            source_p->ip.ss_family, parv[1],
                            source_p->certfp);

  if (aconf == NULL || !IsConfClient(aconf))
    return;

  conf = unmap_conf_item(aconf);

  if (!IsConfDoSpoofIp(aconf) || irccmp(conf->name, "webirc."))
  {
    sendto_realops_flags(UMODE_UNAUTH, L_ALL, "Not a CGI:IRC auth block: %s",
                         source_p->sockhost);
    return;
  }

  if (EmptyString(aconf->passwd))
  {
    sendto_realops_flags(UMODE_UNAUTH, L_ALL,
                         "CGI:IRC auth blocks must have a password");
    return;
  }

  if (!match_conf_password(parv[1], NULL, aconf))
  {
    sendto_realops_flags(UMODE_UNAUTH, L_ALL, "CGI:IRC password incorrect");
    return;
  }

  string_to_ip(parv[4], 0, &source_p->ip);

  strlcpy(original_sockhost, source_p->sockhost, sizeof(original_sockhost));
  strlcpy(source_p->sockhost, parv[4], sizeof(source_p->sockhost));

  if (strlen(parv[3]) <= HOSTLEN)
    strlcpy(source_p->host, parv[3], sizeof(source_p->host));
  else
    strlcpy(source_p->host, source_p->sockhost, sizeof(source_p->host));

  /* Check dlines now, klines will be checked on registration */
  if ((aconf = find_dline_conf(&client_p->ip,
                               client_p->ip.ss_family)))
  {
    if (!(aconf->status & CONF_EXEMPTDLINE))
    {
      exit_client(client_p, &me, "D-lined");
      return;
    }
  }

  sendto_realops_flags(UMODE_CCONN, L_ALL,
                       "CGI:IRC host/IP set %s to %s (%s)", original_sockhost,
                       parv[3], parv[4]);
}

static struct Message webirc_msgtab =
{
  "WEBIRC", 0, 0, 5, MAXPARA, MFLG_SLOW, 0,
  { mr_webirc, m_ignore, m_ignore, m_ignore, m_ignore, m_ignore }
};

static void
module_init()
{
  mod_add_cmd(&webirc_msgtab);
}

static void
module_exit()
{
  mod_del_cmd(&webirc_msgtab);
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
