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


static xmms_xform_t *add_effects (xmms_xform_t *last, xmms_xform_token_manager_t *manager,
                                  GPtrArray *stream_type_goals);
static xmms_xform_t *xmms_xform_new_effect (xmms_xform_t* last, xmms_xform_token_manager_t *manager,
                                            GPtrArray *stream_type_goals, const gchar *name);

/*
void
xmms_xform_metadata_collect (xmms_medialib_session_t *session,
                             xmms_xform_t *start, GString *namestr,
                             gboolean rehashing)
{
	metadata_festate_t info;
	gint times_played;
	gint last_started;
	GTimeVal now;

	info.entry = start->entry;

	info.session = session;
	times_played = xmms_medialib_entry_property_get_int (session, info.entry,
	                                                     XMMS_MEDIALIB_ENTRY_PROPERTY_TIMESPLAYED);

	// times_played == -1 if we haven't played this entry yet. so after initial
	//  metadata collection the mlib would have timesplayed = -1 if we didn't do
	// the following
	if (times_played < 0) {
		times_played = 0;
	}

	last_started = xmms_medialib_entry_property_get_int (session, info.entry,
	                                                     XMMS_MEDIALIB_ENTRY_PROPERTY_LASTSTARTED);

	xmms_medialib_entry_cleanup (session, info.entry);

	xmms_xform_metadata_collect_r (start, &info, namestr);

	xmms_medialib_entry_property_set_str (session, info.entry,
	                                      XMMS_MEDIALIB_ENTRY_PROPERTY_CHAIN,
	                                      namestr->str);

	xmms_medialib_entry_property_set_int (session, info.entry,
	                                      XMMS_MEDIALIB_ENTRY_PROPERTY_TIMESPLAYED,
	                                      times_played + (rehashing ? 0 : 1));

	if (!rehashing || (rehashing && last_started)) {
		g_get_current_time (&now);

		xmms_medialib_entry_property_set_int (session, info.entry,
		                                      XMMS_MEDIALIB_ENTRY_PROPERTY_LASTSTARTED,
		                                      (rehashing ? last_started : now.tv_sec));
	}

	xmms_medialib_entry_status_set (session, info.entry,
	                                XMMS_MEDIALIB_ENTRY_STATUS_OK);
}
*/

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
		XMMS_DBG ("Not in one of %d goal-types", stream_type_goals->len);
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
chain_setup (xmms_xform_token_manager_t *manager, const gchar *url,
             GPtrArray *stream_type_goals)
{
	xmms_xform_t *xform, *last;
	gchar *durl, *args;

	xform = xmms_xform_new (NULL, NULL, manager, stream_type_goals);

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
		xform = xmms_xform_find (last, manager, stream_type_goals);
		if (!xform) {
			xmms_log_error ("Couldn't set up chain for '%s'", url);
			xmms_object_unref (last);

			return NULL;
		}
		xmms_object_unref (last);
		last = xform;
	} while (!has_goalformat (xform, stream_type_goals));

	outdata_type_metadata_collect (last);

	return last;
}

/*
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
*/

xmms_xform_t *
xmms_xform_chain_new (xmms_xform_token_manager_t *manager,
                      GPtrArray *stream_type_goals)
{
	xmms_xform_t *last;
	xmms_plugin_t *plugin;
	xmms_xform_plugin_t *xform_plugin;
	gboolean add_segment = FALSE;
	gint priority;
	gchar *url;

	xmms_xform_token_manager_get_string (manager, "server", "url", &url);

	last = chain_setup (manager, url, stream_type_goals);
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
		last = xmms_xform_new_effect (last, manager, stream_type_goals, "segment");
		if (!last) {
			return NULL;
		}
	}

	//chain_finalize (session, last, entry, url, rehash);
	return last;
}

static xmms_xform_t *
add_effects (xmms_xform_t *last, xmms_xform_token_manager_t *manager,
             GPtrArray *stream_type_goals)
{
	gint effect_no;

	for (effect_no = 0; TRUE; effect_no++) {
		xmms_config_property_t *cfg;
		const gchar *name;
		gchar key[64];

		g_snprintf (key, sizeof (key), "effect.order.%i", effect_no);

		cfg = xmms_config_lookup (key);
		if (!cfg) {
			break;
		}

		name = xmms_config_property_get_string (cfg);
		if (!name[0]) {
			continue;
		}

		last = xmms_xform_new_effect (last, manager, stream_type_goals, name);
	}

	return last;
}

static xmms_xform_t *
xmms_xform_new_effect (xmms_xform_t *last, xmms_xform_token_manager_t *manager,
                       GPtrArray *stream_type_goals, const gchar *name)
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

	xform = xmms_xform_new (xform_plugin, last, manager, stream_type_goals);
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
