MAINTAINERCLEANFILES = Makefile.in
AM_LDFLAGS = -no-undefined -module -avoid-version $(OPENSSL_LDFLAGS)
SUBDIRS = core

AM_CPPFLAGS = -I$(top_srcdir)/include $(OPENSSL_INCLUDES) -I$(top_srcdir)/libuv/include
modulesdir = $(pkglibdir)/modules/autoload

modules_LTLIBRARIES = m_accept.la    \
                      m_admin.la     \
                      m_away.la      \
                      m_capab.la     \
                      m_cap.la       \
                      m_certfp.la    \
                      m_challenge.la \
                      m_close.la     \
                      m_connect.la   \
                      m_dline.la     \
                      m_encap.la     \
                      m_eob.la       \
                      m_etrace.la    \
                      m_globops.la   \
                      m_gnotice.la   \
                      m_hash.la      \
                      m_help.la      \
                      m_info.la      \
                      m_invite.la    \
                      m_ison.la      \
                      m_kline.la     \
                      m_knock.la     \
                      m_links.la     \
                      m_list.la      \
                      m_locops.la    \
                      m_log.la       \
                      m_lusers.la    \
                      m_map.la       \
                      m_module.la    \
                      m_motd.la      \
                      m_names.la     \
                      m_oper.la      \
                      m_operwall.la  \
                      m_pass.la      \
                      m_ping.la      \
                      m_pong.la      \
                      m_post.la      \
                      m_realhost.la  \
                      m_rehash.la    \
                      m_restart.la   \
                      m_resv.la      \
                      m_set.la       \
                      m_shedding.la  \
                      m_services.la  \
                      m_stats.la     \
                      m_svinfo.la    \
                      m_svscloak.la  \
                      m_svsmode.la   \
                      m_svsnick.la   \
                      m_tburst.la    \
                      m_testline.la  \
                      m_testmask.la  \
                      m_time.la      \
                      m_topic.la     \
                      m_trace.la     \
                      m_user.la      \
                      m_userhost.la  \
                      m_users.la     \
                      m_version.la   \
                      m_wallops.la   \
                      m_watch.la     \
                      m_webirc.la    \
                      m_who.la       \
                      m_whois.la     \
                      m_whowas.la    \
                      m_xline.la 
                      #soft_reboot.la



m_accept_la_SOURCES = m_accept.c
m_admin_la_SOURCES = m_admin.c
m_away_la_SOURCES = m_away.c
m_capab_la_SOURCES = m_capab.c
m_cap_la_SOURCES = m_cap.c
m_certfp_la_SOURCES = m_certfp.c
m_challenge_la_SOURCES = m_challenge.c
m_close_la_SOURCES = m_close.c
m_connect_la_SOURCES = m_connect.c
m_dline_la_SOURCES = m_dline.c
m_encap_la_SOURCES = m_encap.c
m_eob_la_SOURCES = m_eob.c
m_etrace_la_SOURCES = m_etrace.c
m_globops_la_SOURCES = m_globops.c
m_gnotice_la_SOURCES = m_gnotice.c
m_hash_la_SOURCES = m_hash.c
m_help_la_SOURCES = m_help.c
m_info_la_SOURCES = m_info.c
m_invite_la_SOURCES = m_invite.c
m_ison_la_SOURCES = m_ison.c
m_kline_la_SOURCES = m_kline.c
m_knock_la_SOURCES = m_knock.c
m_links_la_SOURCES = m_links.c
m_list_la_SOURCES = m_list.c
m_locops_la_SOURCES = m_locops.c
m_log_la_SOURCES = m_log.c
m_lusers_la_SOURCES = m_lusers.c
m_map_la_SOURCES = m_map.c
m_module_la_SOURCES = m_module.c
m_motd_la_SOURCES = m_motd.c
m_names_la_SOURCES = m_names.c
m_oper_la_SOURCES = m_oper.c
m_operwall_la_SOURCES = m_operwall.c
m_pass_la_SOURCES = m_pass.c
m_ping_la_SOURCES = m_ping.c
m_pong_la_SOURCES = m_pong.c
m_post_la_SOURCES = m_post.c
m_realhost_la_SOURCES = m_realhost.c
m_rehash_la_SOURCES = m_rehash.c
m_restart_la_SOURCES = m_restart.c
m_resv_la_SOURCES = m_resv.c
m_set_la_SOURCES = m_set.c
m_services_la_SOURCES = m_services.c
m_shedding_la_SOURCES = m_shedding.c
m_stats_la_SOURCES = m_stats.c
m_svinfo_la_SOURCES = m_svinfo.c
m_svscloak_la_SOURCES = m_svscloak.c
m_svsmode_la_SOURCES = m_svsmode.c
m_svsnick_la_SOURCES = m_svsnick.c
m_tburst_la_SOURCES = m_tburst.c
m_testline_la_SOURCES = m_testline.c
m_testmask_la_SOURCES = m_testmask.c
m_time_la_SOURCES = m_time.c
m_topic_la_SOURCES = m_topic.c
m_trace_la_SOURCES = m_trace.c
m_user_la_SOURCES = m_user.c
m_userhost_la_SOURCES = m_userhost.c
m_users_la_SOURCES = m_users.c
m_version_la_SOURCES = m_version.c
m_wallops_la_SOURCES = m_wallops.c
m_watch_la_SOURCES = m_watch.c
m_webirc_la_SOURCES = m_webirc.c
m_who_la_SOURCES = m_who.c
m_whois_la_SOURCES = m_whois.c
m_whowas_la_SOURCES = m_whowas.c
m_xline_la_SOURCES = m_xline.c
#soft_reboot_la_SOURCES = soft_reboot.c

modules: $(modules_LTLIBRARIES)
