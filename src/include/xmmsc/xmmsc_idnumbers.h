/*  XMMS2 - X Music Multiplexer System
 *  Copyright (C) 2003-2011 XMMS2 Team
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

#ifndef __SIGNAL_XMMS_H__
#define __SIGNAL_XMMS_H__
#include "xmmsc/xmmsc_compiler.h"

/* Don't forget to up this when protocol changes */
#define XMMS_IPC_PROTOCOL_VERSION 19

typedef enum {
	XMMS_IPC_OBJECT_SIGNAL,
	XMMS_IPC_OBJECT_MAIN,
	XMMS_IPC_OBJECT_PLAYLIST,
	XMMS_IPC_OBJECT_CONFIG,
	XMMS_IPC_OBJECT_PLAYBACK,
	XMMS_IPC_OBJECT_MEDIALIB,
	XMMS_IPC_OBJECT_COLLECTION,
	XMMS_IPC_OBJECT_VISUALIZATION,
	XMMS_IPC_OBJECT_MEDIAINFO_READER,
	XMMS_IPC_OBJECT_XFORM,
	XMMS_IPC_OBJECT_BINDATA,
	XMMS_IPC_OBJECT_END
} xmms_ipc_objects_t;

typedef enum {
	XMMS_IPC_SIGNAL_PLAYLIST_CHANGED,
	XMMS_IPC_SIGNAL_CONFIGVALUE_CHANGED,
	XMMS_IPC_SIGNAL_PLAYBACK_STATUS,
	XMMS_IPC_SIGNAL_PLAYBACK_VOLUME_CHANGED,
	XMMS_IPC_SIGNAL_PLAYBACK_PLAYTIME,
	XMMS_IPC_SIGNAL_PLAYBACK_CURRENTID,
	XMMS_IPC_SIGNAL_PLAYLIST_CURRENT_POS,
	XMMS_IPC_SIGNAL_PLAYLIST_LOADED,
	XMMS_IPC_SIGNAL_MEDIALIB_ENTRY_ADDED,
	XMMS_IPC_SIGNAL_MEDIALIB_ENTRY_UPDATE,
	XMMS_IPC_SIGNAL_MEDIALIB_ENTRY_REMOVED,
	XMMS_IPC_SIGNAL_COLLECTION_CHANGED,
	XMMS_IPC_SIGNAL_QUIT,
	XMMS_IPC_SIGNAL_MEDIAINFO_READER_STATUS,
	XMMS_IPC_SIGNAL_MEDIAINFO_READER_UNINDEXED,
	XMMS_IPC_SIGNAL_END
} xmms_ipc_signals_t;

/* Commands 0..31 are reserved for special stuff like marking
 * a reply or an error.
 */
#define XMMS_IPC_CMD_FIRST 32

/* Special "commands" (0..31) */
typedef enum {
	XMMS_IPC_CMD_REPLY,
	XMMS_IPC_CMD_ERROR
} xmms_ipc_pseudo_commands;

/* Signal subsystem methods */
typedef enum {
	XMMS_IPC_CMD_SIGNAL = XMMS_IPC_CMD_FIRST,
	XMMS_IPC_CMD_BROADCAST
} xmms_ipc_signal_cmds_t;

/* Main methods */
typedef enum {
	XMMS_IPC_CMD_HELLO = XMMS_IPC_CMD_FIRST,
	XMMS_IPC_CMD_QUIT,
	XMMS_IPC_CMD_PLUGIN_LIST,
	XMMS_IPC_CMD_STATS
} xmms_ipc_main_cmds_t;

/* Playlist methods */
typedef enum {
	XMMS_IPC_CMD_SHUFFLE = XMMS_IPC_CMD_FIRST,
	XMMS_IPC_CMD_SET_POS,
	XMMS_IPC_CMD_SET_POS_REL,
	XMMS_IPC_CMD_ADD_URL,
	XMMS_IPC_CMD_ADD_ID,
	XMMS_IPC_CMD_ADD_IDLIST,
	XMMS_IPC_CMD_ADD_COLL,
	XMMS_IPC_CMD_REMOVE_ENTRY,
	XMMS_IPC_CMD_MOVE_ENTRY,
	XMMS_IPC_CMD_CLEAR,
	XMMS_IPC_CMD_SORT,
	XMMS_IPC_CMD_LIST,
	XMMS_IPC_CMD_CURRENT_POS,
	XMMS_IPC_CMD_CURRENT_ACTIVE,
	XMMS_IPC_CMD_INSERT_URL,
	XMMS_IPC_CMD_INSERT_ID,
	XMMS_IPC_CMD_INSERT_COLL,
	XMMS_IPC_CMD_LOAD,
	XMMS_IPC_CMD_RADD,
	XMMS_IPC_CMD_RINSERT
} xmms_ipc_playlist_cmds_t;

/* Config methods */
typedef enum {
	XMMS_IPC_CMD_GETVALUE = XMMS_IPC_CMD_FIRST,
	XMMS_IPC_CMD_SETVALUE,
	XMMS_IPC_CMD_REGVALUE,
	XMMS_IPC_CMD_LISTVALUES
} xmms_ipc_config_cmds_t;

/* playback methods */
typedef enum {
	XMMS_IPC_CMD_START = XMMS_IPC_CMD_FIRST,
	XMMS_IPC_CMD_STOP,
	XMMS_IPC_CMD_PAUSE,
	XMMS_IPC_CMD_DECODER_KILL,
	XMMS_IPC_CMD_CPLAYTIME,
	XMMS_IPC_CMD_SEEKMS,
	XMMS_IPC_CMD_SEEKSAMPLES,
	XMMS_IPC_CMD_PLAYBACK_STATUS,
	XMMS_IPC_CMD_CURRENTID,
	XMMS_IPC_CMD_VOLUME_SET,
	XMMS_IPC_CMD_VOLUME_GET
} xmms_ipc_playback_cmds_t;

/* Medialib methods */
typedef enum {
	XMMS_IPC_CMD_INFO = XMMS_IPC_CMD_FIRST,
	XMMS_IPC_CMD_PATH_IMPORT,
	XMMS_IPC_CMD_REHASH,
	XMMS_IPC_CMD_GET_ID,
	XMMS_IPC_CMD_REMOVE_ID,
	XMMS_IPC_CMD_PROPERTY_SET_STR,
	XMMS_IPC_CMD_PROPERTY_SET_INT,
	XMMS_IPC_CMD_PROPERTY_REMOVE,
	XMMS_IPC_CMD_MOVE_URL,
	XMMS_IPC_CMD_MLIB_ADD_URL
} xmms_ipc_medialib_cmds_t;

/* Collection methods */
typedef enum {
	XMMS_IPC_CMD_COLLECTION_GET = XMMS_IPC_CMD_FIRST,
	XMMS_IPC_CMD_COLLECTION_LIST,
	XMMS_IPC_CMD_COLLECTION_SAVE,
	XMMS_IPC_CMD_COLLECTION_REMOVE,
	XMMS_IPC_CMD_COLLECTION_FIND,
	XMMS_IPC_CMD_COLLECTION_RENAME,
	XMMS_IPC_CMD_QUERY,
	XMMS_IPC_CMD_QUERY_INFOS,
	XMMS_IPC_CMD_IDLIST_FROM_PLS,
	XMMS_IPC_CMD_COLLECTION_SYNC
} xmms_ipc_collection_cmds_t;

/* bindata methods */
typedef enum {
	XMMS_IPC_CMD_GET_DATA = XMMS_IPC_CMD_FIRST,
	XMMS_IPC_CMD_ADD_DATA,
	XMMS_IPC_CMD_REMOVE_DATA,
	XMMS_IPC_CMD_LIST_DATA
} xmms_ipc_bindata_cmds_t;

/* visualization methods */
typedef enum {
	XMMS_IPC_CMD_VISUALIZATION_QUERY_VERSION = XMMS_IPC_CMD_FIRST,
	XMMS_IPC_CMD_VISUALIZATION_REGISTER,
	XMMS_IPC_CMD_VISUALIZATION_INIT_SHM,
	XMMS_IPC_CMD_VISUALIZATION_INIT_UDP,
	XMMS_IPC_CMD_VISUALIZATION_PROPERTY,
	XMMS_IPC_CMD_VISUALIZATION_PROPERTIES,
	XMMS_IPC_CMD_VISUALIZATION_SHUTDOWN
} xmms_ipc_visualization_cmds_t;

/* xform methods */
typedef enum {
	XMMS_IPC_CMD_BROWSE = XMMS_IPC_CMD_FIRST
} xmms_ipc_xform_cmds_t;

typedef enum {
	XMMS_PLAYLIST_CHANGED_ADD,
	XMMS_PLAYLIST_CHANGED_INSERT,
	XMMS_PLAYLIST_CHANGED_SHUFFLE,
	XMMS_PLAYLIST_CHANGED_REMOVE,
	XMMS_PLAYLIST_CHANGED_CLEAR,
	XMMS_PLAYLIST_CHANGED_MOVE,
	XMMS_PLAYLIST_CHANGED_SORT,
	XMMS_PLAYLIST_CHANGED_UPDATE
} xmms_playlist_changed_actions_t;

typedef enum {
	XMMS_COLLECTION_CHANGED_ADD,
	XMMS_COLLECTION_CHANGED_UPDATE,
	XMMS_COLLECTION_CHANGED_RENAME,
	XMMS_COLLECTION_CHANGED_REMOVE
} xmms_collection_changed_actions_t;

typedef enum {
	XMMS_PLAYBACK_STATUS_STOP,
	XMMS_PLAYBACK_STATUS_PLAY,
	XMMS_PLAYBACK_STATUS_PAUSE
} xmms_playback_status_t;

typedef enum {
	XMMS_PLAYBACK_SEEK_CUR = 1,
	XMMS_PLAYBACK_SEEK_SET
} xmms_playback_seek_mode_t;

typedef enum {
	XMMS_MEDIAINFO_READER_STATUS_IDLE,
	XMMS_MEDIAINFO_READER_STATUS_RUNNING
} xmms_mediainfo_reader_status_t;

typedef enum {
	XMMS_PLUGIN_TYPE_ALL,
	XMMS_PLUGIN_TYPE_OUTPUT,
	XMMS_PLUGIN_TYPE_XFORM
} xmms_plugin_type_t;

typedef enum {
	XMMS_COLLECTION_TYPE_REFERENCE,
	XMMS_COLLECTION_TYPE_UNIVERSE,
	XMMS_COLLECTION_TYPE_UNION,
	XMMS_COLLECTION_TYPE_INTERSECTION,
	XMMS_COLLECTION_TYPE_COMPLEMENT,
	XMMS_COLLECTION_TYPE_HAS,
	XMMS_COLLECTION_TYPE_MATCH,
	XMMS_COLLECTION_TYPE_TOKEN,
	XMMS_COLLECTION_TYPE_EQUALS,
	XMMS_COLLECTION_TYPE_NOTEQUAL,
	XMMS_COLLECTION_TYPE_SMALLER,
	XMMS_COLLECTION_TYPE_SMALLEREQ,
	XMMS_COLLECTION_TYPE_GREATER,
	XMMS_COLLECTION_TYPE_GREATEREQ,
	XMMS_COLLECTION_TYPE_ORDER,
	XMMS_COLLECTION_TYPE_LIMIT,
	XMMS_COLLECTION_TYPE_MEDIASET,
	XMMS_COLLECTION_TYPE_IDLIST,
	XMMS_COLLECTION_TYPE_LAST = XMMS_COLLECTION_TYPE_IDLIST
} xmmsv_coll_type_t;

typedef enum {
	XMMS_MEDIALIB_ENTRY_STATUS_NEW,
	XMMS_MEDIALIB_ENTRY_STATUS_OK,
	XMMS_MEDIALIB_ENTRY_STATUS_RESOLVING,
	XMMS_MEDIALIB_ENTRY_STATUS_NOT_AVAILABLE,
	XMMS_MEDIALIB_ENTRY_STATUS_REHASH
} xmmsc_medialib_entry_status_t;

typedef const char* xmmsv_coll_namespace_t;
#define	XMMS_COLLECTION_NS_ALL          "*"
#define XMMS_COLLECTION_NS_COLLECTIONS  "Collections"
#define XMMS_COLLECTION_NS_PLAYLISTS    "Playlists"

#define XMMS_ACTIVE_PLAYLIST "_active"

/* Default source preferences for accessing "propdicts" (decl. in value.c) */
extern const char *xmmsv_default_source_pref[] XMMS_PUBLIC;

/* compability */
typedef xmmsv_coll_type_t xmmsc_coll_type_t;
typedef xmmsv_coll_namespace_t xmmsc_coll_namespace_t;


#endif /* __SIGNAL_XMMS_H__ */
