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
 * xforms chain
 */

#include <xmms/xmms_log.h>
#include <xmmspriv/xmms_plugin.h>
#include <xmmspriv/xmms_xform.h>
#include <xmmspriv/xmms_xform_plugin.h>


static xmms_xform_t *add_effects (xmms_xform_t *last, xmms_medialib_t *medialib,
                                  xmms_medialib_entry_t entry,
                                  GPtrArray *stream_type_goals);
static xmms_xform_t *xmms_xform_new_effect (xmms_xform_t* last, xmms_medialib_t *medialib,
                                            xmms_medialib_entry_t entry,
                                            GPtrArray *stream_type_goals,
                                            const gchar *name);

static gboolean
has_goalformat (xmms_xform_t *xform, GPtrArray *stream_type_goals)
{
	const xmms_stream_type_t *current;
	gboolean ret = FALSE;
	gint i;

	current = xmms_xform_get_out_stream_type (xform);

	for (i = 0; i < stream_type_goals->len; i++) {
		xmms_stream_type_t *stream_type;

		stream_type = g_ptr_array_index (stream_type_goals, i);
		if (xmms_stream_type_match (stream_type, current)) {
			ret = TRUE;
			break;
		}

	}

	if (!ret) {
		XMMS_DBG ("Not in one of %d goal-types", g_list_length (stream_type_goals));
	}

	return ret;
}

static void
outdata_type_metadata_collect (xmms_xform_t *xform)
{
	gint val;
	const char *mime;
	xmms_stream_type_t *type;

	type = xmms_xform_outtype_get (xform);
	mime = xmms_stream_type_get_str (type, XMMS_STREAM_TYPE_MIMETYPE);
	if (strcmp (mime, "audio/pcm") != 0) {
		return;
	}

	val = xmms_stream_type_get_int (type, XMMS_STREAM_TYPE_FMT_FORMAT);
	if (val != -1) {
		const gchar *name = xmms_sample_name_get ((xmms_sample_format_t) val);
		xmms_xform_metadata_set_str (xform,
		                             XMMS_MEDIALIB_ENTRY_PROPERTY_SAMPLE_FMT,
		                             name);
	}

	val = xmms_stream_type_get_int (type, XMMS_STREAM_TYPE_FMT_SAMPLERATE);
	if (val != -1) {
		xmms_xform_metadata_set_int (xform,
		                             XMMS_MEDIALIB_ENTRY_PROPERTY_SAMPLERATE,
		                             val);
	}

	val = xmms_stream_type_get_int (type, XMMS_STREAM_TYPE_FMT_CHANNELS);
	if (val != -1) {
		xmms_xform_metadata_set_int (xform,
		                             XMMS_MEDIALIB_ENTRY_PROPERTY_CHANNELS,
		                             val);
	}
}

static xmms_xform_t *
chain_setup (xmms_medialib_t *medialib, xmms_medialib_entry_t entry,
             const gchar *url, GPtrArray *stream_type_goals)
{
	xmms_xform_t *xform, *last;
	gchar *durl, *args;

	if (!entry) {
		entry = 1; /* FIXME: this is soooo ugly, don't do this */
	}

	xform = xmms_xform_new (NULL, NULL, medialib , 0, stream_type_goals);

	durl = g_strdup (url);

	args = strchr (durl, '?');
	if (args) {
		gchar **params;
		gint i;
		*args = 0;
		args++;
		xmms_medialib_decode_url (args);

		params = g_strsplit (args, "&", 0);

		for (i = 0; params && params[i]; i++) {
			gchar *v;
			v = strchr (params[i], '=');
			if (v) {
				*v = 0;
				v++;
				xmms_xform_metadata_set_str (xform, params[i], v);
			} else {
				xmms_xform_metadata_set_int (xform, params[i], 1);
			}
		}
		g_strfreev (params);
	}
	xmms_medialib_decode_url (durl);

	xmms_xform_outdata_type_add (xform, XMMS_STREAM_TYPE_MIMETYPE,
	                             "application/x-url", XMMS_STREAM_TYPE_URL,
	                             durl, XMMS_STREAM_TYPE_END);

	g_free (durl);

	last = xform;

	do {
		xform = xmms_xform_find (last, entry, stream_type_goals);
		if (!xform) {
			xmms_log_error ("Couldn't set up chain for '%s' (%d)",
			                url, entry);
			xmms_object_unref (last);

			return NULL;
		}
		xmms_object_unref (last);
		last = xform;
	} while (!has_goalformat (xform, stream_type_goals));

	outdata_type_metadata_collect (last);

	return last;
}

static void
chain_finalize (xmms_medialib_session_t *session,
                xmms_xform_t *xform, xmms_medialib_entry_t entry,
                const gchar *url, gboolean rehashing)
{
	GString *namestr;

	namestr = g_string_new ("");
	xmms_xform_metadata_collect (session, xform, namestr, rehashing);
	xmms_log_info ("Successfully setup chain for '%s' (%d) containing %s",
	               url, entry, namestr->str);

	g_string_free (namestr, TRUE);
}

static gchar *
get_url_for_entry (xmms_medialib_session_t *session, xmms_medialib_entry_t entry)
{
	gchar *url = NULL;

	url = xmms_medialib_entry_property_get_str (session, entry, XMMS_MEDIALIB_ENTRY_PROPERTY_URL);

	if (!url) {
		xmms_log_error ("Couldn't get url for entry (%d)", entry);
	}

	return url;
}

xmms_xform_t *
xmms_xform_chain_setup (xmms_medialib_t *medialib, xmms_medialib_entry_t entry,
                        GPtrArray *stream_type_goals, gboolean rehash)
{
	xmms_medialib_session_t *session;
	xmms_xform_t *ret = NULL;

	do {
		session = xmms_medialib_session_begin (medialib);
		if (ret != NULL)
			xmms_object_unref (ret);
		ret = xmms_xform_chain_setup_session (medialib, session, entry, stream_type_goals, rehash);
	} while (!xmms_medialib_session_commit (session));

	return ret;
}

xmms_xform_t *
xmms_xform_chain_setup_session (xmms_medialib_t *medialib,
                                xmms_medialib_session_t *session,
                                xmms_medialib_entry_t entry,
                                GPtrArray *stream_type_goals, gboolean rehash)
{
	gchar *url;
	xmms_xform_t *xform;

	if (!(url = get_url_for_entry (session, entry))) {
		return NULL;
	}

	xform = xmms_xform_chain_setup_url_session (medialib, session, entry,
	                                            url, stream_type_goals, rehash);
	g_free (url);

	return xform;
}

xmms_xform_t *
xmms_xform_chain_setup_url_session (xmms_medialib_t *medialib,
                                    xmms_medialib_session_t *session,
                                    xmms_medialib_entry_t entry, const gchar *url,
                                    GPtrArray *stream_type_goals, gboolean rehash)
{
	xmms_xform_t *last;
	xmms_plugin_t *plugin;
	xmms_xform_plugin_t *xform_plugin;
	gboolean add_segment = FALSE;
	gint priority;

	last = chain_setup (medialib, entry, url, stream_type_goals);
	if (!last) {
		return NULL;
	}

	/* first check that segment plugin is available in the system */
	plugin = xmms_plugin_find (XMMS_PLUGIN_TYPE_XFORM, "segment");
	xform_plugin = (xmms_xform_plugin_t *) plugin;

	/* if segment plugin input is the same as current output, include it
	 * for collecting additional duration metadata on audio entries */
	if (xform_plugin) {
		add_segment = xmms_xform_plugin_supports (xform_plugin,
		                                          xmms_xform_outtype_get (last),
		                                          &priority);
		xmms_object_unref (plugin);
	}

	/* add segment plugin to the chain if it can be added */
	if (add_segment) {
		last = xmms_xform_new_effect (last, medialib, entry, stream_type_goals, "segment");
		if (!last) {
			return NULL;
		}
	}

	/* if not rehashing, also initialize all the effect plugins */
	if (!rehash) {
		last = add_effects (last, medialib, entry, stream_type_goals);
		if (!last) {
			return NULL;
		}
	}

	chain_finalize (session, last, entry, url, rehash);
	return last;
}

xmms_xform_t *
xmms_xform_chain_setup_url (xmms_medialib_t *medialib,
                            xmms_medialib_entry_t entry, const gchar *url,
                            GPtrArray *stream_type_goals, gboolean rehash)
{
	xmms_medialib_session_t *session;
	xmms_xform_t *ret = NULL;

	do {
		session = xmms_medialib_session_begin (medialib);
		if (ret != NULL)
			xmms_object_unref (ret);
		ret = xmms_xform_chain_setup_url_session (medialib, session, entry, url,
		                                          stream_type_goals, rehash);
	} while (!xmms_medialib_session_commit (session));

	return ret;
}

static xmms_xform_t *
add_effects (xmms_xform_t *last, xmms_medialib_t *medialib,
             xmms_medialib_entry_t entry, GPtrArray *stream_type_goals)
{
	gint effect_no;

	for (effect_no = 0; TRUE; effect_no++) {
		xmms_config_property_t *cfg;
		gchar key[64];
		const gchar *name;

		g_snprintf (key, sizeof (key), "effect.order.%i", effect_no);

		cfg = xmms_config_lookup (key);
		if (!cfg) {
			break;
		}

		name = xmms_config_property_get_string (cfg);

		if (!name[0]) {
			continue;
		}

		last = xmms_xform_new_effect (last, medialib, entry, stream_type_goals, name);
	}

	return last;
}

static xmms_xform_t *
xmms_xform_new_effect (xmms_xform_t *last, xmms_medialib_t *medialib,
                       xmms_medialib_entry_t entry, GPtrArray *stream_type_goals,
                       const gchar *name)
{
	xmms_plugin_t *plugin;
	xmms_xform_plugin_t *xform_plugin;
	xmms_xform_t *xform;
	gint priority;

	plugin = xmms_plugin_find (XMMS_PLUGIN_TYPE_XFORM, name);
	if (!plugin) {
		xmms_log_error ("Couldn't find any effect named '%s'", name);
		return last;
	}

	xform_plugin = (xmms_xform_plugin_t *) plugin;
	if (!xmms_xform_plugin_supports (xform_plugin, xmms_xform_outtype_get (last), &priority)) {
		xmms_log_info ("Effect '%s' doesn't support format, skipping",
		               xmms_plugin_shortname_get (plugin));
		xmms_object_unref (plugin);
		return last;
	}

	xform = xmms_xform_new (xform_plugin, last, medialib, entry, stream_type_goals);

	if (xform) {
		xmms_object_unref (last);
		last = xform;
	} else {
		xmms_log_info ("Effect '%s' failed to initialize, skipping",
		               xmms_plugin_shortname_get (plugin));
	}
	xmms_xform_plugin_config_property_register (xform_plugin,
	                                            "enabled", "0",
	                                            NULL, NULL);
	xmms_object_unref (plugin);
	return last;
}
