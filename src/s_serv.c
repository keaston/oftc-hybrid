/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  s_serv.c: Server related functions.
 *
 *  Copyright (C) 2005 by the past and present ircd coders, and others.
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
#ifdef HAVE_LIBCRYPTO
#include <openssl/rsa.h>
#include "rsa.h"
#endif
#include "list.h"
#include "channel.h"
#include "channel_mode.h"
#include "client.h"
#include "dbuf.h"
#include "event.h"
#include "fdlist.h"
#include "hash.h"
#include "irc_string.h"
#include "sprintf_irc.h"
#include "ircd.h"
#include "ircd_defs.h"
#include "s_bsd.h"
#include "numeric.h"
#include "packet.h"
#include "irc_res.h"
#include "conf.h"
#include "s_serv.h"
#include "log.h"
#include "s_misc.h"
#include "s_user.h"
#include "send.h"
#include "memory.h"
#include "channel.h" /* chcap_usage_counts stuff...*/
#include "parse.h"

#define MIN_CONN_FREQ 300

static dlink_list cap_list = { NULL, NULL, 0 };
static void server_burst(struct Client *);
void burst_all(struct Client *);
static void send_tb(struct Client *client_p, struct Channel *chptr);

static CNCB serv_connect_callback;

static void burst_members(struct Client *, struct Channel *);

/*
 * write_links_file
 *
 * inputs  - void pointer which is not used
 * output  - NONE
 * side effects  - called from an event, write out list of linked servers
 *      but in no particular order.
 */
void
write_links_file(uv_timer_t *handle, int status)
{
  MessageFileLine *next_mptr = NULL;
  MessageFileLine *mptr = NULL;
  MessageFileLine *currentMessageLine = NULL;
  MessageFileLine *newMessageLine = NULL;
  MessageFile *MessageFileptr = &ConfigFileEntry.linksfile;
  FILE *file;
  char buff[512];
  dlink_node *ptr;

  if ((file = fopen(MessageFileptr->fileName, "w")) == NULL)
    return;

  for (mptr = MessageFileptr->contentsOfFile; mptr; mptr = next_mptr)
  {
    next_mptr = mptr->next;
    MyFree(mptr);
  }

  MessageFileptr->contentsOfFile = NULL;

  DLINK_FOREACH(ptr, global_serv_list.head)
  {
    const struct Client *target_p = ptr->data;

    /* skip ourselves, we send ourselves in /links */
    if (IsMe(target_p))
      continue;

    /* skip hidden servers */
    if (IsHidden(target_p))
      continue;

    newMessageLine = MyMalloc(sizeof(MessageFileLine));

    /*
     * Attempt to format the file in such a way it follows the usual links output
     * ie  "servername uplink :hops info"
     * Mostly for aesthetic reasons - makes it look pretty in mIRC ;)
     * - madmax
     */
    snprintf(newMessageLine->line, sizeof(newMessageLine->line), "%s %s :1 %s",
             target_p->name, me.name, target_p->info);

    if (MessageFileptr->contentsOfFile)
    {
      if (currentMessageLine)
        currentMessageLine->next = newMessageLine;

      currentMessageLine = newMessageLine;
    }
    else
    {
      MessageFileptr->contentsOfFile = newMessageLine;
      currentMessageLine = newMessageLine;
    }

    snprintf(buff, sizeof(buff), "%s %s :1 %s\n",
             target_p->name, me.name, target_p->info);
    fputs(buff, file);
  }

  fclose(file);
}

/* hunt_server()
 *      Do the basic thing in delivering the message (command)
 *      across the relays to the specific server (server) for
 *      actions.
 *
 *      Note:   The command is a format string and *MUST* be
 *              of prefixed style (e.g. ":%s COMMAND %s ...").
 *              Command can have only max 8 parameters.
 *
 *      server  parv[server] is the parameter identifying the
 *              target server.
 *
 *      *WARNING*
 *              parv[server] is replaced with the pointer to the
 *              real servername from the matched client (I'm lazy
 *              now --msa).
 *
 *      returns: (see #defines)
 */
int
hunt_server(struct Client *client_p, struct Client *source_p,
            const char *command,
            int server, int parc, char *parv[])
{
  struct Client *target_p = NULL;
  struct Client *target_tmp = NULL;
  dlink_node *ptr;
  int wilds;

  /* Assume it's me, if no server */
  if (parc <= server || EmptyString(parv[server]))
    return HUNTED_ISME;

  if (!strcmp(parv[server], me.id) || match(parv[server], me.name))
    return HUNTED_ISME;

  /* These are to pickup matches that would cause the following
   * message to go in the wrong direction while doing quick fast
   * non-matching lookups.
   */
  if (MyClient(source_p))
    target_p = hash_find_client(parv[server]);
  else
    target_p = find_person(client_p, parv[server]);

  if (target_p)
    if (target_p->from == source_p->from && !MyConnect(target_p))
      target_p = NULL;

  if (target_p == NULL && (target_p = hash_find_server(parv[server])))
    if (target_p->from == source_p->from && !MyConnect(target_p))
      target_p = NULL;

  collapse(parv[server]);
  wilds = has_wildcards(parv[server]);

  /* Again, if there are no wild cards involved in the server
   * name, use the hash lookup
   */
  if (target_p == NULL)
  {
    if (!wilds)
    {
      if (!(target_p = hash_find_server(parv[server])))
      {
        sendto_one(source_p, form_str(ERR_NOSUCHSERVER),
                   me.name, source_p->name, parv[server]);
        return (HUNTED_NOSUCH);
      }
    }
    else
    {
      DLINK_FOREACH(ptr, global_client_list.head)
      {
        target_tmp = ptr->data;

        if (match(parv[server], target_tmp->name))
        {
          if (target_tmp->from == source_p->from && !MyConnect(target_tmp))
            continue;

          target_p = ptr->data;

          if (IsRegistered(target_p) && (target_p != client_p))
            break;
        }
      }
    }
  }

  if (target_p != NULL)
  {
    if (!IsRegistered(target_p))
    {
      sendto_one(source_p, form_str(ERR_NOSUCHSERVER),
                 me.name, source_p->name, parv[server]);
      return HUNTED_NOSUCH;
    }

    if (IsMe(target_p) || MyClient(target_p))
      return HUNTED_ISME;

    if (!match(target_p->name, parv[server]))
      parv[server] = target_p->name;

    /* This is a little kludgy but should work... */
    if (IsClient(source_p) &&
        ((MyConnect(target_p) && IsCapable(target_p, CAP_TS6)) ||
         (!MyConnect(target_p) && IsCapable(target_p->from, CAP_TS6))))
      parv[0] = ID(source_p);

    sendto_one(target_p, command, parv[0],
               parv[1], parv[2], parv[3], parv[4],
               parv[5], parv[6], parv[7], parv[8]);
    return (HUNTED_PASS);
  }

  sendto_one(source_p, form_str(ERR_NOSUCHSERVER),
             me.name, source_p->name, parv[server]);
  return (HUNTED_NOSUCH);
}

/* try_connections()
 *
 * inputs  - void pointer which is not used
 * output  - NONE
 * side effects  -
 * scan through configuration and try new connections.
 * Returns the calendar time when the next call to this
 * function should be made latest. (No harm done if this
 * is called earlier or later...)
 */
void
try_connections(uv_timer_t *handle, int status)
{
  dlink_node *ptr;
  struct ConfItem *conf;
  struct AccessItem *aconf;
  struct ClassItem *cltmp;
  int confrq;

  /* TODO: change this to set active flag to 0 when added to event! --Habeeb */
  if (GlobalSetOptions.autoconn == 0)
    return;

  DLINK_FOREACH(ptr, server_items.head)
  {
    conf = ptr->data;
    aconf = map_to_conf(conf);

    /* Also when already connecting! (update holdtimes) --SRB
     */
    if (!(aconf->status & CONF_SERVER) || !aconf->port ||
        !(IsConfAllowAutoConn(aconf)))
      continue;

    cltmp = map_to_conf(aconf->class_ptr);

    /* Skip this entry if the use of it is still on hold until
     * future. Otherwise handle this entry (and set it on hold
     * until next time). Will reset only hold times, if already
     * made one successfull connection... [this algorithm is
     * a bit fuzzy... -- msa >;) ]
     */
    if (aconf->hold > CurrentTime)
      continue;

    if (cltmp == NULL)
      confrq = DEFAULT_CONNECTFREQUENCY;
    else
    {
      confrq = cltmp->con_freq;

      if (confrq < MIN_CONN_FREQ)
        confrq = MIN_CONN_FREQ;
    }

    aconf->hold = CurrentTime + confrq;

    /* Found a CONNECT config with port specified, scan clients
     * and see if this server is already connected?
     */
    if (hash_find_server(conf->name) != NULL)
      continue;

    if (cltmp->curr_user_count < cltmp->max_total)
    {
      /* Go to the end of the list, if not already last */
      if (ptr->next != NULL)
      {
        dlinkDelete(ptr, &server_items);
        dlinkAddTail(conf, &conf->node, &server_items);
      }

      if (find_servconn_in_progress(conf->name))
        return;

      /* We used to only print this if serv_connect() actually
       * succeeded, but since comm_tcp_connect() can call the callback
       * immediately if there is an error, we were getting error messages
       * in the wrong order. SO, we just print out the activated line,
       * and let serv_connect() / serv_connect_callback() print an
       * error afterwards if it fails.
       *   -- adrian
       */
      if (ConfigServerHide.hide_server_ips)
        sendto_realops_flags(UMODE_ALL, L_ALL, "Connection to %s activated.",
                             conf->name);
      else
        sendto_realops_flags(UMODE_ALL, L_ALL, "Connection to %s[%s] activated.",
                             conf->name, aconf->host);

      serv_connect(aconf, NULL);
      /* We connect only one at time... */
      return;
    }
  }
}

int
valid_servname(const char *name)
{
  unsigned int length = 0;
  unsigned int dots   = 0;
  const char *p = name;

  for (; *p; ++p)
  {
    if (!IsServChar(*p))
      return 0;

    ++length;

    if (*p == '.')
      ++dots;
  }

  return dots != 0 && length <= HOSTLEN;
}

int
check_server(const char *name, struct Client *client_p)
{
  dlink_node *ptr;
  struct ConfItem *conf           = NULL;
  struct ConfItem *server_conf    = NULL;
  struct AccessItem *aconf        = NULL;
  struct AccessItem *server_aconf = NULL;
  int error = -1;

  assert(client_p != NULL);

  /* loop through looking for all possible connect items that might work */
  DLINK_FOREACH(ptr, server_items.head)
  {
    conf = ptr->data;
    aconf = map_to_conf(conf);

    if (!match(name, conf->name))
      continue;

    error = -3;

    /* XXX: Fix me for IPv6                    */
    /* XXX sockhost is the IPv4 ip as a string */
    if (match(aconf->host, client_p->host) ||
        match(aconf->host, client_p->sockhost))
    {
      error = -2;

      if (!match_conf_password(client_p->localClient->passwd, client_p->certfp,
                               aconf))
        return -2;

      server_conf = conf;
    }
  }

  if (server_conf == NULL)
    return (error);

  attach_conf(client_p, server_conf);

  server_aconf = map_to_conf(server_conf);

  if (server_aconf != NULL)
  {
    struct sockaddr_in *v4;
    struct sockaddr_in6 *v6;

    switch (aconf->aftype)
    {
      case AF_INET6:
        v6 = (struct sockaddr_in6 *)&server_aconf->addr;

        if (IN6_IS_ADDR_UNSPECIFIED(&v6->sin6_addr))
          memcpy(&server_aconf->addr, &client_p->ip, 
                 sizeof(server_aconf->addr));

        break;
      case AF_INET:
        v4 = (struct sockaddr_in *)&server_aconf->addr;

        if (v4->sin_addr.s_addr == INADDR_NONE)
          memcpy(&server_aconf->addr, &client_p->ip, 
                 sizeof(server_aconf->addr));

        break;
      default:
        assert(0);
        return -1;
    }
  }

  return (0);
}

/* add_capability()
 *
 * inputs  - string name of CAPAB
 *    - int flag of capability
 * output  - NONE
 * side effects  - Adds given capability name and bit mask to
 *      current supported capabilities. This allows
 *      modules to dynamically add or subtract their capability.
 */
void
add_capability(const char *capab_name, int cap_flag, int add_to_default)
{
  struct Capability *cap = MyMalloc(sizeof(*cap));

  DupString(cap->name, capab_name);
  cap->cap = cap_flag;
  dlinkAdd(cap, &cap->node, &cap_list);

  if (add_to_default)
    default_server_capabs |= cap_flag;
}

/* delete_capability()
 *
 * inputs  - string name of CAPAB
 * output  - NONE
 * side effects  - delete given capability from ones known.
 */
int
delete_capability(const char *capab_name)
{
  dlink_node *ptr;
  dlink_node *next_ptr;
  struct Capability *cap;

  DLINK_FOREACH_SAFE(ptr, next_ptr, cap_list.head)
  {
    cap = ptr->data;

    if (cap->cap != 0)
    {
      if (irccmp(cap->name, capab_name) == 0)
      {
        default_server_capabs &= ~(cap->cap);
        dlinkDelete(ptr, &cap_list);
        MyFree(cap->name);
        cap->name = NULL;
        MyFree(cap);
      }
    }
  }

  return 0;
}

/*
 * find_capability()
 *
 * inputs  - string name of capab to find
 * output  - 0 if not found CAPAB otherwise
 * side effects  - none
 */
int
find_capability(const char *capab)
{
  const dlink_node *ptr = NULL;

  DLINK_FOREACH(ptr, cap_list.head)
  {
    const struct Capability *cap = ptr->data;

    if (cap->cap && !irccmp(cap->name, capab))
      return cap->cap;
  }

  return 0;
}

/* send_capabilities()
 *
 * inputs  - Client pointer to send to
 *    - Pointer to AccessItem (for crypt)
 *    - int flag of capabilities that this server can send
 * output  - NONE
 * side effects  - send the CAPAB line to a server  -orabidoo
 *
 */
void
send_capabilities(struct Client *client_p, struct AccessItem *aconf,
                  int cap_can_send)
{
  struct Capability *cap = NULL;
  char msgbuf[IRCD_BUFSIZE];
  char *t;
  int tl;
  dlink_node *ptr;

  t = msgbuf;

  DLINK_FOREACH(ptr, cap_list.head)
  {
    cap = ptr->data;

    if (cap->cap & (cap_can_send | default_server_capabs))
    {
      tl = ircsprintf(t, "%s ", cap->name);
      t += tl;
    }
  }

  *(t - 1) = '\0';
  sendto_one(client_p, "CAPAB :%s", msgbuf);
}

/* sendnick_TS()
 *
 * inputs  - client (server) to send nick towards
 *          - client to send nick for
 * output  - NONE
 * side effects  - NICK message is sent towards given client_p
 */
void
sendnick_TS(struct Client *client_p, struct Client *target_p)
{
  static char ubuf[12];

  if (!IsClient(target_p))
    return;

  send_umode(NULL, target_p, 0, SEND_UMODES, ubuf);

  if (ubuf[0] == '\0')
  {
    ubuf[0] = '+';
    ubuf[1] = '\0';
  }

  if (IsCapable(client_p, CAP_SVS))
  {
    if (HasID(target_p) && IsCapable(client_p, CAP_TS6))
      sendto_one(client_p, ":%s UID %s %d %lu %s %s %s %s %s %s :%s",
                 target_p->servptr->id,
                 target_p->name, target_p->hopcount + 1,
                 (unsigned long) target_p->tsinfo,
                 ubuf, target_p->username, target_p->host,
                 (MyClient(target_p) && IsIPSpoof(target_p)) ?
                 "0" : target_p->sockhost, target_p->id,
                 target_p->svid, target_p->info);
    else
      sendto_one(client_p, "NICK %s %d %lu %s %s %s %s %s :%s",
                 target_p->name, target_p->hopcount + 1,
                 (unsigned long) target_p->tsinfo,
                 ubuf, target_p->username, target_p->host,
                 target_p->servptr->name, target_p->svid,
                 target_p->info);
  }
  else
  {
    if (HasID(target_p) && IsCapable(client_p, CAP_TS6))
      sendto_one(client_p, ":%s UID %s %d %lu %s %s %s %s %s :%s",
                 target_p->servptr->id,
                 target_p->name, target_p->hopcount + 1,
                 (unsigned long) target_p->tsinfo,
                 ubuf, target_p->username, target_p->host,
                 (MyClient(target_p) && IsIPSpoof(target_p)) ?
                 "0" : target_p->sockhost, target_p->id, target_p->info);
    else
      sendto_one(client_p, "NICK %s %d %lu %s %s %s %s :%s",
                 target_p->name, target_p->hopcount + 1,
                 (unsigned long) target_p->tsinfo,
                 ubuf, target_p->username, target_p->host,
                 target_p->servptr->name, target_p->info);
  }

  if (!EmptyString(target_p->realhost))
    sendto_one(client_p, ":%s REALHOST :%s", ID_or_name(target_p, client_p), 
               target_p->realhost);

#ifdef HAVE_LIBCRYPTO

  if (!EmptyString(target_p->certfp))
    sendto_one(client_p, ":%s CERTFP :%s", ID_or_name(target_p, client_p), 
               target_p->certfp);

#endif

  if (target_p->away[0])
    sendto_one(client_p, ":%s AWAY :%s", ID_or_name(target_p, client_p),
               target_p->away);

}

/*
 * show_capabilities - show current server capabilities
 *
 * inputs       - pointer to a struct Client
 * output       - pointer to static string
 * side effects - build up string representing capabilities of server listed
 */
const char *
show_capabilities(struct Client *target_p)
{
  static char msgbuf[IRCD_BUFSIZE];
  char *t = msgbuf;
  dlink_node *ptr;

  t += ircsprintf(msgbuf, "TS ");

  DLINK_FOREACH(ptr, cap_list.head)
  {
    const struct Capability *cap = ptr->data;

    if (IsCapable(target_p, cap->cap))
      t += ircsprintf(t, "%s ", cap->name);
  }

  *(t - 1) = '\0';
  return msgbuf;
}

/* make_server()
 *
 * inputs       - pointer to client struct
 * output       - pointer to struct Server
 * side effects - add's an Server information block to a client
 *                if it was not previously allocated.
 */
struct Server *
make_server(struct Client *client_p)
{
  if (client_p->serv == NULL)
    client_p->serv = MyMalloc(sizeof(struct Server));

  return client_p->serv;
}

/* server_estab()
 *
 * inputs       - pointer to a struct Client
 * output       -
 * side effects -
 */
void
server_estab(struct Client *client_p)
{
  struct Client *target_p;
  struct ConfItem *conf;
  struct AccessItem *aconf = NULL;
  char *host;
  static char inpath_ip[HOSTLEN * 2 + USERLEN + 6];
  dlink_node *ptr;
#ifdef HAVE_LIBCRYPTO
  const COMP_METHOD *compression = NULL, *expansion = NULL;
#endif

  assert(client_p != NULL);

  strlcpy(inpath_ip, get_client_name(client_p, SHOW_IP), sizeof(inpath_ip));

  host   = client_p->name;

  if ((conf = find_conf_name(&client_p->localClient->confs, host, SERVER_TYPE))
      == NULL)
  {
    /* This shouldn't happen, better tell the ops... -A1kmm */
    sendto_realops_flags(UMODE_ALL, L_ALL, "Warning: Lost connect{} block "
                         "for server %s(this shouldn't happen)!", host);
    exit_client(client_p, &me, "Lost connect{} block!");
    return;
  }

  MyFree(client_p->localClient->passwd);
  client_p->localClient->passwd = NULL;

  /* Its got identd, since its a server */
  SetGotId(client_p);

  /* If there is something in the serv_list, it might be this
   * connecting server..
   */
  if (!ServerInfo.hub && serv_list.head)
  {
    if (client_p != serv_list.head->data || serv_list.head->next)
    {
      ++ServerStats.is_ref;
      sendto_one(client_p, "ERROR :I'm a leaf not a hub");
      exit_client(client_p, &me, "I'm a leaf");
      return;
    }
  }

  aconf = map_to_conf(conf);

  if (IsUnknown(client_p))
  {
    /* jdc -- 1.  Use EmptyString(), not [0] index reference.
     *        2.  Check aconf->spasswd, not aconf->passwd.
     */
    if (!EmptyString(aconf->spasswd))
      sendto_one(client_p, "PASS %s TS %d %s",
                 aconf->spasswd, TS_CURRENT, me.id);
    else if (!EmptyString(aconf->certfp))
      sendto_one(client_p, "PASS certificate_auth TS %d %s", TS_CURRENT, me.id);

    /* Pass my info to the new server
     *
     * Pass on ZIP if supported
     * Pass on TB if supported.
     * - Dianora
     */

    send_capabilities(client_p, aconf, 0);

    sendto_one(client_p, "SERVER %s 1 :%s%s",
               me.name, ConfigServerHide.hidden ? "(H) " : "", me.info);
  }

  sendto_one(client_p, "SVINFO %d %d 0 :%lu", TS_CURRENT, TS_MIN,
             (unsigned long)CurrentTime);

  /* assumption here is if they passed the correct TS version, they also passed an SID */
  if (IsCapable(client_p, CAP_TS6))
    hash_add_id(client_p);

  /* XXX Does this ever happen? I don't think so -db */
  detach_conf(client_p, OPER_TYPE);

  /* *WARNING*
  **    In the following code in place of plain server's
  **    name we send what is returned by get_client_name
  **    which may add the "sockhost" after the name. It's
  **    *very* *important* that there is a SPACE between
  **    the name and sockhost (if present). The receiving
  **    server will start the information field from this
  **    first blank and thus puts the sockhost into info.
  **    ...a bit tricky, but you have been warned, besides
  **    code is more neat this way...  --msa
  */
  client_p->servptr = &me;

  if (IsClosing(client_p))
    return;

  SetServer(client_p);

  /* Update the capability combination usage counts. -A1kmm */
  set_chcap_usage_counts(client_p);

  /* Some day, all these lists will be consolidated *sigh* */
  dlinkAdd(client_p, &client_p->lnode, &me.serv->server_list);

  assert(dlinkFind(&unknown_list, client_p));

  dlink_move_node(&client_p->localClient->lclient_node,
                  &unknown_list, &serv_list);

  Count.myserver++;

  dlinkAdd(client_p, make_dlink_node(), &global_serv_list);
  hash_add_client(client_p);

  /* doesnt duplicate client_p->serv if allocated this struct already */
  make_server(client_p);

  /* fixing eob timings.. -gnp */
  client_p->localClient->firsttime = CurrentTime;

  if (find_matching_name_conf(SERVICE_TYPE, client_p->name, NULL, NULL, 0))
    AddFlag(client_p, FLAGS_SERVICE);

  /* Show the real host/IP to admins */
#ifdef HAVE_LIBCRYPTO

  if (client_p->localClient->fd.ssl)
  {
    compression = SSL_get_current_compression(client_p->localClient->fd.ssl);
    expansion   = SSL_get_current_expansion(client_p->localClient->fd.ssl);

    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "Link with %s established: [SSL: %s, Compression/Expansion method: %s/%s] (Capabilities: %s)",
                         inpath_ip, ssl_get_cipher(client_p->localClient->fd.ssl),
                         compression ? SSL_COMP_get_name(compression) : "NONE",
                         expansion ? SSL_COMP_get_name(expansion) : "NONE",
                         show_capabilities(client_p));
    ilog(LOG_TYPE_IRCD,
         "Link with %s established: [SSL: %s, Compression/Expansion method: %s/%s] (Capabilities: %s)",
         inpath_ip, ssl_get_cipher(client_p->localClient->fd.ssl),
         compression ? SSL_COMP_get_name(compression) : "NONE",
         expansion ? SSL_COMP_get_name(expansion) : "NONE",
         show_capabilities(client_p));
  }
  else
#endif
  {
    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "Link with %s established: (Capabilities: %s)",
                         inpath_ip, show_capabilities(client_p));
    ilog(LOG_TYPE_IRCD, "Link with %s established: (Capabilities: %s)",
         inpath_ip, show_capabilities(client_p));
  }

  fd_note(&client_p->localClient->fd, "Server: %s", client_p->name);

  /* Old sendto_serv_but_one() call removed because we now
  ** need to send different names to different servers
  ** (domain name matching) Send new server to other servers.
  */
  DLINK_FOREACH(ptr, serv_list.head)
  {
    target_p = ptr->data;

    if (target_p == client_p)
      continue;

    if (IsCapable(target_p, CAP_TS6) && HasID(client_p))
      sendto_one(target_p, ":%s SID %s 2 %s :%s%s",
                 me.id, client_p->name, client_p->id,
                 IsHidden(client_p) ? "(H) " : "",
                 client_p->info);
    else
      sendto_one(target_p, ":%s SERVER %s 2 :%s%s",
                 me.name, client_p->name,
                 IsHidden(client_p) ? "(H) " : "",
                 client_p->info);
  }

  /* Pass on my client information to the new server
  **
  ** First, pass only servers (idea is that if the link gets
  ** cancelled beacause the server was already there,
  ** there are no NICK's to be cancelled...). Of course,
  ** if cancellation occurs, all this info is sent anyway,
  ** and I guess the link dies when a read is attempted...? --msa
  **
  ** Note: Link cancellation to occur at this point means
  ** that at least two servers from my fragment are building
  ** up connection this other fragment at the same time, it's
  ** a race condition, not the normal way of operation...
  **
  ** ALSO NOTE: using the get_client_name for server names--
  **    see previous *WARNING*!!! (Also, original inpath
  **    is destroyed...)
  */

  DLINK_FOREACH_PREV(ptr, global_serv_list.tail)
  {
    target_p = ptr->data;

    /* target_p->from == target_p for target_p == client_p */
    if (IsMe(target_p) || target_p->from == client_p)
      continue;

    if (IsCapable(client_p, CAP_TS6))
    {
      if (HasID(target_p))
        sendto_one(client_p, ":%s SID %s %d %s :%s%s",
                   ID(target_p->servptr), target_p->name, target_p->hopcount + 1,
                   target_p->id, IsHidden(target_p) ? "(H) " : "",
                   target_p->info);
      else  /* introducing non-ts6 server */
        sendto_one(client_p, ":%s SERVER %s %d :%s%s",
                   ID(target_p->servptr), target_p->name, target_p->hopcount + 1,
                   IsHidden(target_p) ? "(H) " : "", target_p->info);
    }
    else
      sendto_one(client_p, ":%s SERVER %s %d :%s%s",
                 target_p->servptr->name, target_p->name, target_p->hopcount + 1,
                 IsHidden(target_p) ? "(H) " : "", target_p->info);
  }

  server_burst(client_p);
}

/* server_burst()
 *
 * inputs       - struct Client pointer server
 *              -
 * output       - none
 * side effects - send a server burst
 * bugs    - still too long
 */
static void
server_burst(struct Client *client_p)
{
  /* Send it in the shortened format with the TS, if
  ** it's a TS server; walk the list of channels, sending
  ** all the nicks that haven't been sent yet for each
  ** channel, then send the channel itself -- it's less
  ** obvious than sending all nicks first, but on the
  ** receiving side memory will be allocated more nicely
  ** saving a few seconds in the handling of a split
  ** -orabidoo
  */

  burst_all(client_p);

  /* EOB stuff is now in burst_all */
  /* Always send a PING after connect burst is done */
  sendto_one(client_p, "PING :%s", ID_or_name(&me, client_p));
}

/* burst_all()
 *
 * inputs  - pointer to server to send burst to
 * output  - NONE
 * side effects - complete burst of channels/nicks is sent to client_p
 */
void
burst_all(struct Client *client_p)
{
  dlink_node *ptr = NULL;

  DLINK_FOREACH(ptr, global_channel_list.head)
  {
    struct Channel *chptr = ptr->data;

    if (dlink_list_length(&chptr->members) != 0)
    {
      burst_members(client_p, chptr);
      send_channel_modes(client_p, chptr);

      if (IsCapable(client_p, CAP_TBURST))
        send_tb(client_p, chptr);
    }
  }

  /* also send out those that are not on any channel
   */
  DLINK_FOREACH(ptr, global_client_list.head)
  {
    struct Client *target_p = ptr->data;

    if (!HasFlag(target_p, FLAGS_BURSTED) && target_p->from != client_p)
      sendnick_TS(client_p, target_p);

    DelFlag(target_p, FLAGS_BURSTED);
  }

  /* We send the time we started the burst, and let the remote host determine an EOB time,
  ** as otherwise we end up sending a EOB of 0   Sending here means it gets sent last -- fl
  */
  /* Its simpler to just send EOB and use the time its been connected.. --fl_ */
  if (IsCapable(client_p, CAP_EOB))
    sendto_one(client_p, ":%s EOB", ID_or_name(&me, client_p));
}

/*
 * send_tb
 *
 * inputs       - pointer to Client
 *              - pointer to channel
 * output       - NONE
 * side effects - Called on a server burst when
 *                server is CAP_TBURST capable
 */
static void
send_tb(struct Client *client_p, struct Channel *chptr)
{
  /*
   * We may also send an empty topic here, but only if topic_time isn't 0,
   * i.e. if we had a topic that got unset.  This is required for syncing
   * topics properly.
   *
   * Imagine the following scenario: Our downlink introduces a channel
   * to us with a TS that is equal to ours, but the channel topic on
   * their side got unset while the servers were in splitmode, which means
   * their 'topic' is newer.  They simply wanted to unset it, so we have to
   * deal with it in a more sophisticated fashion instead of just resetting
   * it to their old topic they had before.  Read m_tburst.c:ms_tburst
   * for further information   -Michael
   */
  if (chptr->topic_time != 0)
    sendto_one(client_p, ":%s TBURST %lu %s %lu %s :%s",
               ID_or_name(&me, client_p),
               (unsigned long)chptr->channelts, chptr->chname,
               (unsigned long)chptr->topic_time,
               chptr->topic_info,
               chptr->topic);
}

/* burst_members()
 *
 * inputs  - pointer to server to send members to
 *     - dlink_list pointer to membership list to send
 * output  - NONE
 * side effects  -
 */
static void
burst_members(struct Client *client_p, struct Channel *chptr)
{
  struct Client *target_p;
  struct Membership *ms;
  dlink_node *ptr;

  DLINK_FOREACH(ptr, chptr->members.head)
  {
    ms       = ptr->data;
    target_p = ms->client_p;

    if (!HasFlag(target_p, FLAGS_BURSTED))
    {
      AddFlag(target_p, FLAGS_BURSTED);

      if (target_p->from != client_p)
        sendnick_TS(client_p, target_p);
    }
  }
}

/* set_autoconn()
 *
 * inputs       - struct Client pointer to oper requesting change
 * output       - none
 * side effects - set autoconnect mode
 */
void
set_autoconn(struct Client *source_p, const char *name, int newval)
{
  struct ConfItem *conf;
  struct AccessItem *aconf;

  if (name != NULL)
  {
    conf = find_exact_name_conf(SERVER_TYPE, NULL, name, NULL, NULL);

    if (conf != NULL)
    {
      aconf = (struct AccessItem *)map_to_conf(conf);

      if (newval)
        SetConfAllowAutoConn(aconf);
      else
        ClearConfAllowAutoConn(aconf);

      sendto_realops_flags(UMODE_ALL, L_ALL,
                           "%s has changed AUTOCONN for %s to %i",
                           source_p->name, name, newval);
      sendto_one(source_p,
                 ":%s NOTICE %s :AUTOCONN for %s is now set to %i",
                 me.name, source_p->name, name, newval);
    }
    else
    {
      sendto_one(source_p, ":%s NOTICE %s :Can't find %s",
                 me.name, source_p->name, name);
    }
  }
  else
  {
    sendto_one(source_p, ":%s NOTICE %s :Please specify a server name!",
               me.name, source_p->name);
  }
}

/* New server connection code
 * Based upon the stuff floating about in s_bsd.c
 *   -- adrian
 */

/* serv_connect() - initiate a server connection
 *
 * inputs  - pointer to conf
 *    - pointer to client doing the connect
 * output  -
 * side effects  -
 *
 * This code initiates a connection to a server. It first checks to make
 * sure the given server exists. If this is the case, it creates a socket,
 * creates a client, saves the socket information in the client, and
 * initiates a connection to the server through comm_connect_tcp(). The
 * completion of this goes through serv_completed_connection().
 *
 * We return 1 if the connection is attempted, since we don't know whether
 * it suceeded or not, and 0 if it fails in here somewhere.
 */
int
serv_connect(struct AccessItem *aconf, struct Client *by)
{
  struct ConfItem *conf;
  struct Client *client_p;
  char buf[HOSTIPLEN + 1];

  /* Make sure aconf is useful */
  assert(aconf != NULL);

  /* XXX should be passing struct ConfItem in the first place */
  conf = unmap_conf_item(aconf);

  /* log */
  ip_to_string(&aconf->addr, buf, sizeof(buf));

  ilog(LOG_TYPE_IRCD, "Connect to %s[%s] @%s", conf->name, aconf->host,
       buf);

  /* Still processing a DNS lookup? -> exit */
  if (aconf->dns_pending)
  {
    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "Error connecting to %s: DNS lookup for connect{} in progress.",
                         conf->name);
    return (0);
  }

  if (aconf->dns_failed)
  {
    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "Error connecting to %s: DNS lookup for connect{} failed.",
                         conf->name);
    return (0);
  }

  /* Make sure this server isn't already connected
   * Note: aconf should ALWAYS be a valid C: line
   */
  if ((client_p = hash_find_server(conf->name)) != NULL)
  {
    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "Server %s already present from %s",
                         conf->name, get_client_name(client_p, SHOW_IP));



    if (by && IsClient(by) && !MyClient(by))
      sendto_one(by, ":%s NOTICE %s :Server %s already present from %s",
                 me.name, by->name, conf->name,
                 get_client_name(client_p, MASK_IP));

    return (0);
  }

  /* Create a local client */
  client_p = make_client(NULL);

  /* Copy in the server, hostname, fd */
  strlcpy(client_p->name, conf->name, sizeof(client_p->name));
  strlcpy(client_p->host, aconf->host, sizeof(client_p->host));

  /* We already converted the ip once, so lets use it - stu */
  strlcpy(client_p->sockhost, buf, sizeof(client_p->sockhost));

  /* create a socket for the server connection */
  if (comm_open(&client_p->localClient->fd, aconf->addr.ss_family,
                SOCK_STREAM, NULL) < 0)
  {
    /* Eek, failure to create the socket */
    report_error(L_ALL, "opening stream socket to %s: %s", conf->name, 
                 uv_last_error(server_state.event_loop));
    SetDead(client_p);
    exit_client(client_p, &me, "Connection failed");
    return (0);
  }

  /* servernames are always guaranteed under HOSTLEN chars */
  fd_note(&client_p->localClient->fd, "Server: %s", conf->name);

  /* Attach config entries to client here rather than in
   * serv_connect_callback(). This to avoid null pointer references.
   */
  if (!attach_connect_block(client_p, conf->name, aconf->host))
  {
    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "Host %s is not enabled for connecting:no C/N-line",
                         conf->name);

    if (by && IsClient(by) && !MyClient(by))
      sendto_one(by, ":%s NOTICE %s :Connect to host %s failed.",
                 me.name, by->name, client_p->name);

    SetDead(client_p);
    exit_client(client_p, client_p, "Connection failed");
    return (0);
  }

  /* at this point we have a connection in progress and C/N lines
   * attached to the client, the socket info should be saved in the
   * client and it should either be resolved or have a valid address.
   *
   * The socket has been connected or connect is in progress.
   */
  make_server(client_p);

  if (by && IsClient(by))
    strlcpy(client_p->serv->by, by->name, sizeof(client_p->serv->by));
  else
    strlcpy(client_p->serv->by, "AutoConn.", sizeof(client_p->serv->by));

  SetConnecting(client_p);
  dlinkAdd(client_p, &client_p->node, &global_client_list);
  /* from def_fam */

  /* Now, initiate the connection */
  /* XXX assume that a non 0 type means a specific bind address
   * for this connect.
   */
  switch (aconf->aftype)
  {
    case AF_INET:
      if (((struct sockaddr_in *)&aconf->bind)->sin_addr.s_addr != 0)
      {
        comm_connect_tcp(&client_p->localClient->fd, aconf->host, aconf->port,
                         (struct sockaddr *)&aconf->bind, 
                         sizeof(struct sockaddr_in), serv_connect_callback, 
                         client_p, aconf->aftype, CONNECTTIMEOUT);
      }
      else if (ServerInfo.specific_ipv4_vhost)
      {
        comm_connect_tcp(&client_p->localClient->fd, aconf->host, aconf->port,
                         (struct sockaddr *)&ServerInfo.ip, 
                         sizeof(struct sockaddr_in), serv_connect_callback, 
                         client_p, aconf->aftype, CONNECTTIMEOUT);
      }
      else
        comm_connect_tcp(&client_p->localClient->fd, aconf->host, aconf->port,
                         NULL, 0, serv_connect_callback, client_p, 
                         aconf->aftype, CONNECTTIMEOUT);
      break;
    case AF_INET6:
    {
      struct sockaddr_in6 *v6 = (struct sockaddr_in6 *)&aconf->bind;
      if(IN6_IS_ADDR_UNSPECIFIED(&v6->sin6_addr))
      {
        comm_connect_tcp(&client_p->localClient->fd,
                         aconf->host, aconf->port,
                         (struct sockaddr *)&aconf->bind, 
                         sizeof(struct sockaddr_in6), serv_connect_callback, 
                         client_p, aconf->aftype, CONNECTTIMEOUT);
      }
      else if (ServerInfo.specific_ipv6_vhost)
      {
        comm_connect_tcp(&client_p->localClient->fd,
                         aconf->host, aconf->port,
                         (struct sockaddr *)&ServerInfo.ip, 
                         sizeof(struct sockaddr_in6), serv_connect_callback, 
                         client_p, aconf->aftype, CONNECTTIMEOUT);
      }
      else
        comm_connect_tcp(&client_p->localClient->fd,
                         aconf->host, aconf->port,
                         NULL, 0, serv_connect_callback, client_p,
                         aconf->aftype, CONNECTTIMEOUT);
    }
    default:
      assert(0);
      return 0;
  }

  return (1);
}

#ifdef HAVE_LIBCRYPTO
void
finish_ssl_server_handshake(struct Client *client_p)
{
  struct ConfItem *conf = NULL;
  struct AccessItem *aconf = NULL;

  conf = find_conf_name(&client_p->localClient->confs,
                        client_p->name, SERVER_TYPE);

  if (conf == NULL)
  {
    sendto_realops_flags(UMODE_ALL, L_ADMIN,
                         "Lost connect{} block for %s", get_client_name(client_p, HIDE_IP));
    sendto_realops_flags(UMODE_ALL, L_OPER,
                         "Lost connect{} block for %s", get_client_name(client_p, MASK_IP));

    exit_client(client_p, &me, "Lost connect{} block");
    return;
  }

  aconf = map_to_conf(conf);

  /* jdc -- Check and send spasswd, not passwd. */
  if (!EmptyString(aconf->spasswd))
    sendto_one(client_p, "PASS %s TS %d %s",
               aconf->spasswd, TS_CURRENT, me.id);
  else if (!EmptyString(aconf->certfp))
    sendto_one(client_p, "PASS certificate_auth TS %d %s", TS_CURRENT, me.id);

  send_capabilities(client_p, aconf, 0);

  sendto_one(client_p, "SERVER %s 1 :%s%s",
             me.name, ConfigServerHide.hidden ? "(H) " : "",
             me.info);

  /* If we've been marked dead because a send failed, just exit
   * here now and save everyone the trouble of us ever existing.
   */
  if (IsDead(client_p))
  {
    sendto_realops_flags(UMODE_ALL, L_ADMIN,
                         "%s[%s] went dead during handshake",
                         client_p->name,
                         client_p->host);
    sendto_realops_flags(UMODE_ALL, L_OPER,
                         "%s went dead during handshake", client_p->name);
    return;
  }

  if(uv_read_start((uv_stream_t*)client_p->localClient->fd.handle, 
                   allocate_uv_buffer, read_packet) < 0)
    dead_link_on_read(client_p, uv_last_error(server_state.event_loop).code);
}
#endif

static void
ssl_connect_init(struct Client *client_p, struct AccessItem *aconf, fde_t *fd)
{
  if ((client_p->localClient->fd.ssl = SSL_new(ServerInfo.client_ctx)) == NULL)
  {
    ilog(LOG_TYPE_IRCD, "SSL_new() ERROR! -- %s",
         ERR_error_string(ERR_get_error(), NULL));
    SetDead(client_p);
    exit_client(client_p, client_p, "SSL_new failed");
    return;
  }

  client_p->localClient->fd.read_bio = BIO_new(BIO_s_mem());
  client_p->localClient->fd.write_bio = BIO_new(BIO_s_mem());

  SSL_set_bio(client_p->localClient->fd.ssl,
              client_p->localClient->fd.read_bio,
              client_p->localClient->fd.write_bio);


  if (!EmptyString(aconf->cipher_list))
    SSL_set_cipher_list(client_p->localClient->fd.ssl, aconf->cipher_list);

  SSL_set_connect_state(client_p->localClient->fd.ssl);

  ssl_handshake(client_p, true);
}

/* serv_connect_callback() - complete a server connection.
 *
 * This routine is called after the server connection attempt has
 * completed. If unsucessful, an error is sent to ops and the client
 * is closed. If sucessful, it goes through the initialisation/check
 * procedures, the capabilities are sent, and the socket is then
 * marked for reading.
 */
static void
serv_connect_callback(fde_t *fd, int status, void *data)
{
  struct Client *client_p = data;
  struct ConfItem *conf = NULL;
  struct AccessItem *aconf = NULL;

  /* First, make sure its a real client! */
  assert(client_p != NULL);
  assert(&client_p->localClient->fd == fd);

  fd->handle->data = client_p;

  /* Next, for backward purposes, record the ip of the server */
  memcpy(&client_p->ip, &fd->connect.hostaddr, sizeof(&client_p->ip));

  /* Check the status */
  if (status != COMM_OK)
  {
    /* We have an error, so report it and quit
     * Admins get to see any IP, mere opers don't *sigh*
     */
    if (ConfigServerHide.hide_server_ips)
      sendto_realops_flags(UMODE_ALL, L_ALL,
                           "Error connecting to %s: %s",
                           client_p->name, comm_errstr(status));
    else
      sendto_realops_flags(UMODE_ALL, L_ADMIN,
                           "Error connecting to %s[%s]: %s", client_p->name,
                           client_p->host, comm_errstr(status));





    /* If a fd goes bad, call dead_link() the socket is no
     * longer valid for reading or writing.
     */
    dead_link_on_write(client_p, 0);
    return;
  }

  /* COMM_OK, so continue the connection procedure */
  /* Get the C/N lines */
  conf = find_conf_name(&client_p->localClient->confs,
                        client_p->name, SERVER_TYPE);

  if (conf == NULL)
  {
    sendto_realops_flags(UMODE_ALL, L_ADMIN,
                         "Lost connect{} block for %s", get_client_name(client_p, HIDE_IP));

    exit_client(client_p, &me, "Lost connect{} block");
    return;
  }

  aconf = map_to_conf(conf);
  /* Next, send the initial handshake */
  SetHandshake(client_p);

#ifdef HAVE_LIBCRYPTO

  if (IsConfSSL(aconf))
  {
    ssl_connect_init(client_p, aconf, fd);
    return;
  }

#endif

  /* jdc -- Check and send spasswd, not passwd. */
  if (!EmptyString(aconf->spasswd))
    sendto_one(client_p, "PASS %s TS %d %s",
               aconf->spasswd, TS_CURRENT, me.id);
  else if (!EmptyString(aconf->certfp))
    sendto_one(client_p, "PASS certificate_auth TS %d %s", TS_CURRENT, me.id);

  send_capabilities(client_p, aconf, 0);

  sendto_one(client_p, "SERVER %s 1 :%s%s",
             me.name, ConfigServerHide.hidden ? "(H) " : "",
             me.info);

  /* If we've been marked dead because a send failed, just exit
   * here now and save everyone the trouble of us ever existing.
   */
  if (IsDead(client_p))
  {
    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "%s[%s] went dead during handshake",
                         client_p->name,
                         client_p->host);


    return;
  }

  /* don't move to serv_list yet -- we haven't sent a burst! */
  /* If we get here, we're ok, so lets start reading some data */
  if(uv_read_start((uv_stream_t*)client_p->localClient->fd.handle, 
                   allocate_uv_buffer, read_packet) < 0)
    dead_link_on_read(client_p, uv_last_error(server_state.event_loop).code);
}

struct Client *
find_servconn_in_progress(const char *name)
{
  dlink_node *ptr;
  struct Client *cptr;

  DLINK_FOREACH(ptr, unknown_list.head)
  {
    cptr = ptr->data;

    if (cptr && cptr->name[0])
      if (match(name, cptr->name))
        return cptr;
  }

  return NULL;
}
