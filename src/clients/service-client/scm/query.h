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

#ifndef __QUERY_H__
#define __QUERY_H__

#include "common.h"

void cb_list_sc_ids (xmmsc_connection_t *conn, xmmsc_result_t *res,
                     xmmsc_service_method_t *method, void *data);
void cb_list_sc (xmmsc_connection_t *conn, xmmsc_result_t *res,
                 xmmsc_service_method_t *method, void *data);
void cb_list_service_ids (xmmsc_connection_t *conn, xmmsc_result_t *res,
                          xmmsc_service_method_t *method, void *data);
void cb_list_service (xmmsc_connection_t *conn, xmmsc_result_t *res,
                      xmmsc_service_method_t *method, void *data);
void cb_list_method_ids (xmmsc_connection_t *conn, xmmsc_result_t *res,
                         xmmsc_service_method_t *method, void *data);
void cb_list_method (xmmsc_connection_t *conn, xmmsc_result_t *res,
                     xmmsc_service_method_t *method, void *data);
void cb_lookup_client (xmmsc_connection_t *conn, xmmsc_result_t *res,
                       xmmsc_service_method_t *method, void *data);

#endif
