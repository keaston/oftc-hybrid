MAINTAINERCLEANFILES = Makefile.in

sbin_PROGRAMS = ircd

AM_YFLAGS = -d

AM_CPPFLAGS = -I$(top_srcdir)/include $(OPENSSL_INCLUDES) -I$(top_srcdir)/libuv/include
ircd_LDFLAGS = -export-dynamic $(OPENSSL_LDFLAGS) `sh ../libuv-ldflags.sh $(MAKE)`
ircd_LDADD = $(OPENSSL_LIBS) ../libuv/libuv.a
ircd_DEPENDENCIES = 

ircd_SOURCES = balloc.c          \
               channel.c         \
               channel_mode.c    \
               client.c          \
               conf.c            \
               conf_parser.y     \
               conf_lexer.l      \
               csvlib.c          \
               dbuf.c            \
               event.c           \
               fdlist.c          \
               getopt.c          \
               hash.c            \
               hook.c            \
               hostmask.c        \
               irc_res.c         \
               irc_reslib.c      \
               irc_string.c      \
               ircd.c            \
               ircd_signal.c     \
               list.c            \
               listener.c        \
               log.c             \
               match.c           \
               memory.c          \
               modules.c         \
               motd.c            \
               numeric.c         \
               packet.c          \
               parse.c           \
               restart.c         \
               resv.c            \
               rsa.c             \
               s_auth.c          \
               s_bsd.c           \
               s_misc.c          \
               s_serv.c          \
               s_user.c          \
               send.c            \
               sprintf_irc.c     \
               version.c         \
               watch.c           \
               whowas.c          \
               messages.tab
