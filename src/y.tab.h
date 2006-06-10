/* A Bison parser, made by GNU Bison 2.1.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     ACCEPT_PASSWORD = 258,
     ACTION = 259,
     ADMIN = 260,
     AFTYPE = 261,
     T_ALLOW = 262,
     ANTI_NICK_FLOOD = 263,
     ANTI_SPAM_EXIT_MESSAGE_TIME = 264,
     AUTOCONN = 265,
     T_BLOCK = 266,
     BURST_AWAY = 267,
     BURST_TOPICWHO = 268,
     BYTES = 269,
     KBYTES = 270,
     MBYTES = 271,
     GBYTES = 272,
     TBYTES = 273,
     CALLER_ID_WAIT = 274,
     CAN_FLOOD = 275,
     CAN_IDLE = 276,
     CHANNEL = 277,
     CIDR_BITLEN_IPV4 = 278,
     CIDR_BITLEN_IPV6 = 279,
     CIPHER_PREFERENCE = 280,
     CLASS = 281,
     COMPRESSED = 282,
     COMPRESSION_LEVEL = 283,
     CONNECT = 284,
     CONNECTFREQ = 285,
     CRYPTLINK = 286,
     DEFAULT_CIPHER_PREFERENCE = 287,
     DEFAULT_FLOODCOUNT = 288,
     DEFAULT_SPLIT_SERVER_COUNT = 289,
     DEFAULT_SPLIT_USER_COUNT = 290,
     DENY = 291,
     DESCRIPTION = 292,
     DIE = 293,
     DISABLE_AUTH = 294,
     DISABLE_FAKE_CHANNELS = 295,
     DISABLE_HIDDEN = 296,
     DISABLE_LOCAL_CHANNELS = 297,
     DISABLE_REMOTE_COMMANDS = 298,
     DOT_IN_IP6_ADDR = 299,
     DOTS_IN_IDENT = 300,
     DURATION = 301,
     EGDPOOL_PATH = 302,
     EMAIL = 303,
     ENABLE = 304,
     ENCRYPTED = 305,
     EXCEED_LIMIT = 306,
     EXEMPT = 307,
     FAILED_OPER_NOTICE = 308,
     FAKENAME = 309,
     IRCD_FLAGS = 310,
     FLATTEN_LINKS = 311,
     FFAILED_OPERLOG = 312,
     FKILLLOG = 313,
     FKLINELOG = 314,
     FGLINELOG = 315,
     FIOERRLOG = 316,
     FOPERLOG = 317,
     FOPERSPYLOG = 318,
     FUSERLOG = 319,
     GECOS = 320,
     GENERAL = 321,
     GLINE = 322,
     GLINES = 323,
     GLINE_EXEMPT = 324,
     GLINE_LOG = 325,
     GLINE_TIME = 326,
     GLINE_MIN_CIDR = 327,
     GLINE_MIN_CIDR6 = 328,
     GLOBAL_KILL = 329,
     IRCD_AUTH = 330,
     NEED_IDENT = 331,
     HAVENT_READ_CONF = 332,
     HIDDEN = 333,
     HIDDEN_ADMIN = 334,
     HIDDEN_NAME = 335,
     HIDDEN_OPER = 336,
     HIDE_SERVER_IPS = 337,
     HIDE_SERVERS = 338,
     HIDE_SPOOF_IPS = 339,
     HOST = 340,
     HUB = 341,
     HUB_MASK = 342,
     IDLETIME = 343,
     IGNORE_BOGUS_TS = 344,
     INVISIBLE_ON_CONNECT = 345,
     IP = 346,
     KILL = 347,
     KILL_CHASE_TIME_LIMIT = 348,
     KLINE = 349,
     KLINE_EXEMPT = 350,
     KLINE_REASON = 351,
     KLINE_WITH_REASON = 352,
     KNOCK_DELAY = 353,
     KNOCK_DELAY_CHANNEL = 354,
     LAZYLINK = 355,
     LEAF_MASK = 356,
     LINKS_DELAY = 357,
     LISTEN = 358,
     T_LOG = 359,
     LOGGING = 360,
     LOG_LEVEL = 361,
     MAX_ACCEPT = 362,
     MAX_BANS = 363,
     MAX_CHANS_PER_USER = 364,
     MAX_GLOBAL = 365,
     MAX_IDENT = 366,
     MAX_LOCAL = 367,
     MAX_NICK_CHANGES = 368,
     MAX_NICK_TIME = 369,
     MAX_NUMBER = 370,
     MAX_TARGETS = 371,
     MESSAGE_LOCALE = 372,
     MIN_NONWILDCARD = 373,
     MIN_NONWILDCARD_SIMPLE = 374,
     MODULE = 375,
     MODULES = 376,
     NAME = 377,
     NEED_PASSWORD = 378,
     NETWORK_DESC = 379,
     NETWORK_NAME = 380,
     NICK = 381,
     NICK_CHANGES = 382,
     NO_CREATE_ON_SPLIT = 383,
     NO_JOIN_ON_SPLIT = 384,
     NO_OPER_FLOOD = 385,
     NO_TILDE = 386,
     NOT = 387,
     NUMBER = 388,
     NUMBER_PER_IDENT = 389,
     NUMBER_PER_CIDR = 390,
     NUMBER_PER_IP = 391,
     NUMBER_PER_IP_GLOBAL = 392,
     OPERATOR = 393,
     OPERS_BYPASS_CALLERID = 394,
     OPER_LOG = 395,
     OPER_ONLY_UMODES = 396,
     OPER_PASS_RESV = 397,
     OPER_SPY_T = 398,
     OPER_UMODES = 399,
     JOIN_FLOOD_COUNT = 400,
     JOIN_FLOOD_TIME = 401,
     PACE_WAIT = 402,
     PACE_WAIT_SIMPLE = 403,
     PASSWORD = 404,
     PATH = 405,
     PING_COOKIE = 406,
     PING_TIME = 407,
     PING_WARNING = 408,
     PORT = 409,
     QSTRING = 410,
     QUIET_ON_BAN = 411,
     REASON = 412,
     REDIRPORT = 413,
     REDIRSERV = 414,
     REGEX_T = 415,
     REHASH = 416,
     TREJECT_HOLD_TIME = 417,
     REMOTE = 418,
     REMOTEBAN = 419,
     RESTRICT_CHANNELS = 420,
     RESTRICTED = 421,
     RSA_PRIVATE_KEY_FILE = 422,
     RSA_PUBLIC_KEY_FILE = 423,
     SSL_CERTIFICATE_FILE = 424,
     RESV = 425,
     RESV_EXEMPT = 426,
     SECONDS = 427,
     MINUTES = 428,
     HOURS = 429,
     DAYS = 430,
     WEEKS = 431,
     SENDQ = 432,
     SEND_PASSWORD = 433,
     SERVERHIDE = 434,
     SERVERINFO = 435,
     SERVLINK_PATH = 436,
     IRCD_SID = 437,
     TKLINE_EXPIRE_NOTICES = 438,
     T_SHARED = 439,
     T_CLUSTER = 440,
     TYPE = 441,
     SHORT_MOTD = 442,
     SILENT = 443,
     SPOOF = 444,
     SPOOF_NOTICE = 445,
     STATS_E_DISABLED = 446,
     STATS_I_OPER_ONLY = 447,
     STATS_K_OPER_ONLY = 448,
     STATS_O_OPER_ONLY = 449,
     STATS_P_OPER_ONLY = 450,
     TBOOL = 451,
     TMASKED = 452,
     T_REJECT = 453,
     TS_MAX_DELTA = 454,
     TS_WARN_DELTA = 455,
     TWODOTS = 456,
     T_ALL = 457,
     T_BOTS = 458,
     T_SOFTCALLERID = 459,
     T_CALLERID = 460,
     T_CCONN = 461,
     T_CLIENT_FLOOD = 462,
     T_DEAF = 463,
     T_DEBUG = 464,
     T_DRONE = 465,
     T_EXTERNAL = 466,
     T_FULL = 467,
     T_INVISIBLE = 468,
     T_IPV4 = 469,
     T_IPV6 = 470,
     T_LOCOPS = 471,
     T_LOGPATH = 472,
     T_L_CRIT = 473,
     T_L_DEBUG = 474,
     T_L_ERROR = 475,
     T_L_INFO = 476,
     T_L_NOTICE = 477,
     T_L_TRACE = 478,
     T_L_WARN = 479,
     T_MAX_CLIENTS = 480,
     T_NCHANGE = 481,
     T_OPERWALL = 482,
     T_REJ = 483,
     T_SERVNOTICE = 484,
     T_SERVCONN = 485,
     T_SKILL = 486,
     T_SPY = 487,
     T_SSL = 488,
     T_UMODES = 489,
     T_UNAUTH = 490,
     T_UNRESV = 491,
     T_UNXLINE = 492,
     T_WALLOP = 493,
     T_GOD = 494,
     T_NICKSERVREG = 495,
     THROTTLE_TIME = 496,
     TOPICBURST = 497,
     TRUE_NO_OPER_FLOOD = 498,
     TKLINE = 499,
     TXLINE = 500,
     TRESV = 501,
     UNKLINE = 502,
     USER = 503,
     USE_EGD = 504,
     USE_EXCEPT = 505,
     USE_INVEX = 506,
     USE_KNOCK = 507,
     USE_LOGGING = 508,
     USE_WHOIS_ACTUALLY = 509,
     VHOST = 510,
     VHOST6 = 511,
     XLINE = 512,
     WARN = 513,
     WARN_NO_NLINE = 514
   };
#endif
/* Tokens.  */
#define ACCEPT_PASSWORD 258
#define ACTION 259
#define ADMIN 260
#define AFTYPE 261
#define T_ALLOW 262
#define ANTI_NICK_FLOOD 263
#define ANTI_SPAM_EXIT_MESSAGE_TIME 264
#define AUTOCONN 265
#define T_BLOCK 266
#define BURST_AWAY 267
#define BURST_TOPICWHO 268
#define BYTES 269
#define KBYTES 270
#define MBYTES 271
#define GBYTES 272
#define TBYTES 273
#define CALLER_ID_WAIT 274
#define CAN_FLOOD 275
#define CAN_IDLE 276
#define CHANNEL 277
#define CIDR_BITLEN_IPV4 278
#define CIDR_BITLEN_IPV6 279
#define CIPHER_PREFERENCE 280
#define CLASS 281
#define COMPRESSED 282
#define COMPRESSION_LEVEL 283
#define CONNECT 284
#define CONNECTFREQ 285
#define CRYPTLINK 286
#define DEFAULT_CIPHER_PREFERENCE 287
#define DEFAULT_FLOODCOUNT 288
#define DEFAULT_SPLIT_SERVER_COUNT 289
#define DEFAULT_SPLIT_USER_COUNT 290
#define DENY 291
#define DESCRIPTION 292
#define DIE 293
#define DISABLE_AUTH 294
#define DISABLE_FAKE_CHANNELS 295
#define DISABLE_HIDDEN 296
#define DISABLE_LOCAL_CHANNELS 297
#define DISABLE_REMOTE_COMMANDS 298
#define DOT_IN_IP6_ADDR 299
#define DOTS_IN_IDENT 300
#define DURATION 301
#define EGDPOOL_PATH 302
#define EMAIL 303
#define ENABLE 304
#define ENCRYPTED 305
#define EXCEED_LIMIT 306
#define EXEMPT 307
#define FAILED_OPER_NOTICE 308
#define FAKENAME 309
#define IRCD_FLAGS 310
#define FLATTEN_LINKS 311
#define FFAILED_OPERLOG 312
#define FKILLLOG 313
#define FKLINELOG 314
#define FGLINELOG 315
#define FIOERRLOG 316
#define FOPERLOG 317
#define FOPERSPYLOG 318
#define FUSERLOG 319
#define GECOS 320
#define GENERAL 321
#define GLINE 322
#define GLINES 323
#define GLINE_EXEMPT 324
#define GLINE_LOG 325
#define GLINE_TIME 326
#define GLINE_MIN_CIDR 327
#define GLINE_MIN_CIDR6 328
#define GLOBAL_KILL 329
#define IRCD_AUTH 330
#define NEED_IDENT 331
#define HAVENT_READ_CONF 332
#define HIDDEN 333
#define HIDDEN_ADMIN 334
#define HIDDEN_NAME 335
#define HIDDEN_OPER 336
#define HIDE_SERVER_IPS 337
#define HIDE_SERVERS 338
#define HIDE_SPOOF_IPS 339
#define HOST 340
#define HUB 341
#define HUB_MASK 342
#define IDLETIME 343
#define IGNORE_BOGUS_TS 344
#define INVISIBLE_ON_CONNECT 345
#define IP 346
#define KILL 347
#define KILL_CHASE_TIME_LIMIT 348
#define KLINE 349
#define KLINE_EXEMPT 350
#define KLINE_REASON 351
#define KLINE_WITH_REASON 352
#define KNOCK_DELAY 353
#define KNOCK_DELAY_CHANNEL 354
#define LAZYLINK 355
#define LEAF_MASK 356
#define LINKS_DELAY 357
#define LISTEN 358
#define T_LOG 359
#define LOGGING 360
#define LOG_LEVEL 361
#define MAX_ACCEPT 362
#define MAX_BANS 363
#define MAX_CHANS_PER_USER 364
#define MAX_GLOBAL 365
#define MAX_IDENT 366
#define MAX_LOCAL 367
#define MAX_NICK_CHANGES 368
#define MAX_NICK_TIME 369
#define MAX_NUMBER 370
#define MAX_TARGETS 371
#define MESSAGE_LOCALE 372
#define MIN_NONWILDCARD 373
#define MIN_NONWILDCARD_SIMPLE 374
#define MODULE 375
#define MODULES 376
#define NAME 377
#define NEED_PASSWORD 378
#define NETWORK_DESC 379
#define NETWORK_NAME 380
#define NICK 381
#define NICK_CHANGES 382
#define NO_CREATE_ON_SPLIT 383
#define NO_JOIN_ON_SPLIT 384
#define NO_OPER_FLOOD 385
#define NO_TILDE 386
#define NOT 387
#define NUMBER 388
#define NUMBER_PER_IDENT 389
#define NUMBER_PER_CIDR 390
#define NUMBER_PER_IP 391
#define NUMBER_PER_IP_GLOBAL 392
#define OPERATOR 393
#define OPERS_BYPASS_CALLERID 394
#define OPER_LOG 395
#define OPER_ONLY_UMODES 396
#define OPER_PASS_RESV 397
#define OPER_SPY_T 398
#define OPER_UMODES 399
#define JOIN_FLOOD_COUNT 400
#define JOIN_FLOOD_TIME 401
#define PACE_WAIT 402
#define PACE_WAIT_SIMPLE 403
#define PASSWORD 404
#define PATH 405
#define PING_COOKIE 406
#define PING_TIME 407
#define PING_WARNING 408
#define PORT 409
#define QSTRING 410
#define QUIET_ON_BAN 411
#define REASON 412
#define REDIRPORT 413
#define REDIRSERV 414
#define REGEX_T 415
#define REHASH 416
#define TREJECT_HOLD_TIME 417
#define REMOTE 418
#define REMOTEBAN 419
#define RESTRICT_CHANNELS 420
#define RESTRICTED 421
#define RSA_PRIVATE_KEY_FILE 422
#define RSA_PUBLIC_KEY_FILE 423
#define SSL_CERTIFICATE_FILE 424
#define RESV 425
#define RESV_EXEMPT 426
#define SECONDS 427
#define MINUTES 428
#define HOURS 429
#define DAYS 430
#define WEEKS 431
#define SENDQ 432
#define SEND_PASSWORD 433
#define SERVERHIDE 434
#define SERVERINFO 435
#define SERVLINK_PATH 436
#define IRCD_SID 437
#define TKLINE_EXPIRE_NOTICES 438
#define T_SHARED 439
#define T_CLUSTER 440
#define TYPE 441
#define SHORT_MOTD 442
#define SILENT 443
#define SPOOF 444
#define SPOOF_NOTICE 445
#define STATS_E_DISABLED 446
#define STATS_I_OPER_ONLY 447
#define STATS_K_OPER_ONLY 448
#define STATS_O_OPER_ONLY 449
#define STATS_P_OPER_ONLY 450
#define TBOOL 451
#define TMASKED 452
#define T_REJECT 453
#define TS_MAX_DELTA 454
#define TS_WARN_DELTA 455
#define TWODOTS 456
#define T_ALL 457
#define T_BOTS 458
#define T_SOFTCALLERID 459
#define T_CALLERID 460
#define T_CCONN 461
#define T_CLIENT_FLOOD 462
#define T_DEAF 463
#define T_DEBUG 464
#define T_DRONE 465
#define T_EXTERNAL 466
#define T_FULL 467
#define T_INVISIBLE 468
#define T_IPV4 469
#define T_IPV6 470
#define T_LOCOPS 471
#define T_LOGPATH 472
#define T_L_CRIT 473
#define T_L_DEBUG 474
#define T_L_ERROR 475
#define T_L_INFO 476
#define T_L_NOTICE 477
#define T_L_TRACE 478
#define T_L_WARN 479
#define T_MAX_CLIENTS 480
#define T_NCHANGE 481
#define T_OPERWALL 482
#define T_REJ 483
#define T_SERVNOTICE 484
#define T_SERVCONN 485
#define T_SKILL 486
#define T_SPY 487
#define T_SSL 488
#define T_UMODES 489
#define T_UNAUTH 490
#define T_UNRESV 491
#define T_UNXLINE 492
#define T_WALLOP 493
#define T_GOD 494
#define T_NICKSERVREG 495
#define THROTTLE_TIME 496
#define TOPICBURST 497
#define TRUE_NO_OPER_FLOOD 498
#define TKLINE 499
#define TXLINE 500
#define TRESV 501
#define UNKLINE 502
#define USER 503
#define USE_EGD 504
#define USE_EXCEPT 505
#define USE_INVEX 506
#define USE_KNOCK 507
#define USE_LOGGING 508
#define USE_WHOIS_ACTUALLY 509
#define VHOST 510
#define VHOST6 511
#define XLINE 512
#define WARN 513
#define WARN_NO_NLINE 514




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 134 "ircd_parser.y"
typedef union YYSTYPE {
  int number;
  char *string;
} YYSTYPE;
/* Line 1447 of yacc.c.  */
#line 561 "y.tab.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



