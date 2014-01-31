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

/**
 * @file
 * xform token manager
 */

#include <xmms/xmms_log.h>
#include <xmms/xmms_object.h>
#include <xmmspriv/xmms_medialib.h>

#include <xmmspriv/xmms_tokenmanager.h>

#include <string.h>

typedef struct xmms_xform_token_node_St xmms_xform_token_node_t;

#define ORIGIN_TOKEN 0

struct xmms_xform_token_manager_St {
	xmms_medialib_t *medialib;

	GMutex lock;

	/** The latest metadata emitted by an xform */
	xmms_xform_token_node_t *head;
	/** The oldest metadata that has been reacted upon */
	xmms_xform_token_node_t *cursor;
	/** The oldest persistent metadata that has been written to the media library */
	xmms_xform_token_node_t *persistent_cursor;
};

struct xmms_xform_token_node_St {
	xmms_xform_token_t token;
	xmms_xform_token_node_t *prev;

	guint8 order;
	gchar *source;
	gchar *key;
	xmmsv_t *value;
	xmms_xform_metadata_lifetime_t value_lifetime;
};

xmms_xform_token_manager_t *
xmms_xform_token_manager_new (xmms_medialib_t *medialib)
{
	xmms_xform_token_manager_t *manager = g_new0 (xmms_xform_token_manager_t, 1);

	g_mutex_init (&manager->lock);

	xmms_object_ref (medialib);
	manager->medialib = medialib;

	return manager;
}

static void
xmms_xform_token_manager_clear (xmms_xform_token_manager_t *manager)
{
	xmms_xform_token_node_t *node, *next;

	g_mutex_lock (&manager->lock);

	node = manager->head;
	while (node != NULL) {
		next = node->prev;

		xmmsv_unref (node->value);
		g_free (node->source);
		g_free (node->key);
		g_free (node);

		node = next;
	}

	manager->head = manager->cursor = manager->persistent_cursor = NULL;

	g_mutex_unlock (&manager->lock);
}

static void
xmms_xform_token_manager_parse_argument (xmms_xform_token_manager_t *manager,
                                         gchar *param)
{
	gchar *v = strchr (param, '=');
	if (v) {
		*v = 0;
		v++;
		xmms_xform_token_manager_set_origin_string (manager, "server",
		                                            param, v);
	} else {
		xmms_xform_token_manager_set_origin_int (manager, "server",
		                                         param, 1);
	}
}

static void
xmms_xform_token_manager_parse_url (xmms_xform_token_manager_t *manager,
                                    xmmsv_t *value)
{
	const gchar *url;
	gchar *durl, *args, **params;
	gint i;

	if (!xmmsv_get_string (value, &url))
		return;

	durl = g_strdup (url);

	args = strchr (durl, '?');
	if (args) {
		*args = 0;
		args++;

		if (!xmms_medialib_decode_url (args))
			goto cleanup;

		params = g_strsplit (args, "&", 0);
		for (i = 0; params[i]; i++)
			xmms_xform_token_manager_parse_argument (manager, params[i]);
		g_strfreev (params);
	}

	if (!xmms_medialib_decode_url (durl))
		goto cleanup;

	xmms_xform_token_manager_set_origin_string (manager, "server",
	                                            "url", durl);
cleanup:
	g_free (durl);
}


static void
xmms_xform_token_manager_add_origin (const gchar *source, const gchar *key,
                                     xmmsv_t *value, void *udata)
{
	xmms_xform_token_manager_t *manager = (xmms_xform_token_manager_t *) udata;
	if (!source)
		return; /* XXX: This is song_id which is the S4 internal attribute set without source, wrong place to handle */

	if (g_strcmp0 ("url", key) == 0) /* need special care to decode and handle args */ {
		xmms_xform_token_manager_parse_url (manager, value);
	}
	else
		xmms_xform_token_manager_set_value (manager, 0, source, key, xmmsv_ref (value),
		                                    XMMS_XFORM_METADATA_LIFETIME_ORIGIN);
}

void
xmms_xform_token_manager_reset (xmms_xform_token_manager_t *manager,
                                xmms_medialib_session_t *session,
                                xmms_medialib_entry_t entry)
{
	xmms_xform_token_manager_clear (manager);
	xmms_xform_token_manager_set_int (manager, 0, "server", "id", entry,
	                                  XMMS_XFORM_METADATA_LIFETIME_ORIGIN);
	xmms_medialib_entry_property_foreach (session, entry,
	                                      xmms_xform_token_manager_add_origin,
	                                      manager);
}

void
xmms_xform_token_manager_free (xmms_xform_token_manager_t *manager)
{
	xmms_xform_token_manager_clear (manager);
	xmms_object_unref (manager->medialib);
	g_mutex_clear (&manager->lock);
	g_free (manager);
}

static gboolean
add_if_missing (xmmsv_t **key_dict, const gchar *source,
                const gchar *key, xmmsv_t *value)
{
	xmmsv_t *source_dict;

	if (!*key_dict)
		*key_dict = xmmsv_new_dict ();

	if (!xmmsv_dict_get (*key_dict, key, &source_dict)) {
		source_dict = xmmsv_new_dict ();
		xmmsv_dict_set (*key_dict, source, source_dict);
		xmmsv_unref (source_dict);
	}

	if (!xmmsv_dict_has_key (source_dict, source)) {
		xmmsv_dict_set (source_dict, source, value);
		return TRUE;
	}

	return FALSE;
}

/**
 * Drop duplicate metadata from the token manager.
 */
static void
xmms_xform_token_manager_compress (xmms_xform_token_manager_t *manager)
{
	/* TODO: Implement me. Needed? */
}

static gboolean
xmms_xform_token_manager_persist (xmms_xform_token_manager_t *manager, xmmsv_t *persist)
{
	/* TODO: should this really be implemented here? */
	return TRUE;
}

/**
 * React to the metadata created between cursor up to the specified token.
 *
 * TODO: Should we or someone else write to the media library?
 * @return TRUE if
 */
gboolean
xmms_xform_token_manager_react (xmms_xform_token_manager_t *manager,
                                xmms_xform_token_t token)
{
	xmms_xform_token_node_t *node, *next;
	xmmsv_t *current_info;
	xmmsv_t *persist;
	gboolean result = FALSE;

	current_info = persist = NULL;

	g_mutex_lock (&manager->lock);

	/* Rewind until we find the node matching the token, or we hit the cursor
	 * of the previous run.
	 */
	for (node = manager->head; node && node != manager->cursor; node = node->prev) {
		if (node->token >= token)
			break;
	}

	next = node;

	/* Collect metadata between cursor and selected node */
	for (; node && node != manager->cursor; node = node->prev) {
		if (add_if_missing (&current_info, node->key, node->source, node->value)) {
			const gchar *s;
			int64_t n;

			if (xmmsv_get_string (node->value, &s))
				XMMS_DBG ("[%s] %s = %s", node->source, node->key, s);
			else if (xmmsv_get_int64 (node->value, &n))
				XMMS_DBG ("[%s] %s = %lld", node->source, node->key, n);

			if (node->value_lifetime == XMMS_XFORM_METADATA_LIFETIME_VOLATILE) {
				/* TODO: Implement me? */
			}
		}

		if (node->value_lifetime == XMMS_XFORM_METADATA_LIFETIME_PERSISTENT) {
			add_if_missing (&persist, node->key, node->source, node->value);
		}
	}

	if (next)
		manager->cursor = next;

	if (persist) {
		result = xmms_xform_token_manager_persist (manager, persist);
		xmms_xform_token_manager_compress (manager);
	}

	g_mutex_unlock (&manager->lock);

	if (persist)
		xmmsv_unref (persist);
	if (current_info)
		xmmsv_unref (current_info);

	return result;
}

xmms_xform_token_t
xmms_xform_token_manager_set_value (xmms_xform_token_manager_t *manager,
                                    guint8 order, const gchar *source,
                                    const gchar *key, xmmsv_t *value,
                                    xmms_xform_metadata_lifetime_t lifetime)
{
	xmms_xform_token_node_t *node;

	node = g_new0 (xmms_xform_token_node_t, 1);
	node->source = g_strdup (source);
	node->key = g_strdup (key);
	node->value = value;
	node->value_lifetime = lifetime;
	node->order = order;

	g_mutex_lock (&manager->lock);
	if (lifetime == XMMS_XFORM_METADATA_LIFETIME_ORIGIN) {
		node->token = ORIGIN_TOKEN;
	} else {
		node->token = (manager->head ? manager->head->token : 0) + 1;
	}
	node->prev = manager->head;
	manager->head = node;
	g_mutex_unlock (&manager->lock);

	return node->token;
}

xmms_xform_token_t
xmms_xform_token_manager_set_int (xmms_xform_token_manager_t *manager,
                                  guint8 order, const gchar *source,
                                  const gchar *key, int64_t value,
                                  xmms_xform_metadata_lifetime_t lifetime)
{
	return xmms_xform_token_manager_set_value (manager, order, source, key,
	                                           xmmsv_new_int (value),
	                                           lifetime);
}

xmms_xform_token_t
xmms_xform_token_manager_set_string (xmms_xform_token_manager_t *manager,
                                     guint8 order, const gchar *source,
                                     const gchar *key, const gchar *value,
                                     xmms_xform_metadata_lifetime_t lifetime)
{
	/* TODO: Could check if string has already been set, if so, avoid alloc */
	return xmms_xform_token_manager_set_value (manager, order, source, key,
	                                           xmmsv_new_string (value),
	                                           lifetime);
}

static gboolean
xmms_xform_token_manager_get_value (xmms_xform_token_manager_t *manager,
                                    xmms_xform_token_t token,
                                    guint8 order, const gchar *source,
                                    const gchar *key, xmmsv_t **value,
                                    gboolean persist)
{
	xmms_xform_token_node_t *node;
	gboolean result = FALSE;

	for (node = manager->head; node; node = node->prev) {
		/* A source may access origin data of its own source or "server" regardless order. */
		if (node->token == ORIGIN_TOKEN) {
			if (g_strcmp0 (node->source, source) != 0)
				if (g_strcmp0 (node->source, "server") != 0)
					continue;
		} else {
			/* Token of metadata must be older or equal to passed token. */
			if (node->token > token)
				continue;
			/* Order must be same or earlier in the chain to be accessed. */
			if (node->order > order)
				continue;
		}

		if (g_strcmp0 (node->key, key) == 0) {
			*value = node->value;
			result = TRUE;
			break;
		}
	}

	return result;
}

gboolean
xmms_xform_token_manager_has_value (xmms_xform_token_manager_t *manager,
                                    xmms_xform_token_t token,
                                    guint8 order, const gchar *source,
                                    const gchar *key)
{
	gboolean result = FALSE;
	xmmsv_t *value;

	g_mutex_lock (&manager->lock);
	result = xmms_xform_token_manager_get_value (manager, token, order, source, key, &value, FALSE);
	g_mutex_unlock (&manager->lock);

	return result;
}

gboolean
xmms_xform_token_manager_get_int (xmms_xform_token_manager_t *manager,
                                  xmms_xform_token_t token,
                                  guint8 order, const gchar *source,
                                  const gchar *key, int64_t *number)
{
	gboolean result = FALSE;
	xmmsv_t *value;

	g_mutex_lock (&manager->lock);

	if (xmms_xform_token_manager_get_value (manager, token, order, source, key, &value, FALSE)) {
		if (xmmsv_get_int64 (value, number)) {
			result = TRUE;
		}
	}

	g_mutex_unlock (&manager->lock);

	return result;
}

gboolean
xmms_xform_token_manager_get_string (xmms_xform_token_manager_t *manager,
                                     xmms_xform_token_t token,
                                     guint8 order, const gchar *source,
                                     const gchar *key, gchar **string)
{
	gboolean result = FALSE;
	xmmsv_t *value;
	const gchar *original;

	g_mutex_lock (&manager->lock);

	if (xmms_xform_token_manager_get_value (manager, token, order, source, key, &value, FALSE)) {
		if (xmmsv_get_string (value, &original)) {
			*string = g_strdup (original);
			result = TRUE;
		}
	}

	g_mutex_unlock (&manager->lock);

	return result;
}
