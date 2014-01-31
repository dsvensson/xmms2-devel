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
#include <locale.h>
#include <stdint.h>

#include "xcu.h"

#include <xmmspriv/xmms_log.h>
#include <xmmspriv/xmms_ipc.h>
#include <xmmspriv/xmms_config.h>
#include <xmmspriv/xmms_medialib.h>
#include <xmmspriv/xmms_collection.h>

#include "utils/jsonism.h"
#include "utils/value_utils.h"
#include "utils/coll_utils.h"
#include "server-utils/ipc_call.h"
#include "server-utils/mlib_utils.h"


#include <xmmspriv/xmms_tokenmanager.h>

static xmms_medialib_t *medialib;
static xmms_xform_token_manager_t *mgr;

SETUP (token_manager) {
	setlocale (LC_COLLATE, "");
	xmms_ipc_init ();
	xmms_log_init (0);
	xmms_config_init ("memory://");
	xmms_config_property_register ("medialib.path", "memory://", NULL, NULL);
	medialib = xmms_medialib_init ();
	mgr = xmms_xform_token_manager_new (medialib);
	return 0;
}

CLEANUP () {
	xmms_xform_token_manager_free (mgr);
	mgr = NULL;
	xmms_object_unref (medialib);
	medialib = NULL;
	xmms_config_shutdown ();
	xmms_ipc_shutdown ();
	return 0;
}

#define SRC_NAME_A "a"
#define SRC_NAME_B "b"
#define SRC_ORDER_A 0
#define SRC_ORDER_B 1

CASE (test_basics) {
	xmms_xform_token_t t1, t2, t3;
	gchar *string;
	int64_t number;

	t1 = xmms_xform_token_manager_set_volatile_string (mgr, SRC_ORDER_A, SRC_NAME_A,
	                                                   "artist", "One");
	CU_ASSERT_EQUAL (1, t1);
	t2 = xmms_xform_token_manager_set_persistent_string (mgr, SRC_ORDER_A, SRC_NAME_A,
	                                                     "artist", "Two");
	CU_ASSERT_EQUAL (2, t2);
	t3 = xmms_xform_token_manager_set_persistent_int (mgr, SRC_ORDER_A, SRC_NAME_A,
	                                                  "tracknr", 42);
	CU_ASSERT_EQUAL (3, t3);

	CU_ASSERT_TRUE (xmms_xform_token_manager_get_string (mgr, t3,
	                                                     SRC_ORDER_B, SRC_NAME_B,
	                                                     "artist", &string));
	CU_ASSERT_STRING_EQUAL ("Two", string);
	g_free (string);

	CU_ASSERT_TRUE (xmms_xform_token_manager_get_string (mgr, t2,
	                                                     SRC_ORDER_A, SRC_NAME_A,
	                                                     "artist", &string));
	CU_ASSERT_STRING_EQUAL ("Two", string);
	g_free (string);

	CU_ASSERT_TRUE (xmms_xform_token_manager_get_string (mgr, t1,
	                                                     SRC_ORDER_A, SRC_NAME_A,
	                                                     "artist", &string));
	CU_ASSERT_STRING_EQUAL ("One", string);
	g_free (string);

	CU_ASSERT_FALSE (xmms_xform_token_manager_get_string (mgr, t3,
	                                                      SRC_ORDER_B, SRC_NAME_B,
	                                                      "title", &string));

	CU_ASSERT_TRUE (xmms_xform_token_manager_get_int (mgr, t3,
	                                                  SRC_ORDER_B, SRC_NAME_B,
	                                                  "tracknr", &number));
	CU_ASSERT_EQUAL (42, number);
}

CASE (test_order)
{
	xmms_xform_token_t t0, t1, t2, t3;
	int64_t number;

	t0 = xmms_xform_token_manager_set_int (mgr, UINT8_MAX, SRC_NAME_B, "key0", 23,
	                                       XMMS_XFORM_METADATA_LIFETIME_ORIGIN);

	t1 = xmms_xform_token_manager_set_persistent_int (mgr, SRC_ORDER_A, SRC_NAME_A,
	                                                  "key1", 1);
	t2 = xmms_xform_token_manager_set_persistent_int (mgr, SRC_ORDER_B, SRC_ORDER_A,
	                                                  "key1", 2);
	t3 = xmms_xform_token_manager_set_persistent_int (mgr, SRC_ORDER_A, SRC_NAME_A,
	                                                  "key2", 3);

	/* Using token t1 and order 0 just to prove that tokens and order don't
	 * matter when accessing origin values where the metadata source match.
	 */
	CU_ASSERT_TRUE (xmms_xform_token_manager_get_int (mgr, t1,
	                                                  0, SRC_NAME_B,
	                                                  "key0", &number));
	CU_ASSERT_EQUAL (23, number);

	/* Accessing origin data via other than origin source will fail */
	CU_ASSERT_FALSE (xmms_xform_token_manager_get_int (mgr, t1,
	                                                  0, SRC_NAME_A,
	                                                  "key0", &number));


	/* Should only be able to access metadata set by self or earlier in the chain,
	 * and will thus not see "key1" set by the second xform in the chain
	 */
	CU_ASSERT_TRUE (xmms_xform_token_manager_get_int (mgr, t3,
	                                                  SRC_ORDER_A, SRC_NAME_A,
	                                                  "key1", &number));
	CU_ASSERT_EQUAL (1, number);
}

CASE (test_url_and_arg_decoding)
{
	xmms_medialib_session_t *session;
	xmms_medialib_entry_t entry;
	xmms_error_t err;
	const gchar *string;

	session = xmms_medialib_session_begin (medialib);
	entry = xmms_medialib_entry_new_encoded (session, "file:///test/test.sid?subtune=9", &err);
	xmms_medialib_session_commit (session);

	session = xmms_medialib_session_begin_ro (medialib);
	xmms_xform_token_manager_reset (mgr, session, entry);
	xmms_medialib_session_commit (session);

	CU_ASSERT_TRUE (xmms_xform_token_manager_get_string (mgr, 0,
	                                                     42, "server",
	                                                     "subtune", &string));
	CU_ASSERT_STRING_EQUAL ("9", string);
}
