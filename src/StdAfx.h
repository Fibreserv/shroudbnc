/*******************************************************************************
 * shroudBNC - an object-oriented framework for IRC                            *
 * Copyright (C) 2005 Gunnar Beutner                                           *
 *                                                                             *
 * This program is free software; you can redistribute it and/or               *
 * modify it under the terms of the GNU General Public License                 *
 * as published by the Free Software Foundation; either version 2              *
 * of the License, or (at your option) any later version.                      *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU General Public License for more details.                                *
 *                                                                             *
 * You should have received a copy of the GNU General Public License           *
 * along with this program; if not, write to the Free Software                 *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. *
 *******************************************************************************/

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers

#include "../config.h"

#ifdef _MSC_VER
#pragma warning ( disable : 4996 )

#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1

/*
 * some newer visual c++ versions define time_t as a 64 bit integer
 * while adns still expects a 32 bit integer
 * -> sbnc crashes while accessing the adns_answer structure
 */
#define _USE_32BIT_TIME_T
#endif

#ifndef _MSC_VER
	#include "libltdl/ltdl.h"
#endif

#ifdef _WIN32
	#include "win32.h"
#else
	#include "unix.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>

#ifdef USESSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#else
typedef void SSL;
typedef void SSL_CTX;
typedef void X509;
typedef void X509_STORE_CTX;
#endif

#ifdef _WIN32
#define HAVE_AF_INET6
#define HAVE_STRUCT_IN6_ADDR
#define HAVE_STRUCT_SOCKADDR_IN6
#endif

#if defined(HAVE_AF_INET6) && defined(HAVE_STRUCT_IN6_ADDR) && defined(HAVE_STRUCT_SOCKADDR_IN6)
#define IPV6
#endif

#include "snprintf.h"

#ifndef SWIG
#include "c-ares/ares.h"
#endif

// Do NOT use sprintf.
#define sprintf __evil_function

#if defined(_WIN32) && defined(_DEBUG) && defined(SBNC)
void *DebugMalloc(size_t Size);
void DebugFree(void *p);
char *DebugStrDup(const char *p);
void *DebugReAlloc(void *p, size_t newsize);
bool ReportMemory(time_t Now, void *Cookie);

#define real_malloc malloc
#define real_free free
#define real_strdup strdup
#define real_realloc realloc

#define malloc DebugMalloc
#define free DebugFree
#undef strdup
#define strdup DebugStrDup
#define realloc DebugReAlloc
#endif

#include "sbncloader/AssocArray.h"

#include "Vector.h"
#include "Hashtable.h"
#include "utility.h"
#include "OwnedObject.h"
#include "SocketEvents.h"
#include "DnsEvents.h"
#include "Timer.h"
#include "Connection.h"
#include "ClientConnection.h"
#include "IRCConnection.h"
#include "User.h"
#include "Core.h"
#include "Log.h"
#include "Config.h"
#include "ModuleFar.h"
#include "Module.h"
#include "Channel.h"
#include "Nick.h"
#include "Keyring.h"
#include "Banlist.h"
#include "IdentSupport.h"
#include "Match.h"
#include "TrafficStats.h"
#include "FIFOBuffer.h"
#include "Queue.h"
#include "FloodControl.h"
#include "Listener.h"
#include "Persistable.h"
