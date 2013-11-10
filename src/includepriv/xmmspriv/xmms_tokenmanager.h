/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2013 XMMS2 Team
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
#ifndef __XMMS_XFORM_TOKEN_MANAGER_H__
#define __XMMS_XFORM_TOKEN_MANAGER_H__

#include <xmmspriv/xmms_medialib.h>

typedef struct xmms_xform_token_manager_St xmms_xform_token_manager_t;
typedef uint64_t xmms_xform_token_t;

typedef enum {
	XMMS_XFORM_METADATA_LIFETIME_ORIGIN,
	XMMS_XFORM_METADATA_LIFETIME_PERSISTENT,
	XMMS_XFORM_METADATA_LIFETIME_VOLATILE
} xmms_xform_metadata_lifetime_t;

xmms_xform_token_manager_t *xmms_xform_token_manager_new (xmms_medialib_t *medialib);
void xmms_xform_token_manager_free (xmms_xform_token_manager_t *manager);

gboolean xmms_xform_token_manager_react (xmms_xform_token_manager_t *manager, xmms_xform_token_t token);

gboolean xmms_xform_token_manager_has_value (xmms_xform_token_manager_t *manager, xmms_xform_token_t token, guint8 order, const gchar *source, const gchar *key);
gboolean xmms_xform_token_manager_get_int (xmms_xform_token_manager_t *manager, xmms_xform_token_t token, guint8 order, const gchar *source, const gchar *key, int64_t *value);
gboolean xmms_xform_token_manager_get_string (xmms_xform_token_manager_t *manager, xmms_xform_token_t token, guint8 order, const gchar *source, const gchar *key, gchar **value);

xmms_xform_token_t xmms_xform_token_manager_set_int (xmms_xform_token_manager_t *manager, guint8 order, const gchar *source, const gchar *key, int64_t value, xmms_xform_metadata_lifetime_t lifetime);
xmms_xform_token_t xmms_xform_token_manager_set_string (xmms_xform_token_manager_t *manager, guint8 order, const gchar *source, const gchar *key, const gchar *value, xmms_xform_metadata_lifetime_t lifetime);

#define xmms_xform_token_manager_set_origin_int(manager, source, key, value) \
	xmms_xform_token_manager_set_int (manager, 0, source, key, value, XMMS_XFORM_METADATA_LIFETIME_ORIGIN)

#define xmms_xform_token_manager_set_persistent_int(manager, order, source, key, value) \
	xmms_xform_token_manager_set_int (manager, order, source, key, value, XMMS_XFORM_METADATA_LIFETIME_PERSISTENT)

#define xmms_xform_token_manager_set_volatile_int(manager, order, source, key, value) \
	xmms_xform_token_manager_set_int (manager, order, source, key, value, XMMS_XFORM_METADATA_LIFETIME_VOLATILE)

#define xmms_xform_token_manager_set_origin_string(manager, source, key, value) \
	xmms_xform_token_manager_set_string (manager, 0, source, key, value, XMMS_XFORM_METADATA_LIFETIME_ORIGIN)

#define xmms_xform_token_manager_set_persistent_string(manager, order, source, key, value) \
	xmms_xform_token_manager_set_string (manager, order, source, key, value, XMMS_XFORM_METADATA_LIFETIME_PERSISTENT)

#define xmms_xform_token_manager_set_volatile_string(manager, order, source, key, value) \
	xmms_xform_token_manager_set_string (manager, order, source, key, value, XMMS_XFORM_METADATA_LIFETIME_VOLATILE)

#endif
