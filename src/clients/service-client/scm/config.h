/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2008 XMMS2 Team
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common.h"

const gchar *config_dir (void);
gboolean read_all (info_t *info);
config_t *read_config (const gchar *name);
gboolean create_config (const gchar *name, const gchar *contents);
void free_config (gpointer v);
void free_service (gpointer v);
void free_method (gpointer v);

#endif
