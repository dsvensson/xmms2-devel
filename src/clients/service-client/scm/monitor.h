/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2007 XMMS2 Team
 *
 *  PLUGINS ARE NOT CONSIDERED TO BE DERIVED WORK !!!
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 */

#ifndef __MONITOR_H__
#define __MONITOR_H__

#include "xmms_configuration.h"
#include "common.h"
#include "config.h"

#ifdef INOTIFY
#include "inotify.h"
#include "inotify-syscalls.h"
#endif

gboolean start_monitor (GHashTable *clients);
void shutdown_monitor (void);
void cb_poll (xmmsc_connection_t *conn, xmmsc_result_t *res,
              xmmsc_service_method_t *method, void *data);

#endif