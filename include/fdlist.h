/*
 *  ircd-hybrid: an advanced Internet Relay Chat Daemon(ircd).
 *  fdlist.h: The file descriptor list header.
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

#ifndef INCLUDED_fdlist_h
#define INCLUDED_fdlist_h
#define FILEIO_V2

#include "ircd_defs.h"
#define FD_DESC_SZ 128  /* hostlen + comment */

enum
{
  COMM_OK,
  COMM_ERR_BIND,
  COMM_ERR_DNS,
  COMM_ERR_TIMEOUT,
  COMM_ERR_CONNECT,
  COMM_ERROR,
  COMM_ERR_MAX
};

struct _fde;
struct Client;

/* Callback for completed IO events */
typedef void PF(struct _fde *, void *);

/* Callback for completed connections */
/* int fd, int status, void * */
typedef void CNCB(struct _fde *, int, void *);

typedef struct _fde
{
  uv_stream_t *handle;           /* So we can use the fde_t as a callback ptr */
  int         comm_index;       /* where in the poll list we live */
  int         evcache;          /* current fd events as set up by the underlying I/O */
  char        desc[FD_DESC_SZ];
  PF          *read_handler;
  void        *read_data;
  PF          *write_handler;
  void        *write_data;
  PF          *timeout_handler;
  void        *timeout_data;
  time_t      timeout;
  PF          *flush_handler;
  void        *flush_data;
  time_t      flush_timeout;

  struct
  {
    unsigned int open: 1;
    unsigned int is_socket: 1;
#ifdef HAVE_LIBCRYPTO
    unsigned int pending_read: 1;
#endif
  } flags;

  struct
  {
    uv_connect_t *handle;
    /* We don't need the host here ? */
    struct sockaddr_storage S;
    struct sockaddr_storage hostaddr;
    CNCB                    *callback;
    void                    *data;
    /* We'd also add the retry count here when we get to that -- adrian */
  } connect;
#ifdef HAVE_LIBCRYPTO
  SSL         *ssl;
  BIO         *read_bio;
  BIO         *write_bio;
#endif
  dlink_node  fnode;
} fde_t;

#define FD_HASH_SIZE CLIENT_HEAP_SIZE

IRCD_EXTERN int number_fd;
IRCD_EXTERN int hard_fdlimit;

IRCD_EXTERN dlink_list fd_list;

IRCD_EXTERN void fdlist_init();
IRCD_EXTERN fde_t *lookup_fd(int);
IRCD_EXTERN void fd_open(fde_t *, uv_stream_t *, const char *);
IRCD_EXTERN void fd_close(fde_t *);
IRCD_EXTERN void fd_dump(struct Client *);
IRCD_EXTERN void fd_note(fde_t *, const char *, ...);
IRCD_EXTERN void close_standard_fds();
IRCD_EXTERN void close_fds(fde_t *);
#endif /* INCLUDED_fdlist_h */
