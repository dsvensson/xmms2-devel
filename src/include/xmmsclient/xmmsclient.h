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

#ifndef __XMMS_CLIENT_H__
#define __XMMS_CLIENT_H__

#include "xmmsc/xmmsc_stdint.h"
#include "xmmsc/xmmsc_ipc_msg.h"
#include "xmmsc/xmmsc_idnumbers.h"
#include "xmmsc/xmmsc_value.h"
#include "xmmsc/xmmsc_coll.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined (__GNUC__) && __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ > 1)
#define XMMS_DEPRECATED __attribute__((deprecated))
#else
#define XMMS_DEPRECATED
#endif

typedef struct xmmsc_connection_St xmmsc_connection_t;
typedef struct xmmsc_result_St xmmsc_result_t;

typedef enum {
	XMMSC_RESULT_CLASS_DEFAULT,
	XMMSC_RESULT_CLASS_SIGNAL,
	XMMSC_RESULT_CLASS_BROADCAST
} xmmsc_result_type_t;

typedef void (*xmmsc_disconnect_func_t) (void *user_data);
typedef void (*xmmsc_user_data_free_func_t) (void *user_data);
typedef void (*xmmsc_io_need_out_callback_func_t) (int, void*);

xmmsc_connection_t *xmmsc_init (const char *clientname);
int xmmsc_connect (xmmsc_connection_t *, const char *);
xmmsc_connection_t *xmmsc_ref (xmmsc_connection_t *c);
void xmmsc_unref (xmmsc_connection_t *c);
void xmmsc_lock_set (xmmsc_connection_t *conn, void *lock, void (*lockfunc)(void *), void (*unlockfunc)(void *));
void xmmsc_disconnect_callback_set (xmmsc_connection_t *c, xmmsc_disconnect_func_t disconnect_func, void *userdata);
void xmmsc_disconnect_callback_set_full (xmmsc_connection_t *c, xmmsc_disconnect_func_t disconnect_func, void *userdata, xmmsc_user_data_free_func_t free_func);

void xmmsc_io_need_out_callback_set (xmmsc_connection_t *c, xmmsc_io_need_out_callback_func_t callback, void *userdata);
void xmmsc_io_need_out_callback_set_full (xmmsc_connection_t *c, xmmsc_io_need_out_callback_func_t callback, void *userdata, xmmsc_user_data_free_func_t free_func);
void xmmsc_io_disconnect (xmmsc_connection_t *c);
int xmmsc_io_want_out (xmmsc_connection_t *c);
int xmmsc_io_out_handle (xmmsc_connection_t *c);
int xmmsc_io_in_handle (xmmsc_connection_t *c);
int xmmsc_io_fd_get (xmmsc_connection_t *c);

char *xmmsc_get_last_error (xmmsc_connection_t *c);

xmmsc_result_t *xmmsc_quit(xmmsc_connection_t *);

xmmsc_result_t *xmmsc_broadcast_quit (xmmsc_connection_t *c);

/* get user config dir */
const char *xmmsc_userconfdir_get (char *buf, int len);


/*
 * PLAYLIST ************************************************
 */

/* commands */
xmmsc_result_t *xmmsc_playlist_list (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_playlist_create (xmmsc_connection_t *c, const char *playlist);
xmmsc_result_t *xmmsc_playlist_shuffle (xmmsc_connection_t *c, const char *playlist);
xmmsc_result_t *xmmsc_playlist_add_args (xmmsc_connection_t *c, const char *playlist, const char *, int, const char **);
xmmsc_result_t *xmmsc_playlist_add_url (xmmsc_connection_t *c, const char *playlist, const char *url);
xmmsc_result_t *xmmsc_playlist_add_id (xmmsc_connection_t *c, const char *playlist, uint32_t id);
xmmsc_result_t *xmmsc_playlist_add_encoded (xmmsc_connection_t *c, const char *playlist, const char *url);
xmmsc_result_t *xmmsc_playlist_add_idlist (xmmsc_connection_t *c, const char *playlist, xmmsv_coll_t *coll);
xmmsc_result_t *xmmsc_playlist_add_collection (xmmsc_connection_t *c, const char *playlist, xmmsv_coll_t *coll, xmmsv_t *order);
xmmsc_result_t *xmmsc_playlist_remove_entry (xmmsc_connection_t *c, const char *playlist, uint32_t);
xmmsc_result_t *xmmsc_playlist_clear (xmmsc_connection_t *c, const char *playlist);
xmmsc_result_t *xmmsc_playlist_remove (xmmsc_connection_t *c, const char *playlist);
xmmsc_result_t *xmmsc_playlist_list_entries (xmmsc_connection_t *c, const char *playlist);
xmmsc_result_t *xmmsc_playlist_sort (xmmsc_connection_t *c, const char *playlist, xmmsv_t *properties);
xmmsc_result_t *xmmsc_playlist_set_next (xmmsc_connection_t *c, uint32_t);
xmmsc_result_t *xmmsc_playlist_set_next_rel (xmmsc_connection_t *c, int32_t);
xmmsc_result_t *xmmsc_playlist_move_entry (xmmsc_connection_t *c, const char *playlist, uint32_t, uint32_t);
xmmsc_result_t *xmmsc_playlist_current_pos (xmmsc_connection_t *c, const char *playlist);
xmmsc_result_t *xmmsc_playlist_current_active (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_playlist_insert_args (xmmsc_connection_t *c, const char *playlist, int pos, const char *url, int numargs, const char **args);
xmmsc_result_t *xmmsc_playlist_insert_url (xmmsc_connection_t *c, const char *playlist, int pos, const char *url);
xmmsc_result_t *xmmsc_playlist_insert_id (xmmsc_connection_t *c, const char *playlist, int pos, uint32_t id);
xmmsc_result_t *xmmsc_playlist_insert_encoded (xmmsc_connection_t *c, const char *playlist, int pos, const char *url);
xmmsc_result_t *xmmsc_playlist_insert_collection (xmmsc_connection_t *c, const char *playlist, int pos, xmmsv_coll_t *coll, xmmsv_t *order);
xmmsc_result_t *xmmsc_playlist_load (xmmsc_connection_t *c, const char *playlist);
xmmsc_result_t *xmmsc_playlist_radd (xmmsc_connection_t *c, const char *playlist, const char *url);
xmmsc_result_t *xmmsc_playlist_radd_encoded (xmmsc_connection_t *c, const char *playlist, const char *url);

/* broadcasts */
xmmsc_result_t *xmmsc_broadcast_playlist_changed (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_broadcast_playlist_current_pos (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_broadcast_playlist_loaded (xmmsc_connection_t *c);


/*
 * PLAYBACK ************************************************
 */

/* commands */
xmmsc_result_t *xmmsc_playback_stop (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_playback_tickle (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_playback_start (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_playback_pause (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_playback_current_id (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_playback_seek_ms (xmmsc_connection_t *c, uint32_t milliseconds);
xmmsc_result_t *xmmsc_playback_seek_ms_rel (xmmsc_connection_t *c, int milliseconds);
xmmsc_result_t *xmmsc_playback_seek_samples (xmmsc_connection_t *c, uint32_t samples);
xmmsc_result_t *xmmsc_playback_seek_samples_rel (xmmsc_connection_t *c, int samples);
xmmsc_result_t *xmmsc_playback_playtime (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_playback_status (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_playback_volume_set (xmmsc_connection_t *c, const char *channel, uint32_t volume);
xmmsc_result_t *xmmsc_playback_volume_get (xmmsc_connection_t *c);

/* broadcasts */
xmmsc_result_t *xmmsc_broadcast_playback_volume_changed (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_broadcast_playback_status (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_broadcast_playback_current_id (xmmsc_connection_t *c);

/* signals */
xmmsc_result_t *xmmsc_signal_playback_playtime (xmmsc_connection_t *c);


/*
 * CONFIG **************************************************
 */

/* commands */
xmmsc_result_t *xmmsc_configval_set (xmmsc_connection_t *c, const char *key, const char *val);
xmmsc_result_t *xmmsc_configval_list (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_configval_get (xmmsc_connection_t *c, const char *key);
xmmsc_result_t *xmmsc_configval_register (xmmsc_connection_t *c, const char *valuename, const char *defaultvalue);

/* broadcasts */
xmmsc_result_t *xmmsc_broadcast_configval_changed (xmmsc_connection_t *c);


/*
 * STATS **************************************************
 */

/* commands */
xmmsc_result_t *xmmsc_plugin_list (xmmsc_connection_t *c, xmms_plugin_type_t type);
xmmsc_result_t *xmmsc_main_stats (xmmsc_connection_t *c);

/* broadcasts */
xmmsc_result_t *xmmsc_broadcast_mediainfo_reader_status (xmmsc_connection_t *c);

/* signals */
xmmsc_result_t *xmmsc_signal_mediainfo_reader_unindexed (xmmsc_connection_t *c);

/*
 * MEDIALIB ***********************************************
 */

/* commands */
int xmmsc_entry_format (char *target, int len, const char *fmt, xmmsv_t *val);
xmmsc_result_t *xmmsc_medialib_add_entry (xmmsc_connection_t *conn, const char *url);
xmmsc_result_t *xmmsc_medialib_add_entry_args (xmmsc_connection_t *conn, const char *url, int numargs, const char **args);
xmmsc_result_t *xmmsc_medialib_add_entry_encoded (xmmsc_connection_t *conn, const char *url);
xmmsc_result_t *xmmsc_medialib_get_info (xmmsc_connection_t *, uint32_t);
xmmsc_result_t *xmmsc_medialib_path_import (xmmsc_connection_t *conn, const char *path);
xmmsc_result_t *xmmsc_medialib_path_import_encoded (xmmsc_connection_t *conn, const char *path);
xmmsc_result_t *xmmsc_medialib_rehash (xmmsc_connection_t *conn, uint32_t id);
xmmsc_result_t *xmmsc_medialib_get_id (xmmsc_connection_t *conn, const char *url);
xmmsc_result_t *xmmsc_medialib_remove_entry (xmmsc_connection_t *conn, uint32_t entry);
xmmsc_result_t *xmmsc_medialib_move_entry (xmmsc_connection_t *conn, uint32_t entry, const char *url);

xmmsc_result_t *xmmsc_medialib_entry_property_set_int (xmmsc_connection_t *c, uint32_t id, const char *key, int32_t value);
xmmsc_result_t *xmmsc_medialib_entry_property_set_int_with_source (xmmsc_connection_t *c, uint32_t id, const char *source, const char *key, int32_t value);

xmmsc_result_t *xmmsc_medialib_entry_property_set_str (xmmsc_connection_t *c, uint32_t id, const char *key, const char *value);
xmmsc_result_t *xmmsc_medialib_entry_property_set_str_with_source (xmmsc_connection_t *c, uint32_t id, const char *source, const char *key, const char *value);

xmmsc_result_t *xmmsc_medialib_entry_property_remove (xmmsc_connection_t *c, uint32_t id, const char *key);
xmmsc_result_t *xmmsc_medialib_entry_property_remove_with_source (xmmsc_connection_t *c, uint32_t id, const char *source, const char *key);

/* XForm object */
xmmsc_result_t *xmmsc_xform_media_browse (xmmsc_connection_t *c, const char *url);
xmmsc_result_t *xmmsc_xform_media_browse_encoded (xmmsc_connection_t *c, const char *url);

/* Bindata object */
xmmsc_result_t *xmmsc_bindata_add (xmmsc_connection_t *c, const unsigned char *data, unsigned int len);
xmmsc_result_t *xmmsc_bindata_retrieve (xmmsc_connection_t *c, const char *hash);
xmmsc_result_t *xmmsc_bindata_remove (xmmsc_connection_t *c, const char *hash);
xmmsc_result_t *xmmsc_bindata_list (xmmsc_connection_t *c);

/* broadcasts */
xmmsc_result_t *xmmsc_broadcast_medialib_entry_changed (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_broadcast_medialib_entry_added (xmmsc_connection_t *c);


/*
 * COLLECTION ***********************************************
 */

xmmsc_result_t* xmmsv_coll_get (xmmsc_connection_t *conn, const char *collname, xmmsv_coll_namespace_t ns);
xmmsc_result_t* xmmsv_coll_list (xmmsc_connection_t *conn, xmmsv_coll_namespace_t ns);
xmmsc_result_t* xmmsv_coll_save (xmmsc_connection_t *conn, xmmsv_coll_t *coll, const char* name, xmmsv_coll_namespace_t ns);
xmmsc_result_t* xmmsv_coll_remove (xmmsc_connection_t *conn, const char* name, xmmsv_coll_namespace_t ns);
xmmsc_result_t* xmmsv_coll_find (xmmsc_connection_t *conn, unsigned int mediaid, xmmsv_coll_namespace_t ns);
xmmsc_result_t* xmmsv_coll_rename (xmmsc_connection_t *conn, const char* from_name, const char* to_name, xmmsv_coll_namespace_t ns);
xmmsc_result_t *xmmsv_coll_idlist_from_playlist_file (xmmsc_connection_t *conn, const char *path);
xmmsc_result_t* xmmsv_coll_sync (xmmsc_connection_t *conn);

xmmsc_result_t* xmmsv_coll_query_ids (xmmsc_connection_t *conn, xmmsv_coll_t *coll, xmmsv_t *order, unsigned int limit_start, unsigned int limit_len);
xmmsc_result_t* xmmsv_coll_query_infos (xmmsc_connection_t *conn, xmmsv_coll_t *coll, xmmsv_t *order, unsigned int limit_start, unsigned int limit_len, xmmsv_t *fetch, xmmsv_t *group);

/* string-to-collection parser */
typedef enum {
	XMMS_COLLECTION_TOKEN_INVALID,
	XMMS_COLLECTION_TOKEN_GROUP_OPEN,
	XMMS_COLLECTION_TOKEN_GROUP_CLOSE,
	XMMS_COLLECTION_TOKEN_REFERENCE,
	XMMS_COLLECTION_TOKEN_SYMBOL_ID,
	XMMS_COLLECTION_TOKEN_STRING,
	XMMS_COLLECTION_TOKEN_PATTERN,
	XMMS_COLLECTION_TOKEN_INTEGER,
	XMMS_COLLECTION_TOKEN_SEQUENCE,
	XMMS_COLLECTION_TOKEN_PROP_LONG,
	XMMS_COLLECTION_TOKEN_PROP_SHORT,
	XMMS_COLLECTION_TOKEN_OPSET_UNION,
	XMMS_COLLECTION_TOKEN_OPSET_INTERSECTION,
	XMMS_COLLECTION_TOKEN_OPSET_COMPLEMENT,
	XMMS_COLLECTION_TOKEN_OPFIL_HAS,
	XMMS_COLLECTION_TOKEN_OPFIL_EQUALS,
	XMMS_COLLECTION_TOKEN_OPFIL_MATCH,
	XMMS_COLLECTION_TOKEN_OPFIL_SMALLER,
	XMMS_COLLECTION_TOKEN_OPFIL_GREATER,
	XMMS_COLLECTION_TOKEN_OPFIL_SMALLEREQ,
	XMMS_COLLECTION_TOKEN_OPFIL_GREATEREQ
} xmmsv_coll_token_type_t;

#define XMMS_COLLECTION_TOKEN_CUSTOM 32

typedef struct xmmsv_coll_token_St xmmsv_coll_token_t;

struct xmmsv_coll_token_St {
	xmmsv_coll_token_type_t type;
	char *string;
	xmmsv_coll_token_t *next;
};

typedef xmmsv_coll_token_t* (*xmmsv_coll_parse_tokens_f) (const char *str, const char **newpos);
typedef xmmsv_coll_t* (*xmmsv_coll_parse_build_f) (xmmsv_coll_token_t *tokens);

int xmmsv_coll_parse (const char *pattern, xmmsv_coll_t** coll);
int xmmsv_coll_parse_custom (const char *pattern, xmmsv_coll_parse_tokens_f parse_f, xmmsv_coll_parse_build_f build_f, xmmsv_coll_t** coll);

xmmsv_coll_t *xmmsv_coll_default_parse_build (xmmsv_coll_token_t *tokens);
xmmsv_coll_token_t *xmmsv_coll_default_parse_tokens (const char *str, const char **newpos);


/* broadcasts */
xmmsc_result_t *xmmsc_broadcast_collection_changed (xmmsc_connection_t *c);


/*
 * MACROS
 */

#define XMMS_CALLBACK_SET(conn,meth,callback,udata) \
	XMMS_CALLBACK_SET_FULL(conn,meth,callback,udata,NULL);

#define XMMS_CALLBACK_SET_FULL(conn,meth,callback,udata,free_func) {\
	xmmsc_result_t *res = meth (conn); \
	xmmsc_result_notifier_set_full (res, callback, udata, free_func);\
	xmmsc_result_unref (res);\
}

/*
 * RESULTS
 */

typedef int (*xmmsc_result_notifier_t) (xmmsv_t *val, void *user_data);

xmmsc_result_type_t xmmsc_result_get_class (xmmsc_result_t *res);

xmmsc_result_t *xmmsc_result_ref (xmmsc_result_t *res);
void xmmsc_result_unref (xmmsc_result_t *res);

void xmmsc_result_notifier_set (xmmsc_result_t *res, xmmsc_result_notifier_t func, void *user_data);
void xmmsc_result_notifier_set_full (xmmsc_result_t *res, xmmsc_result_notifier_t func, void *user_data, xmmsc_user_data_free_func_t free_func);
void xmmsc_result_wait (xmmsc_result_t *res);

xmmsv_t *xmmsc_result_get_value (xmmsc_result_t *res);

/* Legacy aliases for convenience. */
#define xmmsc_result_iserror(res) xmmsv_iserror(xmmsc_result_get_value(res))
#define xmmsc_result_get_error(res) xmmsv_get_error_old(xmmsc_result_get_value(res))

/*
 * Service Clients
 */
typedef struct xmmsc_service_St xmmsc_service_t;

typedef xmmsv_t* (*xmmsc_service_notifier_t) (xmmsc_connection_t *conn,
                                             xmmsc_service_t *svc,
                                             const char *method,
                                             xmmsv_t *args,
                                             void *udata);
xmmsc_service_t *xmmsc_service_new (const char *name, const char *desc,
                                    uint32_t major, uint32_t minor);
void xmmsc_service_unref (xmmsc_service_t *svc);
int xmmsc_service_method_add (xmmsc_service_t *svc, const char *name,
                              const char *desc, xmmsv_type_t rettype,
                              xmmsc_service_notifier_t func, void *udata, ...);
int xmmsc_service_method_add_full (xmmsc_service_t *svc, const char *name,
                                   const char *desc, xmmsv_type_t rettype,
                                   xmmsc_service_notifier_t func, void *udata,
                                   xmmsc_user_data_free_func_t ufree, ...);
int xmmsc_service_method_add_noarg (xmmsc_service_t *svc, const char *name,
                                    const char *desc, xmmsv_type_t rettype,
                                    xmmsc_service_notifier_t func, void *udata,
                                    xmmsc_user_data_free_func_t ufree);
int xmmsc_service_method_add_arg (xmmsc_service_t *svc, const char *name,
                                  xmmsv_type_t type, int32_t optional);
xmmsc_result_t *xmmsc_service_register (xmmsc_connection_t *conn,
                                        xmmsc_service_t *svc);
xmmsc_result_t *xmmsc_service_unregister (xmmsc_connection_t *conn,
                                          xmmsc_service_t *svc);
xmmsc_result_t *xmmsc_service_list (xmmsc_connection_t *conn);
xmmsc_result_t *xmmsc_service_describe (xmmsc_connection_t *conn,
                                        const char *service);
xmmsc_result_t *xmmsc_service_query (xmmsc_connection_t *conn,
	                                 const char *svc, const char *meth,
                                     xmmsv_t *args);
xmmsc_result_t *xmmsc_service_shutdown (xmmsc_connection_t *conn,
                                        const char *service);

/* broadcasts */
xmmsc_result_t *xmmsc_broadcast_service_changed (xmmsc_connection_t *c);
xmmsc_result_t *xmmsc_broadcast_service_shutdown (xmmsc_connection_t *c);

#ifdef __cplusplus
}
#endif

#endif
