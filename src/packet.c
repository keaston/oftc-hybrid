/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  packet.c: Packet handlers.
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
#include "s_bsd.h"
#include "conf.h"
#include "s_serv.h"
#include "client.h"
#include "ircd.h"
#include "parse.h"
#include "fdlist.h"
#include "packet.h"
#include "irc_string.h"
#include "memory.h"
#include "hook.h"
#include "send.h"
#include "s_misc.h"

struct Callback *iorecv_cb = NULL;

static void client_dopacket(struct Client *, char *, size_t);

/* extract_one_line()
 *
 * inputs       - pointer to a dbuf queue
 *              - pointer to buffer to copy data to
 * output       - length of <buffer>
 * side effects - one line is copied and removed from the dbuf
 */
static int
extract_one_line(struct dbuf_queue *qptr, char *buffer)
{
  struct dbuf_block *block;
  int line_bytes = 0, empty_bytes = 0, phase = 0;
  unsigned int idx;

  char c;
  dlink_node *ptr;

  /*
   * Phase 0: "empty" characters before the line
   * Phase 1: copying the line
   * Phase 2: "empty" characters after the line
   *          (delete them as well and free some space in the dbuf)
   *
   * Empty characters are CR, LF and space (but, of course, not
   * in the middle of a line). We try to remove as much of them as we can,
   * since they simply eat server memory.
   *
   * --adx
   */
  DLINK_FOREACH(ptr, qptr->blocks.head)
  {
    block = ptr->data;

    for (idx = 0; idx < block->size; idx++)
    {
      c = block->data[idx];

      if (IsEol(c) || (c == ' ' && phase != 1))
      {
        empty_bytes++;

        if (phase == 1)
          phase = 2;
      }
      else switch (phase)
        {
          case 0:
            phase = 1;

          case 1:
            if (line_bytes++ < IRCD_BUFSIZE - 2)
              *buffer++ = c;

            break;

          case 2:
            *buffer = '\0';
            dbuf_delete(qptr, line_bytes + empty_bytes);
            return IRCD_MIN(line_bytes, IRCD_BUFSIZE - 2);
        }
    }
  }

  /*
   * Now, if we haven't reached phase 2, ignore all line bytes
   * that we have read, since this is a partial line case.
   */
  if (phase != 2)
    line_bytes = 0;
  else
    *buffer = '\0';

  /* Remove what is now unnecessary */
  dbuf_delete(qptr, line_bytes + empty_bytes);
  return IRCD_MIN(line_bytes, IRCD_BUFSIZE - 2);
}

/*
 * parse_client_queued - parse client queued messages
 */
static void
parse_client_queued(struct Client *client_p)
{
  int dolen = 0;
  int checkflood = 1;
  struct LocalUser *lclient_p = client_p->localClient;
  char buffer[IRCD_BUFSIZE + 1] = { '\0' };
  bool keep_going = true;
  int i = 0;

  while(keep_going)
  {
    if (IsDefunct(client_p))
      return;

    /* rate unknown clients at MAX_FLOOD per loop */
    if (IsUnknown(client_p))
    {
      if (i >= MAX_FLOOD)
        break;
    }
    else if (IsClient(client_p))
    {
      if (ConfigFileEntry.no_oper_flood && (HasUMode(client_p, UMODE_OPER)
                                            || IsCanFlood(client_p)))
      {
        if (ConfigFileEntry.true_no_oper_flood)
          checkflood = -1;
        else
          checkflood = 0;
      }
      /* This flood protection works as follows:
       *
       * A client is given allow_read lines to send to the server.  Every
       * time a line is parsed, sent_parsed is increased.  sent_parsed
       * is decreased by 1 every time flood_recalc is called.
       *
       * Thus a client can 'burst' allow_read lines to the server, any
       * excess lines will be parsed one per flood_recalc() call.
       *
       * Therefore a client will be penalised more if they keep flooding,
       * as sent_parsed will always hover around the allow_read limit
       * and no 'bursts' will be permitted.
       */
      if (checkflood > 0)
      {
        if (lclient_p->sent_parsed >= lclient_p->allow_read)
          break;
      }

      /* allow opers 4 times the amount of messages as users. why 4?
       * why not. :) --fl_
       */
      else if (lclient_p->sent_parsed >= (4 * lclient_p->allow_read) &&
               checkflood != -1)
        break;

    }

    dolen = extract_one_line(&lclient_p->buf_recvq, buffer);

    if (dolen == 0)
      break;

    client_dopacket(client_p, buffer, dolen);
    if(IsClient(client_p))
      lclient_p->sent_parsed++;
    i++;
  }
}

/* flood_endgrace()
 *
 * marks the end of the clients grace period
 */
void
flood_endgrace(struct Client *client_p)
{
  SetFloodDone(client_p);

  /* Drop their flood limit back down */
  client_p->localClient->allow_read = MAX_FLOOD;

  /* sent_parsed could be way over MAX_FLOOD but under MAX_FLOOD_BURST,
   * so reset it.
   */
  client_p->localClient->sent_parsed = 0;
}

/*
 * flood_recalc
 *
 * recalculate the number of allowed flood lines. this should be called
 * once a second on any given client. We then attempt to flush some data.
 */
void
flood_recalc(fde_t *fd, void *data)
{
  struct Client *client_p = data;
  struct LocalUser *lclient_p = client_p->localClient;

  /* allow a bursting client their allocation per second, allow
   * a client whos flooding an extra 2 per second
   */
  if (IsFloodDone(client_p))
    lclient_p->sent_parsed -= 2;
  else
    lclient_p->sent_parsed = 0;

  if (lclient_p->sent_parsed < 0)
    lclient_p->sent_parsed = 0;

  parse_client_queued(client_p);

  /* And now, try flushing .. */
  if (!IsDead(client_p))
  {
    /* and finally, reset the flood check */
    comm_setflush(fd, 1000, flood_recalc, client_p);
  }
}

/*
 * iorecv_default - append a packet to the recvq dbuf
 */
void *
iorecv_default(va_list args)
{
  struct Client *client_p = va_arg(args, struct Client *);
  int length = va_arg(args, int);
  char *buf = va_arg(args, char *);

  dbuf_put(&client_p->localClient->buf_recvq, buf, length);
  return NULL;
}

/*
 * read_packet - Read a 'packet' of data from a connection and process it.
 */
void
read_packet(uv_stream_t *stream, ssize_t nread, uv_buf_t buf)
{
  struct Client *client_p = stream->data;
  int length = 0;
  bool is_ssl;
  char *buffer;

  if (IsDefunct(client_p))
  {
    BlockHeapFree(buffer_heap, buf.base);
    return;
  }

  if(nread <= 0)
  {
    dead_link_on_read(client_p, uv_last_error(server_state.event_loop).code);
    BlockHeapFree(buffer_heap, buf.base);
    return;
  }

#ifdef HAVE_LIBCRYPTO
  if (client_p->localClient->fd.ssl != NULL)
  {
    int offset = 0;

    char ssl_buffer[READBUF_SIZE] = { 0 };

    buffer = ssl_buffer;

    BIO_write(client_p->localClient->fd.read_bio, buf.base, nread);

    while(BIO_pending(client_p->localClient->fd.read_bio) != 0)
    {
      int len = SSL_read(client_p->localClient->fd.ssl, ssl_buffer + offset, 
                        READBUF_SIZE - offset);
      if(len == -1)
      {
        int err = SSL_get_error(client_p->localClient->fd.ssl, len);

        switch(err)
        {
          case SSL_ERROR_WANT_READ:
          case SSL_ERROR_WANT_WRITE:
            ssl_flush_write(client_p);
            break;

          default:
            break;
        }
      }
      else
      {
        offset += len;
        length += len;
      }
    }
  }
  else
#endif
  {
    length = nread;
    buffer = buf.base;
    is_ssl = false;
  }

  if(length != 0)
    execute_callback(iorecv_cb, client_p, length, buffer);

  BlockHeapFree(buffer_heap, buf.base);

  // This could happen if we have some ssl data but not enough to decrypt
  if(length == 0)
    return;

  if (IsServer(client_p) && IsPingSent(client_p) && IsPingWarning(client_p))
  {
    char timestamp[200];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S Z",
             gmtime(&CurrentTime));
    sendto_realops_flags(UMODE_ALL, L_ALL,
                         "Finally received packets from %s again after %d seconds (at %s)",
                         get_client_name(client_p, SHOW_IP),
                         CurrentTime - client_p->localClient->lasttime, timestamp);
  }

  if (client_p->localClient->lasttime < CurrentTime)
    client_p->localClient->lasttime = CurrentTime;

  if (client_p->localClient->lasttime > client_p->localClient->since)
    client_p->localClient->since = CurrentTime;

  ClearPingSent(client_p);

  /* Attempt to parse what we have */
  parse_client_queued(client_p);

  if (IsDefunct(client_p))
    return;

  /* Check to make sure we're not flooding */
  if (!(IsServer(client_p) || IsHandshake(client_p) || IsConnecting(client_p))
      && (dbuf_length(&client_p->localClient->buf_recvq) >
          get_recvq(client_p)))
  {
    if (!(ConfigFileEntry.no_oper_flood && HasUMode(client_p, UMODE_OPER)))
    {
      exit_client(client_p, client_p, "Excess Flood");
      return;
    }
  }
}

/*
 * client_dopacket - copy packet to client buf and parse it
 *      client_p - pointer to client structure for which the buffer data
 *             applies.
 *      buffer - pointr to the buffer containing the newly read data
 *      length - number of valid bytes of data in the buffer
 *
 * Note:
 *      It is implicitly assumed that dopacket is called only
 *      with client_p of "local" variation, which contains all the
 *      necessary fields (buffer etc..)
 */
static void
client_dopacket(struct Client *client_p, char *buffer, size_t length)
{
  /*
   * Update messages received
   */
  ++me.localClient->recv.messages;
  ++client_p->localClient->recv.messages;

  /*
   * Update bytes received
   */
  client_p->localClient->recv.bytes += length;
  me.localClient->recv.bytes += length;

  parse(client_p, buffer, buffer + length);
}
