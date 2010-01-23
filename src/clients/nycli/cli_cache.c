/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2007 XMMS2 Team
 *
 *  PLUGINS ARE NOT CONSIDERED TO BE DERIVED WORK !!!
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 */

#include "cli_cache.h"
#include "cli_infos.h"


static void
freshness_init (freshness_t *fresh)
{
	fresh->status = CLI_CACHE_NOT_INIT;
	fresh->pending_queries = 0;
}

static gboolean
freshness_is_fresh (freshness_t *fresh)
{
	return fresh->status == CLI_CACHE_FRESH;
}

static void
freshness_requested (freshness_t *fresh)
{
	fresh->status = CLI_CACHE_PENDING;
	fresh->pending_queries++;
}

static void
freshness_received (freshness_t *fresh)
{
	if (fresh->status == CLI_CACHE_NOT_INIT) {
		fresh->status = CLI_CACHE_FRESH;
		fresh->pending_queries = 0;
	} else if (fresh->status == CLI_CACHE_PENDING) {
		if (fresh->pending_queries > 0) {
			fresh->pending_queries--;
		}
		if (fresh->pending_queries == 0) {
			fresh->status = CLI_CACHE_FRESH;
		}
	}
}

/** Get or create a playlist */
static playlist_t *
playlist_get (cli_cache_t *cache, const gchar *name)
{
	playlist_t *pls;

	if (strcmp (name, XMMS_ACTIVE_PLAYLIST) == 0) {
		name = cache->active_playlist_name;
	}

	pls = (playlist_t *) g_hash_table_lookup (cache->playlists, name);
	if (pls != NULL) {
		return pls;
	}

	pls = g_new0 (playlist_t, 1);
	pls->content = g_array_new (FALSE, TRUE, sizeof (gint32));

	freshness_init (&pls->freshness_content);
	freshness_init (&pls->freshness_position);

	g_hash_table_insert (cache->playlists, g_strdup (name), pls);

	return pls;
}

static void
playlist_update_position (cli_cache_t *cache, const gchar *name, gint position)
{
	playlist_t *pls = playlist_get (cache, name);
	pls->position = position;

	freshness_received (&pls->freshness_position);
}


static void
playlist_entry_add (cli_cache_t *cache, const gchar *name, gint id)
{
	playlist_t *playlist = playlist_get (cache, name);
	g_array_append_val (playlist->content, id);
}

static void
playlist_entry_insert (cli_cache_t *cache, const gchar *name, gint position, gint id)
{
	playlist_t *playlist = playlist_get (cache, name);
	g_array_insert_val (playlist->content, position, id);
}

static void
playlist_entry_move (cli_cache_t *cache, const gchar *name, gint oldpos, gint newpos, gint id)
{
	playlist_t *playlist = playlist_get (cache, name);
	g_array_remove_index (playlist->content, oldpos);
	g_array_insert_val (playlist->content, newpos, id);
}

static void
playlist_entry_remove (cli_cache_t *cache, const gchar *name, gint pos)
{
	playlist_t *playlist = playlist_get (cache, name);
	g_array_remove_index (playlist->content, pos);
}

static void
playlist_rename (cli_cache_t *cache, const gchar *name, const gchar *newname)
{
	gpointer key, value;

	if (g_hash_table_lookup_extended (cache->playlists, name, &key, &value)) {
		g_hash_table_steal (cache->playlists, name);
		g_free (value);

		g_hash_table_insert (cache->playlists, g_strdup (name), value);
	}
}

static void
playlist_remove (cli_cache_t *cache, const gchar *name)
{
	g_hash_table_remove (cache->playlists, name);
}

static void
playlist_clear (playlist_t *playlist)
{
	if (playlist->content->len == 0) {
		return;
	}
	
	playlist->content = g_array_remove_range (playlist->content, 0,
	                                          playlist->content->len);
}


static gint
playlist_refresh_content (xmmsv_t *val, void *udata)
{
	playlist_t *playlist = (playlist_t *) udata;
	xmmsv_list_iter_t *it;
	gint32 id;

	if (xmmsv_is_error (val)) {
		return TRUE;
	}

	playlist_clear (playlist);

	xmmsv_get_list_iter (val, &it);

	/* .. and refill it */
	while (xmmsv_list_iter_valid (it)) {
		xmmsv_t *entry;
		xmmsv_list_iter_entry (it, &entry);
		xmmsv_get_int (entry, &id);
		g_array_append_val (playlist->content, id);

		xmmsv_list_iter_next (it);
	}

	freshness_received (&playlist->freshness_content);

	return TRUE;
}

static gint
playlist_refresh_position (xmmsv_t *val, void *udata)
{
	cli_cache_t *cache = (cli_cache_t *) udata;
	const gchar *playlist = NULL;
	gint position;

	if (xmmsv_is_error (val)) {
		return TRUE;
	}
	
	xmmsv_dict_entry_get_string (val, "name", &playlist);
	xmmsv_dict_entry_get_int (val, "position", &position);

	g_assert (playlist != NULL);
	
	playlist_update_position (cache, playlist, position);
	
	return TRUE;
}

static void
playlist_refresh_position_sync (cli_infos_t *infos, const gchar *name)
{
	playlist_t *playlist = playlist_get (infos->cache, name);
	xmmsc_result_t *res;
	xmmsv_t *value;

	freshness_requested (&playlist->freshness_content);

	res = xmmsc_playlist_current_pos (infos->conn, name);
	xmmsc_result_wait (res);

	value = xmmsc_result_get_value (res);
	playlist_refresh_position (value, infos->cache);

	xmmsc_result_unref (res);
}

static void
playlist_refresh_content_sync (cli_infos_t *infos, const gchar *name)
{
	playlist_t *playlist = playlist_get (infos->cache, name);
	xmmsc_result_t *res;
	xmmsv_t *value;

	freshness_requested (&playlist->freshness_content);

	res = xmmsc_playlist_list_entries (infos->sync, name);
	xmmsc_result_wait (res);

	value = xmmsc_result_get_value (res);
	playlist_refresh_content (value, playlist);

	xmmsc_result_unref (res);
}


static gint
refresh_currid (xmmsv_t *val, void *udata)
{
	cli_cache_t *cache = (cli_cache_t *) udata;

	if (!xmmsv_is_error (val)) {
		xmmsv_get_int (val, &cache->currid);
	}

	freshness_received (&cache->freshness_currid);

	return TRUE;
}

static gint
refresh_playback_status (xmmsv_t *val, void *udata)
{
	cli_cache_t *cache = (cli_cache_t *) udata;

	if (!xmmsv_is_error (val)) {
		xmmsv_get_int (val, &cache->playback_status);
	}

	freshness_received (&cache->freshness_playback_status);

	return TRUE;
}

static gint
refresh_active_playlist_name (xmmsv_t *val, void *udata)
{
	cli_cache_t *cache = (cli_cache_t *) udata;
	const gchar *buf;

	if (!xmmsv_is_error (val) && xmmsv_get_string (val, &buf)) {
		g_free (cache->active_playlist_name);
		cache->active_playlist_name = g_strdup (buf);
	}

	freshness_received (&cache->freshness_active_playlist_name);

	return TRUE;
}

static gint
playlist_content_changed (xmmsv_t *val, void *udata)
{
	cli_infos_t *infos = (cli_infos_t *) udata;
	cli_cache_t *cache = infos->cache;
	xmmsc_result_t *refres;
	gint pos, newpos, type;
	gint id;
	const gchar *name;

	xmmsv_dict_entry_get_int (val, "type", &type);
	xmmsv_dict_entry_get_int (val, "position", &pos);
	xmmsv_dict_entry_get_int (val, "id", &id);
	xmmsv_dict_entry_get_string (val, "name", &name);

	/* Apply changes to the cached playlist */
	switch (type) {
	case XMMS_PLAYLIST_CHANGED_ADD:
		playlist_entry_add (cache, name, id);
		break;

	case XMMS_PLAYLIST_CHANGED_INSERT:
		playlist_entry_insert (cache, name, pos, id);
		break;

	case XMMS_PLAYLIST_CHANGED_MOVE:
		xmmsv_dict_entry_get_int (val, "newposition", &newpos);
		playlist_entry_move (cache, name, pos, newpos, id);
		break;

	case XMMS_PLAYLIST_CHANGED_REMOVE:
		playlist_entry_remove (cache, name, pos);
		break;

	case XMMS_PLAYLIST_CHANGED_SHUFFLE:
	case XMMS_PLAYLIST_CHANGED_SORT:
	case XMMS_PLAYLIST_CHANGED_CLEAR:
		/* Oops, reload the whole playlist */
		refres = xmmsc_playlist_list_entries (infos->conn, name);
		playlist_t *playlist = playlist_get (infos->cache, name);
		xmmsc_result_notifier_set (refres, &playlist_refresh_content, playlist);
		xmmsc_result_unref (refres);
		freshness_requested (&playlist->freshness_content);
		break;
	}

	return TRUE;
}

static gint
playlist_loaded (xmmsv_t *val, void *udata)
{
	cli_infos_t *infos = (cli_infos_t *) udata;
	xmmsc_result_t *refres;
	const gchar *buf;

	/* Refresh playlist name */
	if (xmmsv_get_string (val, &buf)) {
		g_free (infos->cache->active_playlist_name);
		infos->cache->active_playlist_name = g_strdup (buf);
	}

	playlist_t *playlist = playlist_get (infos->cache, infos->cache->active_playlist_name);

	/* Get all the entries again */
	refres = xmmsc_playlist_list_entries (infos->conn, infos->cache->active_playlist_name);
	xmmsc_result_notifier_set (refres, &playlist_refresh_content, playlist);
	xmmsc_result_unref (refres);
	
	freshness_requested (&playlist->freshness_content);

	return TRUE;
}

static gint
playlist_changed (xmmsv_t *val, void *udata)
{
	cli_infos_t *infos = (cli_infos_t *) udata;
	const gchar *name, *newname;
	gint type;

	xmmsv_dict_entry_get_int (val, "type", &type);
	xmmsv_dict_entry_get_string (val, "name", &name);

	switch (type) {
	case XMMS_COLLECTION_CHANGED_RENAME:
		xmmsv_dict_entry_get_string (val, "newname", &newname);
		playlist_rename (infos->cache, name, newname);
		break;
	case XMMS_COLLECTION_CHANGED_REMOVE:
		playlist_remove (infos->cache, name);
		break;
	}

	return TRUE;
}

static void
free_playlist (gpointer data)
{
	playlist_t *playlist = (playlist_t *) data;
	g_array_free (playlist->content, TRUE);	
}

/** Initialize the cache, must still be started to be filled. */
cli_cache_t *
cli_cache_init ()
{
	cli_cache_t *cache;

	cache = g_new0 (cli_cache_t, 1);
	cache->currid = 0;
	cache->playback_status = 0;
	cache->active_playlist_name = NULL;
	cache->playlists = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                          g_free, free_playlist);

	/* Init the freshness state */
	freshness_init (&cache->freshness_currid);
	freshness_init (&cache->freshness_playback_status);
	freshness_init (&cache->freshness_active_playlist_name);

	return cache;
}

void
cli_cache_refresh (cli_infos_t *infos)
{
	xmmsc_result_t *res;
	
	res = xmmsc_playlist_current_active (infos->conn);
	xmmsc_result_wait (res);
	refresh_active_playlist_name (xmmsc_result_get_value (res), infos->cache);
	xmmsc_result_unref (res);

	res = xmmsc_playback_current_id (infos->conn);
	xmmsc_result_wait (res);
	refresh_currid (xmmsc_result_get_value (res), infos->cache);
	xmmsc_result_unref (res);

	res = xmmsc_playback_status (infos->conn);
	xmmsc_result_wait (res);
	refresh_playback_status (xmmsc_result_get_value (res), infos->cache);
	xmmsc_result_unref (res);

	playlist_refresh_position_sync (infos, infos->cache->active_playlist_name);
	playlist_refresh_content_sync (infos, infos->cache->active_playlist_name);
}

/** Fill the cache with initial (current) data, setup listeners. */
void
cli_cache_start (cli_infos_t *infos)
{
	xmmsc_result_t *res;

	/* Setup one-time value fetchers, for init */
	cli_cache_refresh (infos);

	/* Setup async listeners */
	res = xmmsc_broadcast_playlist_current_pos (infos->conn);
	xmmsc_result_notifier_set (res, &playlist_refresh_position, infos->cache);
	xmmsc_result_unref (res);

	res = xmmsc_broadcast_playback_current_id (infos->conn);
	xmmsc_result_notifier_set (res, &refresh_currid, infos->cache);
	xmmsc_result_unref (res);

	res = xmmsc_broadcast_playback_status (infos->conn);
	xmmsc_result_notifier_set (res, &refresh_playback_status, infos->cache);
	xmmsc_result_unref (res);

	res = xmmsc_broadcast_playlist_changed (infos->conn);
	xmmsc_result_notifier_set (res, &playlist_content_changed, infos);
	xmmsc_result_unref (res);

	res = xmmsc_broadcast_playlist_loaded (infos->conn);
	xmmsc_result_notifier_set (res, &playlist_loaded, infos);
	xmmsc_result_unref (res);

	res = xmmsc_broadcast_collection_changed (infos->conn);
	xmmsc_result_notifier_set (res, &playlist_changed, infos);
	xmmsc_result_unref (res);
}

/** Check whether the cache is currently fresh (up-to-date). */
gboolean
cli_cache_is_fresh (cli_cache_t *cache)
{
	/* Check if all items are fresh */
	return freshness_is_fresh (&cache->freshness_currid) &&
	       freshness_is_fresh (&cache->freshness_playback_status) &&
	       freshness_is_fresh (&cache->freshness_active_playlist_name);
}

/** Get the position of a playlist */
gint32
cli_cache_playlist_position (cli_infos_t *infos, const gchar *name)
{
	playlist_t *playlist = playlist_get (infos->cache, name);

	if (!freshness_is_fresh (&playlist->freshness_position)) {
		playlist_refresh_position_sync (infos, name);
	}

	return playlist->position;
}

/** Get the length of a playlist */
gint32
cli_cache_playlist_length (cli_infos_t *infos, const gchar *name)
{
	playlist_t *playlist = playlist_get (infos->cache, name);

	if (!freshness_is_fresh (&playlist->freshness_content)) {
		playlist_refresh_content_sync (infos, name);
	}

	return playlist->content->len;
}

/** Get the id of a position in a playlist */
gint32
cli_cache_playlist_position_id (cli_infos_t *infos, const gchar *name, gint32 position)
{
	playlist_t *playlist = playlist_get (infos->cache, name);

	if (!freshness_is_fresh (&playlist->freshness_content)) {
		playlist_refresh_content_sync (infos, name);
	}

	return g_array_index (playlist->content, guint, position);
}

/** Free all memory owned by the cache. */
void
cli_cache_free (cli_cache_t *cache)
{
	g_free (cache->active_playlist_name);
	g_hash_table_unref (cache->playlists);
	g_free (cache);
}
