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
 * xforms
 */

#include <string.h>

#include <xmmspriv/xmms_plugin.h>
#include <xmmspriv/xmms_xform.h>
#include <xmmspriv/xmms_streamtype.h>
#include <xmmspriv/xmms_utils.h>
#include <xmmspriv/xmms_xform_plugin.h>
#include <xmms/xmms_ipc.h>
#include <xmms/xmms_log.h>
#include <xmms/xmms_object.h>

struct xmms_xform_St {
	xmms_object_t obj;
	struct xmms_xform_St *prev;

	const xmms_xform_plugin_t *plugin;

	guint8 order;

	xmms_xform_token_manager_t *manager;
	xmms_xform_token_t token;

	gboolean inited;

	void *priv;

	xmms_stream_type_t *out_type;

	GPtrArray *stream_type_goals;

	gboolean eos;
	gboolean error;

	char *buffer;
	gint buffered;
	gint buffersize;

	GHashTable *privdata;
	GQueue *hotspots;

	xmmsv_t *browse_list;
	xmmsv_t *browse_dict;
	gint browse_index;

	/** used for line reading */
	struct {
		gchar buf[XMMS_XFORM_MAX_LINE_SIZE];
		gchar *bufend;
	} lr;
};

typedef struct xmms_xform_hotspot_St {
	guint pos;
	gchar *key;
	xmmsv_t *obj;
} xmms_xform_hotspot_t;

#define READ_CHUNK 4096


const char *xmms_xform_shortname (xmms_xform_t *xform);
static void xmms_xform_destroy (xmms_object_t *object);

static xmms_xform_t *
xmms_xform_directory_listing_trampoline_new (const gchar *url)
{
	xmms_xform_t *xform;

	xform = xmms_object_new (xmms_xform_t, xmms_xform_destroy);

	xmms_xform_outdata_type_add (xform,
	                             XMMS_STREAM_TYPE_MIMETYPE, "application/x-url",
	                             XMMS_STREAM_TYPE_URL, url,
	                             XMMS_STREAM_TYPE_END);
	return xform;
}

static xmms_xform_t *
xmms_xform_playlist_entries_trampoline_new (const gchar *url)
{
	xmms_stream_type_t *stream_type;
	GPtrArray *stream_type_goals;
	xmms_xform_t *xform;

	stream_type = xmms_stream_type_new (XMMS_STREAM_TYPE_BEGIN,
	                                    XMMS_STREAM_TYPE_MIMETYPE,
	                                    "application/x-xmms2-playlist-entries",
	                                    XMMS_STREAM_TYPE_END);

	stream_type_goals = g_ptr_array_new_with_free_func (xmms_object_destroy_notify);
	g_ptr_array_add (stream_type_goals, stream_type);

	xform = xmms_xform_directory_listing_trampoline_new (url);
	xform->stream_type_goals = stream_type_goals;

	return xform;
}

void
xmms_xform_browse_add_entry_property_str (xmms_xform_t *xform,
                                          const gchar *key,
                                          const gchar *value)
{
	xmmsv_t *val = xmmsv_new_string (value);
	xmms_xform_browse_add_entry_property (xform, key, val);
	xmmsv_unref (val);
}


void
xmms_xform_browse_add_entry_property_int (xmms_xform_t *xform,
                                          const gchar *key,
                                          gint value)
{
	xmmsv_t *val = xmmsv_new_int (value);
	xmms_xform_browse_add_entry_property (xform, key, val);
	xmmsv_unref (val);
}

void
xmms_xform_browse_add_symlink_args (xmms_xform_t *xform, const gchar *basename,
                                    const gchar *url, gint nargs, gchar **args)
{
	GString *s;
	gchar *eurl;
	gchar bname[32];
	gint i;

	if (!basename) {
		g_snprintf (bname, sizeof (bname), "%d", xform->browse_index++);
		basename = bname;
	}

	xmms_xform_browse_add_entry (xform, basename, 0);
	eurl = xmms_medialib_url_encode (url);
	s = g_string_new (eurl);

	for (i = 0; i < nargs; i++) {
		g_string_append_c (s, i == 0 ? '?' : '&');
		g_string_append (s, args[i]);
	}

	xmms_xform_browse_add_entry_property_str (xform, "realpath", s->str);

	g_free (eurl);
	g_string_free (s, TRUE);
}

void
xmms_xform_browse_add_symlink (xmms_xform_t *xform, const gchar *basename,
                               const gchar *url)
{
	xmms_xform_browse_add_symlink_args (xform, basename, url, 0, NULL);
}

void
xmms_xform_browse_add_entry_property (xmms_xform_t *xform, const gchar *key,
                                      xmmsv_t *val)
{
	g_return_if_fail (xform);
	g_return_if_fail (xform->browse_dict);
	g_return_if_fail (key);
	g_return_if_fail (val);

	xmmsv_dict_set (xform->browse_dict, key, val);
}

void
xmms_xform_browse_add_entry (xmms_xform_t *xform, const gchar *filename,
                             guint32 flags)
{
	const gchar *url;
	gchar *efile, *eurl, *t;
	gint l, isdir;

	g_return_if_fail (filename);

	t = strchr (filename, '/');
	g_return_if_fail (!t); /* filenames can't contain '/', can they? */

	url = xmms_xform_get_url (xform);
	g_return_if_fail (url);

	xform->browse_dict = xmmsv_new_dict ();

	eurl = xmms_medialib_url_encode (url);
	efile = xmms_medialib_url_encode (filename);

	/* can't use g_build_filename as we need to preserve
	   slashes stuff like file:/// */
	l = strlen (url);
	if (l && url[l - 1] == '/') {
		t = g_strdup_printf ("%s%s", eurl, efile);
	} else {
		t = g_strdup_printf ("%s/%s", eurl, efile);
	}

	isdir = !!(flags & XMMS_XFORM_BROWSE_FLAG_DIR);
	xmms_xform_browse_add_entry_property_str (xform, "path", t);
	xmms_xform_browse_add_entry_property_int (xform, "isdir", isdir);

	if (xform->browse_list == NULL) {
		xform->browse_list = xmmsv_new_list ();
	}
	xmmsv_list_append (xform->browse_list, xform->browse_dict);
	xmmsv_unref (xform->browse_dict);

	g_free (t);
	g_free (efile);
	g_free (eurl);
}

static gint
xmms_browse_list_sortfunc (xmmsv_t **a, xmmsv_t **b)
{
	xmmsv_t *val1, *val2;
	const gchar *s1, *s2;
	int r1, r2;

	g_return_val_if_fail (xmmsv_is_type (*a, XMMSV_TYPE_DICT), 0);
	g_return_val_if_fail (xmmsv_is_type (*b, XMMSV_TYPE_DICT), 0);

	r1 = xmmsv_dict_get (*a, "intsort", &val1);
	r2 = xmmsv_dict_get (*b, "intsort", &val2);

	if (r1 && r2) {
		gint i1, i2;

		if (!xmmsv_get_int (val1, &i1))
			return 0;
		if (!xmmsv_get_int (val2, &i2))
			return 0;
		return i1 > i2;
	}

	if (!xmmsv_dict_get (*a, "path", &val1))
		return 0;
	if (!xmmsv_dict_get (*b, "path", &val2))
		return 0;

	if (!xmmsv_get_string (val1, &s1))
		return 0;
	if (!xmmsv_get_string (val2, &s2))
		return 0;

	return xmms_natcmp (s1, s2);
}

xmmsv_t *
xmms_xform_browse_method (xmms_xform_t *xform, const gchar *url,
                          xmms_error_t *error)
{
	xmmsv_t *list = NULL;

	if (xmms_xform_plugin_can_browse (xform->plugin)) {
		xform->browse_list = xmmsv_new_list ();
		if (!xmms_xform_plugin_browse (xform->plugin, xform, url, error)) {
			return NULL;
		}
		list = xform->browse_list;
		xform->browse_list = NULL;
		xmmsv_list_sort (list, xmms_browse_list_sortfunc);
	} else {
		xmms_error_set (error, XMMS_ERROR_GENERIC, "Couldn't handle that URL");
	}

	return list;
}

xmmsv_t *
xmms_xform_browse (const gchar *url, xmms_error_t *error)
{
	xmms_xform_token_manager_t *manager;
	xmms_xform_t *trampoline, *xform;
	xmmsv_t *list;

	manager = xmms_xform_token_manager_new (NULL);
	trampoline = xmms_xform_directory_listing_trampoline_new (url);

	xform = xmms_xform_find (trampoline, manager, NULL);
	xmms_object_unref (trampoline);

	if (!xform) {
		xmms_error_set (error, XMMS_ERROR_GENERIC, "Couldn't handle that URL");
		xmms_object_unref (xform);
		xmms_xform_token_manager_free (manager);
		return NULL;
	}

	XMMS_DBG ("found xform %s", xmms_xform_shortname (xform));

	list = xmms_xform_browse_method (xform, url, error);

	xmms_object_unref (xform);
	xmms_xform_token_manager_free (manager);

	return list;
}

static void
xmms_xform_destroy (xmms_object_t *object)
{
	xmms_xform_t *xform = (xmms_xform_t *)object;

	XMMS_DBG ("Freeing xform '%s'", xmms_xform_shortname (xform));

	/* The 'destroy' method is not mandatory */
	if (xform->plugin && xform->inited) {
		if (xmms_xform_plugin_can_destroy (xform->plugin)) {
			xmms_xform_plugin_destroy (xform->plugin, xform);
		}
	}

	if (xform->stream_type_goals) {
		g_ptr_array_unref (xform->stream_type_goals);
	}

	if (xform->privdata) {
		g_hash_table_destroy (xform->privdata);
	}

	if (xform->hotspots) {
		g_queue_free (xform->hotspots);
	}

	if (xform->buffer) {
		g_free (xform->buffer);
	}

	xmms_object_unref (xform->out_type);
	xmms_object_unref (xform->plugin);

	if (xform->prev) {
		xmms_object_unref (xform->prev);
	}

}

xmms_xform_t *
xmms_xform_new (xmms_xform_plugin_t *plugin, xmms_xform_t *prev,
                xmms_xform_token_manager_t *manager,
                GPtrArray *stream_type_goals)
{
	xmms_xform_t *xform;

	g_return_val_if_fail (plugin, NULL);
	g_return_val_if_fail (manager, NULL);

	xform = xmms_object_new (xmms_xform_t, xmms_xform_destroy);

	xmms_object_ref (plugin);
	xform->plugin = plugin;
	xform->manager = manager;
	xform->lr.bufend = &xform->lr.buf[0];

	if (stream_type_goals) {
		xform->stream_type_goals = g_ptr_array_ref (stream_type_goals);
	}

	if (prev) {
		xmms_object_ref (prev);
		xform->prev = prev;
	}

	xform->privdata = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                         g_free,
	                                         (GDestroyNotify) xmmsv_unref);
	xform->hotspots = g_queue_new ();

	if (!xmms_xform_plugin_init (xform->plugin, xform)) {
		xmms_object_unref (xform);
		return NULL;
	}

	xform->inited = TRUE;

	g_return_val_if_fail (xform->out_type, NULL);

	xform->buffer = g_malloc (READ_CHUNK);
	xform->buffersize = READ_CHUNK;

	return xform;
}

gpointer
xmms_xform_private_data_get (xmms_xform_t *xform)
{
	return xform->priv;
}

void
xmms_xform_private_data_set (xmms_xform_t *xform, gpointer data)
{
	xform->priv = data;
}

void
xmms_xform_outdata_type_add (xmms_xform_t *xform, ...)
{
	va_list ap;
	va_start (ap, xform);
	xform->out_type = xmms_stream_type_parse (ap);
	va_end (ap);
}

void
xmms_xform_outdata_type_set (xmms_xform_t *xform, xmms_stream_type_t *type)
{
	xmms_object_ref (type);
	xform->out_type = type;
}

void
xmms_xform_outdata_type_copy (xmms_xform_t *xform)
{
	xmms_object_ref (xform->prev->out_type);
	xform->out_type = xform->prev->out_type;
}

const char *
xmms_xform_indata_find_str (xmms_xform_t *xform, xmms_stream_type_key_t key)
{
	const gchar *r;
	r = xmms_stream_type_get_str (xform->prev->out_type, key);
	if (r) {
		return r;
	} else if (xform->prev) {
		return xmms_xform_indata_find_str (xform->prev, key);
	}
	return NULL;
}

const char *
xmms_xform_indata_get_str (xmms_xform_t *xform, xmms_stream_type_key_t key)
{
	return xmms_stream_type_get_str (xform->prev->out_type, key);
}

gint
xmms_xform_indata_get_int (xmms_xform_t *xform, xmms_stream_type_key_t key)
{
	return xmms_stream_type_get_int (xform->prev->out_type, key);
}

xmms_stream_type_t *
xmms_xform_outtype_get (xmms_xform_t *xform)
{
	return xform->out_type;
}

xmms_stream_type_t *
xmms_xform_intype_get (xmms_xform_t *xform)
{
	return xmms_xform_outtype_get (xform->prev);
}



const char *
xmms_xform_outtype_get_str (xmms_xform_t *xform, xmms_stream_type_key_t key)
{
	return xmms_stream_type_get_str (xform->out_type, key);
}

gint
xmms_xform_outtype_get_int (xmms_xform_t *xform, xmms_stream_type_key_t key)
{
	return xmms_stream_type_get_int (xform->out_type, key);
}

gboolean
xmms_xform_metadata_mapper_match (xmms_xform_t *xform, const gchar *key, const gchar *value, gsize length)
{
	return xmms_xform_plugin_metadata_mapper_match (xform->plugin, xform, key, value, length);
}

gboolean
xmms_xform_metadata_set_int (xmms_xform_t *xform, const char *key, int64_t value)
{
	xform->token = xmms_xform_token_manager_set_persistent_int (xform->manager, xform->order,
	                                                            xmms_xform_shortname (xform),
	                                                            key, value);
	return TRUE;
}

gboolean
xmms_xform_metadata_set_str (xmms_xform_t *xform, const char *key,
                             const char *val)
{
	if (!g_utf8_validate (val, -1, NULL)) {
		xmms_log_error ("xform '%s' tried to set property '%s' to a NON UTF-8 string!", xmms_xform_shortname (xform), key);
		return FALSE;
	}

	xform->token = xmms_xform_token_manager_set_persistent_string (xform->manager, xform->order,
	                                                               xmms_xform_shortname (xform),
	                                                               key, val);
	return TRUE;
}

gboolean
xmms_xform_metadata_has_val (xmms_xform_t *xform, const gchar *key)
{
	return xmms_xform_token_manager_has_value (xform->manager, xform->token, xform->order,
	                                           xmms_xform_shortname (xform), key);
}

gboolean
xmms_xform_metadata_get_int (xmms_xform_t *xform, const char *key, gint32 *val)
{
	gboolean result = FALSE;
	int64_t v;

	result = xmms_xform_token_manager_get_int (xform->manager, xform->token, xform->order,
	                                           xmms_xform_shortname (xform), key, &v);
	if (result) {
		*val = MAX (MIN (v, G_MAXINT32), G_MININT32);
	}

	return result;
}

gboolean
xmms_xform_metadata_get_str (xmms_xform_t *xform, const char *key,
                             gchar **val)
{
	return xmms_xform_token_manager_get_string (xform->manager, xform->token, xform->order,
	                                            xmms_xform_shortname (xform), key, val);
}

static void
xmms_xform_auxdata_set_val (xmms_xform_t *xform, char *key, xmmsv_t *val)
{
	xmms_xform_hotspot_t *hs;

	hs = g_new0 (xmms_xform_hotspot_t, 1);
	hs->pos = xform->buffered;
	hs->key = key;
	hs->obj = val;

	g_queue_push_tail (xform->hotspots, hs);
}

void
xmms_xform_auxdata_barrier (xmms_xform_t *xform)
{
	xmmsv_t *val = xmmsv_new_none ();
	xmms_xform_auxdata_set_val (xform, NULL, val);
}

void
xmms_xform_auxdata_set_int (xmms_xform_t *xform, const char *key, int intval)
{
	xmmsv_t *val = xmmsv_new_int (intval);
	xmms_xform_auxdata_set_val (xform, g_strdup (key), val);
}

void
xmms_xform_auxdata_set_str (xmms_xform_t *xform, const gchar *key,
                            const gchar *strval)
{
	xmmsv_t *val;
	const char *old;

	if (xmms_xform_auxdata_get_str (xform, key, &old)) {
		if (strcmp (old, strval) == 0) {
			return;
		}
	}

	val = xmmsv_new_string (strval);
	xmms_xform_auxdata_set_val (xform, g_strdup (key), val);
}

void
xmms_xform_auxdata_set_bin (xmms_xform_t *xform, const gchar *key,
                            gpointer data, gssize len)
{
	xmmsv_t *val;

	val = xmmsv_new_bin (data, len);
	xmms_xform_auxdata_set_val (xform, g_strdup (key), val);
}

static const xmmsv_t *
xmms_xform_auxdata_get_val (xmms_xform_t *xform, const gchar *key)
{
	guint i;
	xmms_xform_hotspot_t *hs;
	xmmsv_t *val = NULL;

	/* privdata is always got from the previous xform */
	xform = xform->prev;

	/* check if we have unhandled current (pos 0) hotspots for this key */
	for (i=0; (hs = g_queue_peek_nth (xform->hotspots, i)) != NULL; i++) {
		if (hs->pos != 0) {
			break;
		} else if (hs->key && !strcmp (key, hs->key)) {
			val = hs->obj;
		}
	}

	if (!val) {
		val = g_hash_table_lookup (xform->privdata, key);
	}

	return val;
}

gboolean
xmms_xform_auxdata_has_val (xmms_xform_t *xform, const gchar *key)
{
	return !!xmms_xform_auxdata_get_val (xform, key);
}

gboolean
xmms_xform_auxdata_get_int (xmms_xform_t *xform, const gchar *key, gint32 *val)
{
	const xmmsv_t *obj;

	obj = xmms_xform_auxdata_get_val (xform, key);
	if (obj && xmmsv_get_type (obj) == XMMSV_TYPE_INT32) {
		xmmsv_get_int (obj, val);
		return TRUE;
	}

	return FALSE;
}

gboolean
xmms_xform_auxdata_get_str (xmms_xform_t *xform, const gchar *key,
                            const gchar **val)
{
	const xmmsv_t *obj;

	obj = xmms_xform_auxdata_get_val (xform, key);
	if (obj && xmmsv_get_type (obj) == XMMSV_TYPE_STRING) {
		xmmsv_get_string (obj, val);
		return TRUE;
	}

	return FALSE;
}

gboolean
xmms_xform_auxdata_get_bin (xmms_xform_t *xform, const gchar *key,
                            const guchar **data, gsize *datalen)
{
	const xmmsv_t *obj;

	obj = xmms_xform_auxdata_get_val (xform, key);
	if (obj && xmmsv_get_type (obj) == XMMSV_TYPE_BIN) {
		xmmsv_get_bin (obj, data, (guint *) datalen);
		return TRUE;
	}

	return FALSE;
}

const char *
xmms_xform_shortname (xmms_xform_t *xform)
{
	return (xform->plugin)
	       ? xmms_plugin_shortname_get ((xmms_plugin_t *) xform->plugin)
	       : "unknown";
}

static gint
xmms_xform_this_peek (xmms_xform_t *xform, gpointer buf, gint siz,
                      xmms_error_t *err)
{
	while (xform->buffered < siz) {
		gint res;

		if (xform->buffered + READ_CHUNK > xform->buffersize) {
			xform->buffersize *= 2;
			xform->buffer = g_realloc (xform->buffer, xform->buffersize);
		}

		res = xmms_xform_plugin_read (xform->plugin, xform,
		                              &xform->buffer[xform->buffered],
		                              READ_CHUNK, err);

		if (res < -1) {
			XMMS_DBG ("Read method of %s returned bad value (%d) - BUG IN PLUGIN",
			          xmms_xform_shortname (xform), res);
			res = -1;
		}

		if (res == 0) {
			xform->eos = TRUE;
			break;
		} else if (res == -1) {
			xform->error = TRUE;
			return -1;
		} else {
			xform->buffered += res;
		}
	}

	/* might have eosed */
	siz = MIN (siz, xform->buffered);
	memcpy (buf, xform->buffer, siz);
	return siz;
}

static void
xmms_xform_hotspot_callback (gpointer data, gpointer user_data)
{
	xmms_xform_hotspot_t *hs = data;
	gint *read = user_data;

	hs->pos -= *read;
}

static gint
xmms_xform_hotspots_update (xmms_xform_t *xform)
{
	xmms_xform_hotspot_t *hs;
	gint ret = -1;

	hs = g_queue_peek_head (xform->hotspots);
	while (hs != NULL && hs->pos == 0) {
		g_queue_pop_head (xform->hotspots);
		if (hs->key) {
			g_hash_table_insert (xform->privdata, hs->key, hs->obj);
		}
		hs = g_queue_peek_head (xform->hotspots);
	}

	if (hs != NULL) {
		ret = hs->pos;
	}

	return ret;
}

gint
xmms_xform_this_read (xmms_xform_t *xform, gpointer buf, gint siz,
                      xmms_xform_token_t *token, xmms_error_t *err)
{
	gint read = 0;
	gint nexths;

	if (xform->error) {
		xmms_error_set (err, XMMS_ERROR_GENERIC, "Read on errored xform");
		return -1;
	}

	/* update hotspots */
	nexths = xmms_xform_hotspots_update (xform);
	if (nexths >= 0) {
		siz = MIN (siz, nexths);
	}

	if (xform->buffered) {
		read = MIN (siz, xform->buffered);
		memcpy (buf, xform->buffer, read);
		xform->buffered -= read;

		/* buffer edited, update hotspot positions */
		g_queue_foreach (xform->hotspots, &xmms_xform_hotspot_callback, &read);

		if (xform->buffered) {
			/* unless we are _peek:ing often
			   this should be fine */
			memmove (xform->buffer, &xform->buffer[read], xform->buffered);
		}
	}

	if (xform->eos) {
		return read;
	}

	while (read < siz) {
		gint res;

		res = xmms_xform_plugin_read (xform->plugin, xform, buf + read, siz - read, err);
		if (res < -1) {
			XMMS_DBG ("Read method of %s returned bad value (%d) - BUG IN PLUGIN", xmms_xform_shortname (xform), res);
			res = -1;
		}

		if (xform->prev->token > xform->token)
			xform->token = xform->prev->token;

		if (res == 0) {
			xform->eos = TRUE;
			break;
		} else if (res == -1) {
			xform->error = TRUE;
			return -1;
		} else {
			if (read == 0)
				xmms_xform_hotspots_update (xform);

			if (!g_queue_is_empty (xform->hotspots)) {
				if (xform->buffered + res > xform->buffersize) {
					xform->buffersize = MAX (xform->buffersize * 2,
					                         xform->buffersize + res);
					xform->buffer = g_realloc (xform->buffer,
					                           xform->buffersize);
				}

				g_memmove (xform->buffer + xform->buffered, buf + read, res);
				xform->buffered += res;
				break;
			}
			read += res;
		}
	}

	*token = xform->token;

	return read;
}

gint64
xmms_xform_this_seek (xmms_xform_t *xform, gint64 offset,
                      xmms_xform_seek_mode_t whence, xmms_error_t *err)
{
	gint64 res;

	if (xform->error) {
		xmms_error_set (err, XMMS_ERROR_GENERIC, "Seek on errored xform");
		return -1;
	}

	if (!xmms_xform_plugin_can_seek (xform->plugin)) {
		XMMS_DBG ("Seek not implemented in '%s'", xmms_xform_shortname (xform));
		xmms_error_set (err, XMMS_ERROR_GENERIC, "Seek not implemented");
		return -1;
	}

	if (xform->buffered && whence == XMMS_XFORM_SEEK_CUR) {
		offset -= xform->buffered;
	}

	res = xmms_xform_plugin_seek (xform->plugin, xform, offset, whence, err);
	if (res != -1) {
		xmms_xform_hotspot_t *hs;

		xform->eos = FALSE;
		xform->buffered = 0;

		/* flush the hotspot queue on seek */
		while ((hs = g_queue_pop_head (xform->hotspots)) != NULL) {
			g_free (hs->key);
			xmmsv_unref (hs->obj);
			g_free (hs);
		}
	}

	return res;
}

gint
xmms_xform_peek (xmms_xform_t *xform, gpointer buf, gint siz,
                 xmms_error_t *err)
{
	g_return_val_if_fail (xform->prev, -1);
	return xmms_xform_this_peek (xform->prev, buf, siz, err);
}

gchar *
xmms_xform_read_line (xmms_xform_t *xform, gchar *line, xmms_error_t *err)
{
	gchar *p;

	g_return_val_if_fail (xform, NULL);
	g_return_val_if_fail (line, NULL);

	p = strchr (xform->lr.buf, '\n');

	if (!p) {
		gint l, r;

		l = (XMMS_XFORM_MAX_LINE_SIZE - 1) - (xform->lr.bufend - xform->lr.buf);
		if (l) {
			r = xmms_xform_read (xform, xform->lr.bufend, l, err);
			if (r < 0) {
				return NULL;
			}
			xform->lr.bufend += r;
		}
		if (xform->lr.bufend <= xform->lr.buf)
			return NULL;

		*(xform->lr.bufend) = '\0';
		p = strchr (xform->lr.buf, '\n');
		if (!p) {
			p = xform->lr.bufend;
		}
	}

	if (p > xform->lr.buf && *(p-1) == '\r') {
		*(p-1) = '\0';
	} else {
		*p = '\0';
	}

	strcpy (line, xform->lr.buf);
	memmove (xform->lr.buf, p + 1, xform->lr.bufend - p);
	xform->lr.bufend -= (p - xform->lr.buf) + 1;
	*xform->lr.bufend = '\0';

	return line;
}

gint
xmms_xform_read (xmms_xform_t *xform, gpointer buf, gint siz, xmms_error_t *err)
{
	g_return_val_if_fail (xform->prev, -1);
	return xmms_xform_this_read (xform->prev, buf, siz, &xform->token, err);
}

gint64
xmms_xform_seek (xmms_xform_t *xform, gint64 offset,
                 xmms_xform_seek_mode_t whence, xmms_error_t *err)
{
	g_return_val_if_fail (xform->prev, -1);
	return xmms_xform_this_seek (xform->prev, offset, whence, err);
}

const gchar *
xmms_xform_get_url (xmms_xform_t *xform)
{
	const gchar *url = NULL;
	xmms_xform_t *x;
	x = xform;

	while (!url && x) {
		url = xmms_xform_indata_get_str (x, XMMS_STREAM_TYPE_URL);
		x = x->prev;
	}

	return url;
}


typedef struct match_state_St {
	xmms_xform_plugin_t *match;
	xmms_stream_type_t *out_type;
	gint priority;
} match_state_t;

static gboolean
xmms_xform_match (xmms_plugin_t *plugin, gpointer user_data)
{
	xmms_xform_plugin_t *xform_plugin = (xmms_xform_plugin_t *) plugin;
	match_state_t *state = (match_state_t *) user_data;
	gint priority = 0;

	g_assert (plugin->type == XMMS_PLUGIN_TYPE_XFORM);

	XMMS_DBG ("Trying plugin '%s'", xmms_plugin_shortname_get (plugin));
	if (!xmms_xform_plugin_supports (xform_plugin, state->out_type, &priority)) {
		return TRUE;
	}

	XMMS_DBG ("Plugin '%s' matched (priority %d)",
	          xmms_plugin_shortname_get (plugin), priority);

	if (priority > state->priority) {
		if (state->match) {
			xmms_plugin_t *previous_plugin = (xmms_plugin_t *) state->match;
			XMMS_DBG ("Using plugin '%s' (priority %d) instead of '%s' (priority %d)",
			          xmms_plugin_shortname_get (plugin), priority,
			          xmms_plugin_shortname_get (previous_plugin),
			          state->priority);
		}

		state->match = xform_plugin;
		state->priority = priority;
	}

	return TRUE;
}

xmms_xform_t *
xmms_xform_find (xmms_xform_t *prev, xmms_xform_token_manager_t *manager,
                 GPtrArray *stream_type_goals)
{
	match_state_t state;
	xmms_xform_t *xform = NULL;

	state.out_type = prev->out_type;
	state.match = NULL;
	state.priority = -1;

	xmms_plugin_foreach (XMMS_PLUGIN_TYPE_XFORM, xmms_xform_match, &state);

	if (state.match) {
		xform = xmms_xform_new (state.match, prev, manager, stream_type_goals);
	} else {
		XMMS_DBG ("Found no matching plugin...");
	}

	return xform;
}

gboolean
xmms_xform_iseos (xmms_xform_t *xform)
{
	gboolean ret = TRUE;

	if (xform->prev) {
		ret = xform->prev->eos;
	}

	return ret;
}

const xmms_stream_type_t *
xmms_xform_get_out_stream_type (xmms_xform_t *xform)
{
	return xform->out_type;
}

GPtrArray *
xmms_xform_stream_type_goals (xmms_xform_t *xform)
{
	return g_ptr_array_ref (xform->stream_type_goals);
}

xmms_config_property_t *
xmms_xform_config_lookup (xmms_xform_t *xform, const gchar *path)
{
	g_return_val_if_fail (xform->plugin, NULL);

	return xmms_plugin_config_lookup ((xmms_plugin_t *) xform->plugin, path);
}
