/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2009 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Alexey Romanenko <santiago739@gmail.com>                     |
  +----------------------------------------------------------------------+
*/

/* $Id: gupnp.c,v 1.2 2009/05/15 11:19:26 santiago Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_gupnp.h"

ZEND_DECLARE_MODULE_GLOBALS(gupnp)

#ifndef Z_ADDREF_P
# define Z_ADDREF_P(x) (x)->refcount++
#endif

typedef struct _php_gupnp_callback_t { /* {{{ */
    zval *func;
    zval *arg;
} php_gupnp_callback_t;
/* }}} */

typedef struct _php_gupnp_context_t { /* {{{ */
	GUPnPContext *context;
	int rsrc_id;
	php_gupnp_callback_t *callback_timeout;
#ifdef ZTS
	void ***thread_ctx;
#endif
} php_gupnp_context_t;
/* }}} */

typedef struct _php_gupnp_cpoint_t { /* {{{ */
	GUPnPControlPoint *cp;
	int rsrc_id;
	php_gupnp_callback_t *callbacks[4];
	GMainLoop *main_loop;
#ifdef ZTS
	void ***thread_ctx;
#endif
} php_gupnp_cpoint_t;
/* }}} */

typedef struct _php_gupnp_rdevice_t { /* {{{ */
	GUPnPRootDevice *rd;
	int rsrc_id;
	GMainLoop *main_loop;
} php_gupnp_rdevice_t;
/* }}} */

typedef struct _php_gupnp_service_t { /* {{{ */
	GUPnPService *service;
	int rsrc_id;
} php_gupnp_service_t;
/* }}} */

typedef struct _php_gupnp_service_proxy_t { /* {{{ */
	GUPnPServiceProxy *proxy;
	int rsrc_id;
	php_gupnp_callback_t *callback;
	php_gupnp_callback_t *callback_signal;
#ifdef ZTS
	void ***thread_ctx;
#endif
} php_gupnp_service_proxy_t;
/* }}} */

/*
typedef struct _php_gupnp_service_proxy_action_t { / {{{ /
	//GUPnPServiceAction *action;
	int rsrc_id;
	php_gupnp_callback_t *callback;
	//php_gupnp_service_t *service;
#ifdef ZTS
	void ***thread_ctx;
#endif
} php_gupnp_service_proxy_action_t; */
/* }}} */

typedef struct _php_gupnp_device_proxy_t { /* {{{ */
	GUPnPDeviceProxy *proxy;
	int rsrc_id;
} php_gupnp_device_proxy_t;
/* }}} */

typedef struct _php_gupnp_service_info_t { /* {{{ */
	GUPnPServiceInfo *service_info;
	int rsrc_id;
	php_gupnp_callback_t *callback;
#ifdef ZTS
	void ***thread_ctx;
#endif
} php_gupnp_service_info_t;
/* }}} */

typedef struct _php_gupnp_service_introspection_t { /* {{{ */
	GUPnPServiceIntrospection *introspection;
	int rsrc_id;
	php_gupnp_callback_t *callback;
#ifdef ZTS
	void ***thread_ctx;
#endif
} php_gupnp_service_introspection_t;
/* }}} */

typedef struct _php_gupnp_service_action_t { /* {{{ */
	GUPnPServiceAction *action;
	int rsrc_id;
	php_gupnp_callback_t *callback;
	php_gupnp_service_t *service;
#ifdef ZTS
	void ***thread_ctx;
#endif
} php_gupnp_service_action_t;
/* }}} */

static int le_context;
static int le_cpoint;
static int le_rdevice;
static int le_service;
static int le_service_proxy;
//static int le_service_proxy_action;
static int le_device_proxy;
static int le_service_info;
static int le_service_introspection;
static int le_service_action;

#define ZVAL_TO_CONTEXT(zval, context) \
		ZEND_FETCH_RESOURCE(context, php_gupnp_context_t *, &zval, -1, "context", le_context)

#define ZVAL_TO_RDEVICE(zval, rdevice) \
		ZEND_FETCH_RESOURCE(rdevice, php_gupnp_rdevice_t *, &zval, -1, "root device", le_rdevice)

#define ZVAL_TO_CPOINT(zval, cpoint) \
		ZEND_FETCH_RESOURCE(cpoint, php_gupnp_cpoint_t *, &zval, -1, "control point", le_cpoint)

#define ZVAL_TO_SERVICE(zval, service) \
		ZEND_FETCH_RESOURCE(service, php_gupnp_service_t *, &zval, -1, "service", le_service)

#define ZVAL_TO_SERVICE_PROXY(zval, service_proxy) \
		ZEND_FETCH_RESOURCE(service_proxy, php_gupnp_service_proxy_t *, &zval, -1, "service proxy", le_service_proxy)

#define ZVAL_TO_DEVICE_PROXY(zval, device_proxy) \
		ZEND_FETCH_RESOURCE(device_proxy, php_gupnp_device_proxy_t *, &zval, -1, "device proxy", le_device_proxy)

#define ZVAL_TO_SERVICE_INFO(zval, service_info) \
		ZEND_FETCH_RESOURCE(service_info, php_gupnp_service_info_t *, &zval, -1, "service info", le_service_info)

#define ZVAL_TO_SERVICE_INTROSPECTION(zval, service_introspection) \
		ZEND_FETCH_RESOURCE(service_introspection, php_gupnp_service_introspection_t *, &zval, -1, "service introspection", le_service_introspection)

#define ZVAL_TO_SERVICE_ACTION(zval, service_action) \
		ZEND_FETCH_RESOURCE(service_action, php_gupnp_service_action_t *, &zval, -1, "service action", le_service_action)

#if PHP_VERSION_ID >= 50300
# define zend_is_callable(callable, check_flags, callable_name) \
		 zend_is_callable(callable, check_flags, callable_name TSRMLS_CC) 
#endif
		
#ifdef COMPILE_DL_GUPNP
ZEND_GET_MODULE(gupnp)
#endif

static inline void _php_gupnp_callback_free(php_gupnp_callback_t *callback) /* {{{ */
{
	if (!callback) {
		return;
	}
	
	if (callback->func) {
		zval_ptr_dtor(&callback->func);
	}
	
	if (callback->arg) {
		zval_ptr_dtor(&callback->arg);
	}
	efree(callback);
}
/* }}} */

static void _php_gupnp_cpoint_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	int i;
	php_gupnp_cpoint_t *cpoint = (php_gupnp_cpoint_t *)rsrc->ptr;
	
	for (i = 0; i < 4; i++) {
		_php_gupnp_callback_free(cpoint->callbacks[i]);
	}
	
	g_object_unref(cpoint->cp);
	g_main_loop_unref(cpoint->main_loop);
	efree(cpoint);
}
/* }}} */

static void _php_gupnp_rdevice_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_rdevice_t *rdevice = (php_gupnp_rdevice_t *)rsrc->ptr;
	
	g_object_unref(rdevice->rd);
	g_main_loop_unref(rdevice->main_loop);
	efree(rdevice);
}
/* }}} */

static void _php_gupnp_service_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_service_t *service = (php_gupnp_service_t *)rsrc->ptr;
	
	efree(service);
}
/* }}} */

static void _php_gupnp_service_proxy_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_service_proxy_t *sproxy = (php_gupnp_service_proxy_t *)rsrc->ptr;
	
	_php_gupnp_callback_free(sproxy->callback);
	_php_gupnp_callback_free(sproxy->callback_signal);
	efree(sproxy);
}
/* }}} */

/*static void _php_gupnp_service_proxy_action_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) / {{{ /
{
	php_gupnp_service_proxy_action_t *sp_action = (php_gupnp_service_proxy_action_t *)rsrc->ptr;
	
	_php_gupnp_callback_free(sp_action->callback);
	efree(sp_action);
}*/
/* }}} */

static void _php_gupnp_device_proxy_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_device_proxy_t *dproxy = (php_gupnp_device_proxy_t *)rsrc->ptr;
	
	efree(dproxy);
}
/* }}} */

static void _php_gupnp_context_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_context_t *context = (php_gupnp_context_t *)rsrc->ptr;
	
	g_object_unref(context->context);
	_php_gupnp_callback_free(context->callback_timeout);
	efree(context);
}
/* }}} */

static void _php_gupnp_service_info_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_service_info_t *service = (php_gupnp_service_info_t *)rsrc->ptr;
	
	g_object_unref(service->service_info);
	_php_gupnp_callback_free(service->callback);
	efree(service);
}
/* }}} */

static void _php_gupnp_service_introspection_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_service_introspection_t *sintrospection = (php_gupnp_service_introspection_t *)rsrc->ptr;
	
	g_object_unref(sintrospection->introspection);
	_php_gupnp_callback_free(sintrospection->callback);
	efree(sintrospection);
}
/* }}} */

static void _php_gupnp_service_action_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_service_action_t *service_action = (php_gupnp_service_action_t *)rsrc->ptr;
	
	_php_gupnp_callback_free(service_action->callback);
	efree(service_action);
}
/* }}} */

static gboolean _php_gupnp_context_timeout_cb(gpointer userdata) /* {{{ */
{
	zval *args[1];
	php_gupnp_context_t *context = (php_gupnp_context_t *)userdata;
	php_gupnp_callback_t *callback;
	zval retval;
	TSRMLS_FETCH_FROM_CTX(context ? context->thread_ctx : NULL);
	if (!context || !context->callback_timeout) {
		return FALSE;
	}
	
	callback = context->callback_timeout;
	args[0] = callback->arg;
	Z_ADDREF_P(callback->arg);
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 1, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));

	return TRUE;
}
/* }}} */

static void _php_gupnp_service_proxy_cb(GUPnPControlPoint *cp, GUPnPServiceProxy *proxy, php_gupnp_callback_t *callback TSRMLS_DC)  /* {{{ */
{
	zval *args[2];
	php_gupnp_service_proxy_t *sproxy;
	zval retval;
	
	sproxy = emalloc(sizeof(php_gupnp_service_proxy_t));
	sproxy->proxy = proxy;
	sproxy->callback = NULL;
	sproxy->callback_signal = NULL;
	TSRMLS_SET_CTX(sproxy->thread_ctx);
	sproxy->rsrc_id = zend_list_insert(sproxy, le_service_proxy);
	
	MAKE_STD_ZVAL(args[0]);
	ZVAL_RESOURCE(args[0], sproxy->rsrc_id);
	zend_list_addref(sproxy->rsrc_id);
	
	args[1] = callback->arg;
	Z_ADDREF_P(callback->arg);
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 2, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	zval_ptr_dtor(&(args[1]));

	return;
}
/* }}} */

//static void _php_gupnp_service_proxy_action_cb(GUPnPServiceProxy *proxy, GUPnPServiceProxyAction *action, gpointer user_data TSRMLS_DC)  /* {{{ */
static void _php_gupnp_service_proxy_action_cb(GUPnPServiceProxy *proxy, GUPnPServiceProxyAction *action, gpointer user_data)
{
	char       *didl_xml;
	guint32     number_returned;
	guint32     total_matches;
	GError     *error;

	didl_xml = NULL;
	error = NULL;
	
	//zval *args[2];
	//zval retval;
	
	printf("[func] _php_gupnp_service_proxy_action_cb\n");
	
	gupnp_service_proxy_end_action(
		proxy,
		action,
		&error,
		/* OUT args */
		"Result",
		G_TYPE_STRING,
		&didl_xml,
		"NumberReturned",
		G_TYPE_UINT,
		&number_returned,
		"TotalMatches",
		G_TYPE_UINT,
		&total_matches,
		NULL
	);
	
	printf("didl_xml: %s\n\n", didl_xml);
	
	/*
	MAKE_STD_ZVAL(args[0]);
	ZVAL_STRING(args[0], (char *)didl_xml, 1); 
	
	if (error && error->message) {
		MAKE_STD_ZVAL(args[1]);
		ZVAL_STRING(args[1], (char *)error->message, 1); 
		g_error_free(error);
	} else {
		ALLOC_INIT_ZVAL(args[1]);
	}
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 2, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	zval_ptr_dtor(&(args[1]));
	*/
	return;
}
/* }}} */

static void _php_gupnp_service_proxy_available_cb(GUPnPControlPoint *cp, GUPnPServiceProxy *proxy, gpointer userdata) /* {{{ */
{
	php_gupnp_cpoint_t *cpoint = (php_gupnp_cpoint_t *)userdata;
	TSRMLS_FETCH_FROM_CTX(cpoint ? cpoint->thread_ctx : NULL);
	if (!cpoint || !cpoint->callbacks[GUPNP_SIGNAL_SERVICE_PROXY_AVAILABLE]) {
		return;
	}
	_php_gupnp_service_proxy_cb(cp, proxy, cpoint->callbacks[GUPNP_SIGNAL_SERVICE_PROXY_AVAILABLE] TSRMLS_CC);
	
	return;
}
/* }}} */

static void _php_gupnp_service_proxy_unavailable_cb(GUPnPControlPoint *cp, GUPnPServiceProxy *proxy, gpointer userdata) /* {{{ */
{
	php_gupnp_cpoint_t *cpoint = (php_gupnp_cpoint_t *)userdata;
	TSRMLS_FETCH_FROM_CTX(cpoint ? cpoint->thread_ctx : NULL);
	if (!cpoint || !cpoint->callbacks[GUPNP_SIGNAL_SERVICE_PROXY_UNAVAILABLE]) {
		return;
	}
	_php_gupnp_service_proxy_cb(cp, proxy, cpoint->callbacks[GUPNP_SIGNAL_SERVICE_PROXY_UNAVAILABLE] TSRMLS_CC);
	
	return;
}
/* }}} */

static void _php_gupnp_device_proxy_cb(GUPnPControlPoint *cp, GUPnPDeviceProxy *proxy, php_gupnp_callback_t *callback TSRMLS_DC) /* {{{ */
{
	zval *args[2];
	php_gupnp_device_proxy_t *dproxy;
	zval retval;
	
	dproxy = emalloc(sizeof(php_gupnp_device_proxy_t));
	dproxy->proxy = proxy;
	dproxy->rsrc_id = zend_list_insert(dproxy, le_device_proxy);
	
	MAKE_STD_ZVAL(args[0]);
	ZVAL_RESOURCE(args[0], dproxy->rsrc_id);
	zend_list_addref(dproxy->rsrc_id);
	
	args[1] = callback->arg;
	Z_ADDREF_P(callback->arg);
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 2, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	zval_ptr_dtor(&(args[1]));

	return;
}
/* }}} */

static void _php_gupnp_device_proxy_available_cb(GUPnPControlPoint *cp, GUPnPDeviceProxy *proxy, gpointer userdata) /* {{{ */
{
	php_gupnp_cpoint_t *cpoint = (php_gupnp_cpoint_t *)userdata;
	TSRMLS_FETCH_FROM_CTX(cpoint ? cpoint->thread_ctx : NULL);
	if (!cpoint || !cpoint->callbacks[GUPNP_SIGNAL_DEVICE_PROXY_AVAILABLE]) {
		return;
	}
	_php_gupnp_device_proxy_cb(cp, proxy, cpoint->callbacks[GUPNP_SIGNAL_DEVICE_PROXY_AVAILABLE] TSRMLS_CC);
	
	return;
}
/* }}} */

static void _php_gupnp_device_proxy_unavailable_cb(GUPnPControlPoint *cp, GUPnPDeviceProxy *proxy, gpointer userdata) /* {{{ */
{
	php_gupnp_cpoint_t *cpoint = (php_gupnp_cpoint_t *)userdata;
	TSRMLS_FETCH_FROM_CTX(cpoint ? cpoint->thread_ctx : NULL);
	if (!cpoint || !cpoint->callbacks[GUPNP_SIGNAL_DEVICE_PROXY_UNAVAILABLE]) {
		return;
	}
	_php_gupnp_device_proxy_cb(cp, proxy, cpoint->callbacks[GUPNP_SIGNAL_DEVICE_PROXY_UNAVAILABLE] TSRMLS_CC);
	
	return;
}
/* }}} */

static void _php_gupnp_service_proxy_notify_cb(GUPnPServiceProxy *proxy, const char *variable, GValue *value, gpointer userdata) /* {{{ */
{
	zval *args[3];
	php_gupnp_service_proxy_t *sproxy = (php_gupnp_service_proxy_t *)userdata;
	php_gupnp_callback_t *callback;
	zval retval;
	TSRMLS_FETCH_FROM_CTX(sproxy ? sproxy->thread_ctx : NULL);
	if (!sproxy || !sproxy->callback) {
		return;
	}
	
	MAKE_STD_ZVAL(args[0]);
	ZVAL_STRING(args[0], (char *)variable, 1); 
	
	MAKE_STD_ZVAL(args[1]);
	switch (G_VALUE_TYPE(value)) {
		case G_TYPE_BOOLEAN: 
			ZVAL_BOOL(args[1], g_value_get_boolean(value));
			break; 
			
		case G_TYPE_INT: 
			ZVAL_LONG(args[1], g_value_get_int(value));
			break; 
			
		case G_TYPE_LONG: 
			ZVAL_LONG(args[1], g_value_get_long(value));
			break; 
			
		case G_TYPE_FLOAT:
			ZVAL_DOUBLE(args[1], g_value_get_float(value));
			break; 
			
		case G_TYPE_DOUBLE:
			ZVAL_DOUBLE(args[1], g_value_get_double(value));
			break; 
			
		case G_TYPE_STRING: 
			ZVAL_STRING(args[1], (char *)g_value_get_string(value), 1);
			break; 
			
		default: 
			ZVAL_NULL(args[1]);
			break; 
	}
	
	callback = sproxy->callback;
	args[2] = callback->arg;
	Z_ADDREF_P(callback->arg);
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 3, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	zval_ptr_dtor(&(args[1]));
	zval_ptr_dtor(&(args[2]));
	
	return;
}
/* }}} */

static void _php_gupnp_subscription_lost_cb(GUPnPServiceProxy *proxy, const GError *error, gpointer userdata) /* {{{ */
{
	zval *args[2];
	php_gupnp_service_proxy_t *sproxy = (php_gupnp_service_proxy_t *)userdata;
	php_gupnp_callback_t *callback;
	zval retval;
	
	TSRMLS_FETCH_FROM_CTX(sproxy ? sproxy->thread_ctx : NULL);

	if (!sproxy || !sproxy->callback_signal) {
		return;
	}
	
	if (error && error->message) {
		MAKE_STD_ZVAL(args[0]);
		ZVAL_STRING(args[0], (char *)error->message, 1); 
	} else {
		ALLOC_INIT_ZVAL(args[0]);
	}
	
	callback = sproxy->callback_signal;
	args[1] = callback->arg;
	Z_ADDREF_P(callback->arg);
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 2, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	zval_ptr_dtor(&(args[1]));
	
	return;
}
/* }}} */

static void _php_gupnp_service_action_invoked_cb(GUPnPService *gupnp_service, GUPnPServiceAction *gupnp_action, gpointer userdata)  /* {{{ */
{
	zval *args[3];
	php_gupnp_callback_t *callback;
	php_gupnp_service_t *service;
	php_gupnp_service_action_t *service_action = (php_gupnp_service_action_t *)userdata;
	zval retval;
	
	TSRMLS_FETCH_FROM_CTX(service_action ? service_action->thread_ctx : NULL);
	
	if (!service_action || !service_action->callback) {
		return;
	}
	
	MAKE_STD_ZVAL(args[0]);
	service = service_action->service;
	service->service = gupnp_service;
	ZVAL_RESOURCE(args[0], service->rsrc_id);
	zend_list_addref(service->rsrc_id);
	
	MAKE_STD_ZVAL(args[1]);
	service_action->action = gupnp_action;
	ZVAL_RESOURCE(args[1], service_action->rsrc_id);
	zend_list_addref(service_action->rsrc_id);
	
	callback = service_action->callback;
	args[2] = callback->arg;
	Z_ADDREF_P(callback->arg);
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 3, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	zval_ptr_dtor(&(args[1]));
	zval_ptr_dtor(&(args[2]));
}
/* }}} */

static void _php_gupnp_service_notify_failed_cb(GUPnPService *gupnp_service, const GList *callback_urls, const GError *error, gpointer userdata)  /* {{{ */
{
	zval *args[3];
	php_gupnp_callback_t *callback;
	php_gupnp_service_t *service;
	php_gupnp_service_action_t *service_action = (php_gupnp_service_action_t *)userdata;
	zval retval;
	
	TSRMLS_FETCH_FROM_CTX(service_action ? service_action->thread_ctx : NULL);
	
	if (!service_action || !service_action->callback) {
		return;
	}
	
	MAKE_STD_ZVAL(args[0]);
	service = service_action->service;
	service->service = gupnp_service;
	ZVAL_RESOURCE(args[0], service->rsrc_id);
	zend_list_addref(service->rsrc_id);
	
	if (error && error->message) {
		MAKE_STD_ZVAL(args[1]);
		ZVAL_STRING(args[1], (char *)error->message, 1); 
	} else {
		ALLOC_INIT_ZVAL(args[1]);
	}
	
	callback = service_action->callback;
	args[2] = callback->arg;
	Z_ADDREF_P(callback->arg);
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 3, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	zval_ptr_dtor(&(args[1]));
	zval_ptr_dtor(&(args[2]));
}
/* }}} */

static void _php_gupnp_service_get_introspection_cb(GUPnPServiceInfo *info, GUPnPServiceIntrospection *introspection, const GError *error, gpointer userdata) /* {{{ */
{
	zval *args[3];
	php_gupnp_service_introspection_t *sintrospection = (php_gupnp_service_introspection_t *)userdata;
	php_gupnp_callback_t *callback;
	zval retval;
	
	TSRMLS_FETCH_FROM_CTX(sintrospection ? sintrospection->thread_ctx : NULL);

	if (!sintrospection || !sintrospection->callback) {
		return;
	}
	
	sintrospection->introspection = introspection;
	sintrospection->rsrc_id = zend_list_insert(sintrospection, le_service_introspection);
	
	MAKE_STD_ZVAL(args[0]);
	ZVAL_RESOURCE(args[0], sintrospection->rsrc_id);
	zend_list_addref(sintrospection->rsrc_id);
	
	if (error && error->message) {
		MAKE_STD_ZVAL(args[1]);
		ZVAL_STRING(args[1], (char *)error->message, 1); 
	} else {
		ALLOC_INIT_ZVAL(args[1]);
	}
	
	callback = sintrospection->callback;
	args[2] = callback->arg;
	Z_ADDREF_P(callback->arg);
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 3, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	zval_ptr_dtor(&(args[1]));
	zval_ptr_dtor(&(args[2]));
	
	return;
}
/* }}} */

char *_php_gupnp_get_value_type_name(GType g_type)  /* {{{ */
{
	switch (g_type) {
		case G_TYPE_BOOLEAN:
			return "GUPNP_TYPE_BOOLEAN";
		case G_TYPE_INT:
			return "GUPNP_TYPE_INT";
		case G_TYPE_LONG:
			return "GUPNP_TYPE_LONG";
		case G_TYPE_FLOAT:
			return "GUPNP_TYPE_FLOAT";
		case G_TYPE_DOUBLE:
			return "GUPNP_TYPE_DOBLE";
		case G_TYPE_STRING:
			return "GUPNP_TYPE_STRING";
		default:
			return "unknown";
	}
}
/* }}} */

zval *_php_gupnp_get_zval_by_gvalue(const GValue *g_value)  /* {{{ */
{
	zval *z_value;
	
	MAKE_STD_ZVAL(z_value);
	switch (G_VALUE_TYPE(g_value)) {
		case G_TYPE_BOOLEAN: 
			ZVAL_BOOL(z_value, g_value_get_boolean(g_value));
			break; 
			
		case G_TYPE_INT:
			ZVAL_LONG(z_value, g_value_get_int(g_value));
			break; 
			
		case G_TYPE_LONG:
			ZVAL_LONG(z_value, g_value_get_long(g_value));
			break; 
			
		case G_TYPE_FLOAT:
			ZVAL_DOUBLE(z_value, g_value_get_float(g_value));
			break; 
			
		case G_TYPE_DOUBLE:
			ZVAL_DOUBLE(z_value, g_value_get_double(g_value));
			break; 
			
		case G_TYPE_STRING: 
			ZVAL_STRING(z_value, (char *)g_value_get_string(g_value), 1);
			break; 
			
		default: 
			ZVAL_NULL(z_value);
			break; 
	}
	return z_value;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(gupnp)
{
	REGISTER_LONG_CONSTANT("GUPNP_TYPE_BOOLEAN", G_TYPE_BOOLEAN,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_TYPE_INT", G_TYPE_INT,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_TYPE_LONG", G_TYPE_LONG,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_TYPE_DOUBLE", G_TYPE_DOUBLE,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_TYPE_FLOAT",  G_TYPE_FLOAT,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_TYPE_STRING", G_TYPE_STRING,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_SIGNAL_DEVICE_PROXY_AVAILABLE", GUPNP_SIGNAL_DEVICE_PROXY_AVAILABLE,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_SIGNAL_DEVICE_PROXY_UNAVAILABLE", GUPNP_SIGNAL_DEVICE_PROXY_UNAVAILABLE,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_SIGNAL_SERVICE_PROXY_AVAILABLE", GUPNP_SIGNAL_SERVICE_PROXY_AVAILABLE,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_SIGNAL_SERVICE_PROXY_UNAVAILABLE", GUPNP_SIGNAL_SERVICE_PROXY_UNAVAILABLE,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_SIGNAL_ACTION_INVOKED", GUPNP_SIGNAL_ACTION_INVOKED,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_SIGNAL_NOTIFY_FAILED", GUPNP_SIGNAL_NOTIFY_FAILED,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_SIGNAL_SUBSCRIPTION_LOST", GUPNP_SIGNAL_SUBSCRIPTION_LOST,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_CONTROL_ERROR_INVALID_ACTION", GUPNP_CONTROL_ERROR_INVALID_ACTION,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_CONTROL_ERROR_INVALID_ARGS", GUPNP_CONTROL_ERROR_INVALID_ARGS,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_CONTROL_ERROR_OUT_OF_SYNC", GUPNP_CONTROL_ERROR_OUT_OF_SYNC,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_CONTROL_ERROR_ACTION_FAILED", GUPNP_CONTROL_ERROR_ACTION_FAILED,  CONST_CS | CONST_PERSISTENT);
	
	le_context = zend_register_list_destructors_ex(_php_gupnp_context_dtor, NULL, "context", module_number);
	le_cpoint = zend_register_list_destructors_ex(_php_gupnp_cpoint_dtor, NULL, "control point", module_number);
	le_rdevice = zend_register_list_destructors_ex(_php_gupnp_rdevice_dtor, NULL, "root device", module_number);
	le_service = zend_register_list_destructors_ex(_php_gupnp_service_dtor, NULL, "service", module_number);
	le_service_proxy = zend_register_list_destructors_ex(_php_gupnp_service_proxy_dtor, NULL, "service proxy", module_number);
	//le_service_proxy_action = zend_register_list_destructors_ex(_php_gupnp_service_proxy_action_dtor, NULL, "service proxy action", module_number);
	le_device_proxy = zend_register_list_destructors_ex(_php_gupnp_device_proxy_dtor, NULL, "device proxy", module_number);
	le_service_info = zend_register_list_destructors_ex(_php_gupnp_service_info_dtor, NULL, "service info", module_number);
	le_service_introspection = zend_register_list_destructors_ex(_php_gupnp_service_introspection_dtor, NULL, "service introspection", module_number);
	le_service_action = zend_register_list_destructors_ex(_php_gupnp_service_action_dtor, NULL, "service action", module_number);
	
	/* Required initialisation */
	g_thread_init(NULL);
	g_type_init();
	
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(gupnp)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(gupnp)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(gupnp)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(gupnp)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "gupnp support", "enabled");
	php_info_print_table_end();
}
/* }}} */

/* {{{ proto resource gupnp_context_new([string host_ip[, int port]])
   Create a new context with the specified host_ip and port. */
PHP_FUNCTION(gupnp_context_new)
{
	char *host_ip = NULL;
	int host_ip_len;
	long port = 0;
	GError *error = NULL;
	php_gupnp_context_t *context = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|sl", &host_ip, &host_ip_len, &port) == FAILURE) {
		return;
	}
	
	context = emalloc(sizeof(php_gupnp_context_t));
	context->context = gupnp_context_new(NULL, host_ip, port, &error);
	context->callback_timeout = NULL;
	TSRMLS_SET_CTX(context->thread_ctx);
	
	if (context->context == NULL) {
		if (error != NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to create context: %s", error->message);
			g_error_free(error);
		}
		efree(context);	
		RETURN_FALSE;
	}
	context->rsrc_id = zend_list_insert(context, le_context);
	
	RETURN_RESOURCE(context->rsrc_id);
}
/* }}} */

/* {{{ proto string gupnp_context_get_host_ip(resource context)
   Get the IP address we advertise ourselves as using. */
PHP_FUNCTION(gupnp_context_get_host_ip)
{
	zval *zcontext;
	php_gupnp_context_t *context = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zcontext) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CONTEXT(zcontext, context);
	RETURN_STRING((char *)gupnp_context_get_host_ip(context->context), 1);
}
/* }}} */

/* {{{ proto int gupnp_context_get_port(resource context)
   Get the port that the SOAP server is running on. */
PHP_FUNCTION(gupnp_context_get_port)
{
	zval *zcontext;
	php_gupnp_context_t *context = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zcontext) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CONTEXT(zcontext, context);
	RETURN_LONG(gupnp_context_get_port(context->context));
}
/* }}} */

/* {{{ proto void gupnp_context_set_subscription_timeout(resource context, int timeout)
   Sets the event subscription timeout (in seconds) to time out. 
   Use 0 if you don't want subscriptions to time out. 
   Note that any client side subscriptions will automatically be renewed. */
PHP_FUNCTION(gupnp_context_set_subscription_timeout)
{
	zval *zcontext;
	long timeout;
	php_gupnp_context_t *context = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl", &zcontext, &timeout) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CONTEXT(zcontext, context);
	gupnp_context_set_subscription_timeout(context->context, (guint)timeout);
}
/* }}} */

/* {{{ proto int gupnp_context_get_subscription_timeout(resource context)
   Get the event subscription timeout (in seconds), or 0 meaning there is no timeout. */
PHP_FUNCTION(gupnp_context_get_subscription_timeout)
{
	zval *zcontext;
	php_gupnp_context_t *context = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zcontext) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CONTEXT(zcontext, context);
	RETURN_LONG(gupnp_context_get_subscription_timeout(context->context));
}
/* }}} */

/* {{{ proto bool gupnp_context_host_path(resource context, string local_path, string server_path)
   Start hosting local_path at server_path. Files with the path local_path.LOCALE (if they exist) 
   will be served up when LOCALE is specified in the request's Accept-Language header. */
PHP_FUNCTION(gupnp_context_host_path)
{
	zval *zcontext;
	char *local_path, *server_path;
	int local_path_len, server_path_len;
	php_gupnp_context_t *context = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rss", &zcontext, 
			&local_path, &local_path_len, &server_path, &server_path_len) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CONTEXT(zcontext, context);
	gupnp_context_host_path(context->context, local_path, server_path);
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_context_unhost_path(resource context, string server_path)
   Stop hosting the file or folder at server_path. */
PHP_FUNCTION(gupnp_context_unhost_path)
{
	zval *zcontext;
	char *server_path;
	int server_path_len;
	php_gupnp_context_t *context = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zcontext, 
			&server_path, &server_path_len) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CONTEXT(zcontext, context);
	gupnp_context_unhost_path(context->context, server_path);
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_timeout_add(resource context, int timeout, mixed callback[, mixed arg]) 
   Sets a function to be called at regular intervals.  */
PHP_FUNCTION(gupnp_context_timeout_add)
{
	zval *zcontext, *zcallback, *zarg = NULL;
	char *func_name;
	int timeout;
	php_gupnp_callback_t *callback, *old_callback;
	php_gupnp_context_t *context;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlz|z", &zcontext, &timeout, &zcallback, &zarg) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CONTEXT(zcontext, context);
	
	if (!zend_is_callable(zcallback, 0, &func_name)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%s' is not a valid callback", func_name);
		efree(func_name);
		RETURN_FALSE;
	}
	efree(func_name);
	
	zval_add_ref(&zcallback);
	if (zarg) {
		zval_add_ref(&zarg);
	} else {
		ALLOC_INIT_ZVAL(zarg);
	}
	
	callback = emalloc(sizeof(php_gupnp_callback_t));
	callback->func = zcallback;
	callback->arg = zarg;
	
	old_callback = context->callback_timeout;
	context->callback_timeout = callback;
	if (old_callback) {
		_php_gupnp_callback_free(old_callback);
	}

	g_timeout_add (timeout, _php_gupnp_context_timeout_cb, context);
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto resource gupnp_root_device_new(resource context, string location)
   Create a new root device, automatically downloading and parsing location. */
PHP_FUNCTION(gupnp_root_device_new)
{
	char *location = NULL;
	int location_len;
	zval *zcontext;
	php_gupnp_rdevice_t *rdevice = NULL;
	php_gupnp_context_t *context = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zcontext, &location, &location_len) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CONTEXT(zcontext, context);
		
	rdevice = emalloc(sizeof(php_gupnp_rdevice_t));
	rdevice->rd = gupnp_root_device_new(context->context, location);
	rdevice->main_loop = g_main_loop_new(NULL, FALSE);
	rdevice->rsrc_id = zend_list_insert(rdevice, le_rdevice);
	
	RETURN_RESOURCE(rdevice->rsrc_id);
}
/* }}} */

/* {{{ proto bool gupnp_root_device_start(resource root_device)
   Start root server's main loop. */
PHP_FUNCTION(gupnp_root_device_start)
{
	zval *zrdevice;
	php_gupnp_rdevice_t *rdevice = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zrdevice) == FAILURE) {
		return;
	}
	
	ZVAL_TO_RDEVICE(zrdevice, rdevice);
	
	g_main_loop_run(rdevice->main_loop);
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_root_device_stop(resource root_device)
   Stop root server's main loop. */
PHP_FUNCTION(gupnp_root_device_stop)
{
	zval *zrdevice;
	php_gupnp_rdevice_t *rdevice = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zrdevice) == FAILURE) {
		return;
	}
	
	ZVAL_TO_RDEVICE(zrdevice, rdevice);
	
	g_main_loop_quit(rdevice->main_loop);
		
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_root_device_set_available(resource root_device, bool available)
   Controls whether or not root_device is available (announcing its presence). */
PHP_FUNCTION(gupnp_root_device_set_available)
{
	gboolean available;
	zval *zrdevice;
	php_gupnp_rdevice_t *rdevice = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rb", &zrdevice, &available) == FAILURE) {
		return;
	}
	
	ZVAL_TO_RDEVICE(zrdevice, rdevice);
	
	gupnp_root_device_set_available(rdevice->rd, available);
		
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_root_device_get_available(resource root_device)
   Get whether or not root_device is available (announcing its presence). */
PHP_FUNCTION(gupnp_root_device_get_available)
{
	gboolean result;
	zval *zrdevice;
	php_gupnp_rdevice_t *rdevice = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zrdevice) == FAILURE) {
		return;
	}
	
	ZVAL_TO_RDEVICE(zrdevice, rdevice);
	
	result = gupnp_root_device_get_available(rdevice->rd);
	
	if (result) {
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto string gupnp_root_device_get_relative_location(resource root_device)
   Get the relative location of root_device. */
PHP_FUNCTION(gupnp_root_device_get_relative_location)
{
	char *location;
	zval *zrdevice;
	php_gupnp_rdevice_t *rdevice = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zrdevice) == FAILURE) {
		return;
	}
	
	ZVAL_TO_RDEVICE(zrdevice, rdevice);
	
	location = (char *)gupnp_root_device_get_relative_location(rdevice->rd);
	
	RETURN_STRING(location, 1);
}
/* }}} */

/* {{{ proto array gupnp_device_info_get(resource root_device)
   Get info of root_device. */
PHP_FUNCTION(gupnp_device_info_get)
{
	zval *zdproxy;
	php_gupnp_device_proxy_t *dproxy;
	char *info_data[2][14];
	int i;
	SoupURI *url_base;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zdproxy) == FAILURE) {
		return;
	}
	
	ZVAL_TO_DEVICE_PROXY(zdproxy, dproxy);
	
	url_base = (SoupURI *)gupnp_device_info_get_url_base(GUPNP_DEVICE_INFO(dproxy->proxy));
	
	info_data[0][0] = "location";
	info_data[1][0] = (char *)gupnp_device_info_get_location(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][1] = "url_base";
	info_data[1][1] = soup_uri_to_string(url_base, 1);
	info_data[0][2] = "udn";
	info_data[1][2] = (char *)gupnp_device_info_get_udn(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][3] = "device_type";
	info_data[1][3] = (char *)gupnp_device_info_get_device_type(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][4] = "friendly_name";
	info_data[1][4] = gupnp_device_info_get_friendly_name(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][5] = "manufacturer";
	info_data[1][5] = gupnp_device_info_get_manufacturer(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][6] = "manufacturer_url";
	info_data[1][6] = gupnp_device_info_get_manufacturer_url(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][7] = "model_description";
	info_data[1][7] = gupnp_device_info_get_model_description(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][8] = "model_name";
	info_data[1][8] = gupnp_device_info_get_model_name(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][9] = "model_number";
	info_data[1][9] = gupnp_device_info_get_model_number(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][10] = "model_url";
	info_data[1][10] = gupnp_device_info_get_model_url(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][11] = "serial_number";
	info_data[1][11] = gupnp_device_info_get_serial_number(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][12] = "presentation_url";
	info_data[1][12] = gupnp_device_info_get_presentation_url(GUPNP_DEVICE_INFO(dproxy->proxy));
	info_data[0][13] = "upc";
	info_data[1][13] = gupnp_device_info_get_upc(GUPNP_DEVICE_INFO(dproxy->proxy));
	
	array_init(return_value);
	
	for (i = 0; i <= 13; i++) {
		if (info_data[1][i]) {
			add_assoc_string(return_value, info_data[0][i], info_data[1][i], 1);
		}
		if ((i >= 4) && (i <= 13)) {
			g_free(info_data[1][i]);
		}
	}
}
/* }}} */

/* {{{ proto resource gupnp_device_info_get_service(resource root_device, string type)
   Get the service with type or false if no such device was found.  */
PHP_FUNCTION(gupnp_device_info_get_service)
{
	zval *zrdevice;
	php_gupnp_rdevice_t *rdevice = NULL;
	char *type;
	int type_len;
	php_gupnp_service_info_t *service;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zrdevice, &type, &type_len) == FAILURE) {
		return;
	}
	
	ZVAL_TO_RDEVICE(zrdevice, rdevice);
	
	service = emalloc(sizeof(php_gupnp_service_info_t));
	service->service_info = gupnp_device_info_get_service(GUPNP_DEVICE_INFO(rdevice->rd), type);
	service->callback = NULL;
	TSRMLS_SET_CTX(service->thread_ctx);
	if (service->service_info != NULL) {
		service->rsrc_id = zend_list_insert(service, le_service_info);
		RETURN_RESOURCE(service->rsrc_id);
	}
	efree(service);
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool gupnp_device_action_callback_set(resource root_device, int signal, string action_name, mixed callback[, mixed arg]) 
   Set device callback function for signal and action.  */
PHP_FUNCTION(gupnp_device_action_callback_set)
{
	zval *zservice_info, *zcallback, *zarg = NULL;
	char *func_name, *action_name, *action_name_full, *signal_name;
	int action_name_len;
	int signal;
	php_gupnp_callback_t *callback;
	php_gupnp_service_t *service;
	php_gupnp_service_info_t *service_info;
	php_gupnp_service_action_t *service_action;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlsz|z", &zservice_info, &signal, &action_name, &action_name_len, &zcallback, &zarg) == FAILURE) {
		return;
	}
	
	switch (signal) {
		case GUPNP_SIGNAL_ACTION_INVOKED:
			signal_name = "action-invoked::";
			break;
			
		case GUPNP_SIGNAL_NOTIFY_FAILED:
			signal_name = "notify-failed::";
			break;
			
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Not valid signal");
			RETURN_FALSE;
			break;
	}
		
	ZVAL_TO_SERVICE_INFO(zservice_info, service_info);
	
	if (!zend_is_callable(zcallback, 0, &func_name)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%s' is not a valid callback", func_name);
		efree(func_name);
		RETURN_FALSE;
	}
	efree(func_name);
	
	zval_add_ref(&zcallback);
	if (zarg) {
		zval_add_ref(&zarg);
	} else {
		ALLOC_INIT_ZVAL(zarg);
	}
	
	callback = emalloc(sizeof(php_gupnp_callback_t));
	callback->func = zcallback;
	callback->arg = zarg;
	
	service = emalloc(sizeof(php_gupnp_service_t));
	service->service = NULL;
	service->rsrc_id = zend_list_insert(service, le_service);
	
	service_action = emalloc(sizeof(php_gupnp_service_action_t));
	service_action->action = NULL;
	service_action->service = service;
	service_action->callback = callback;
	service_action->rsrc_id = zend_list_insert(service_action, le_service_action);
	TSRMLS_SET_CTX(service_action->thread_ctx);
	
	action_name_full = emalloc(strlen(signal_name) + action_name_len);
	strcpy(action_name_full, signal_name);
	strcat(action_name_full, action_name);
	
	switch (signal) {
		case GUPNP_SIGNAL_ACTION_INVOKED:
			g_signal_connect(service_info->service_info, action_name_full, G_CALLBACK(_php_gupnp_service_action_invoked_cb), service_action);
			break;
			
		case GUPNP_SIGNAL_NOTIFY_FAILED:
			g_signal_connect(service_info->service_info, action_name_full, G_CALLBACK(_php_gupnp_service_notify_failed_cb), service_action);
			break;
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto resource gupnp_control_point_new(resource context, string target)
   Create a new Control point with the specified target */
PHP_FUNCTION(gupnp_control_point_new)
{
	char *target = NULL;
	int target_len, i;
	zval *zcontext;
	php_gupnp_cpoint_t *cpoint = NULL;
	php_gupnp_context_t *context = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zcontext, &target, &target_len) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CONTEXT(zcontext, context);
		
	cpoint = emalloc(sizeof(php_gupnp_cpoint_t));
	cpoint->cp = gupnp_control_point_new(context->context, target);
	cpoint->main_loop = g_main_loop_new(NULL, FALSE);
	TSRMLS_SET_CTX(cpoint->thread_ctx);
	
	for (i = 0; i < 4; i++) {
		cpoint->callbacks[i] = NULL;
	}
	
	cpoint->rsrc_id = zend_list_insert(cpoint, le_cpoint);
	
	RETURN_RESOURCE(cpoint->rsrc_id);
}
/* }}} */

/* {{{ proto bool gupnp_browse_service(resource cpoint, int signal, mixed callback[, mixed arg])
   Set control point callback function for signal. */
PHP_FUNCTION(gupnp_control_point_callback_set)
{
	zval *zcpoint, *zcallback, *zarg = NULL;
	char *func_name;
	int signal;
	php_gupnp_callback_t *callback, *old_callback = NULL;
	php_gupnp_cpoint_t *cpoint = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlz|z", &zcpoint, &signal, &zcallback, &zarg) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CPOINT(zcpoint, cpoint);
	
	if (!zend_is_callable(zcallback, 0, &func_name)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%s' is not a valid callback", func_name);
		efree(func_name);
		RETURN_FALSE;
	}
	efree(func_name);
	
	zval_add_ref(&zcallback);
	if (zarg) {
		zval_add_ref(&zarg);
	} else {
		ALLOC_INIT_ZVAL(zarg);
	}
	
	if (signal != GUPNP_SIGNAL_DEVICE_PROXY_AVAILABLE
		&& signal != GUPNP_SIGNAL_DEVICE_PROXY_UNAVAILABLE
		&& signal != GUPNP_SIGNAL_SERVICE_PROXY_AVAILABLE
		&& signal != GUPNP_SIGNAL_SERVICE_PROXY_UNAVAILABLE) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%d' is not a valid signal", signal);
		RETURN_FALSE;
	}
	
	callback = emalloc(sizeof(php_gupnp_callback_t));
	callback->func = zcallback;
	callback->arg = zarg;
	
	old_callback = cpoint->callbacks[signal];
	cpoint->callbacks[signal] = callback;
	
	if (signal == GUPNP_SIGNAL_DEVICE_PROXY_AVAILABLE) {
		g_signal_connect(cpoint->cp, "device-proxy-available", 
			G_CALLBACK(_php_gupnp_device_proxy_available_cb), cpoint);
	} else if (signal == GUPNP_SIGNAL_DEVICE_PROXY_UNAVAILABLE) {
		g_signal_connect(cpoint->cp, "device-proxy-unavailable", 
			G_CALLBACK(_php_gupnp_device_proxy_unavailable_cb), cpoint);
	} else if (signal == GUPNP_SIGNAL_SERVICE_PROXY_AVAILABLE) {
		g_signal_connect(cpoint->cp, "service-proxy-available", 
			G_CALLBACK(_php_gupnp_service_proxy_available_cb), cpoint);
	} else if (signal == GUPNP_SIGNAL_SERVICE_PROXY_UNAVAILABLE) {
		g_signal_connect(cpoint->cp, "service-proxy-unavailable", 
			G_CALLBACK(_php_gupnp_service_proxy_unavailable_cb), cpoint);
	}
	
	if (old_callback) {
		_php_gupnp_callback_free(old_callback);
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_control_point_browse_start(resource cpoint)
   Starts the search and calls user-defined callback. */
PHP_FUNCTION(gupnp_control_point_browse_start)
{
	zval *zcpoint;
	php_gupnp_cpoint_t *cpoint = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zcpoint) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CPOINT(zcpoint, cpoint);
	
	gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(cpoint->cp), TRUE);
  
	g_main_loop_run(cpoint->main_loop);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_control_point_browse_stop(resource cpoint)
   Stop the search and calls user-defined callback. */
PHP_FUNCTION(gupnp_control_point_browse_stop)
{
	zval *zcpoint;
	php_gupnp_cpoint_t *cpoint = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zcpoint) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CPOINT(zcpoint, cpoint);
	
	g_main_loop_quit(cpoint->main_loop);
  
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto array gupnp_service_info_get(resource proxy)
   Get full info of service. */
PHP_FUNCTION(gupnp_service_info_get)
{
	zval *zproxy;
	php_gupnp_service_proxy_t *sproxy;
	char *info_data[2][8];
	int i;
	SoupURI *url_base;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zproxy) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
	
	url_base = (SoupURI *)gupnp_service_info_get_url_base(GUPNP_SERVICE_INFO(sproxy->proxy));
	
	info_data[0][0] = "location";
	info_data[1][0] = (char *)gupnp_service_info_get_location(GUPNP_SERVICE_INFO(sproxy->proxy));
	info_data[0][1] = "url_base";
	info_data[1][1] = soup_uri_to_string(url_base, 1);
	info_data[0][2] = "udn";
	info_data[1][2] = (char *)gupnp_service_info_get_udn(GUPNP_SERVICE_INFO(sproxy->proxy));
	info_data[0][3] = "service_type";
	info_data[1][3] = (char *)gupnp_service_info_get_service_type(GUPNP_SERVICE_INFO(sproxy->proxy));
	info_data[0][4] = "id";
	info_data[1][4] = gupnp_service_info_get_id(GUPNP_SERVICE_INFO(sproxy->proxy));
	info_data[0][5] = "scpd_url";
	info_data[1][5] = gupnp_service_info_get_scpd_url(GUPNP_SERVICE_INFO(sproxy->proxy));
	info_data[0][6] = "control_url";
	info_data[1][6] = gupnp_service_info_get_control_url(GUPNP_SERVICE_INFO(sproxy->proxy));
	info_data[0][7] = "event_subscription_url";
	info_data[1][7] = gupnp_service_info_get_event_subscription_url(GUPNP_SERVICE_INFO(sproxy->proxy));
	
	array_init(return_value);
	
	for (i = 0; i <= 7; i++) {
		if (info_data[1][i]) {
			add_assoc_string(return_value, info_data[0][i], info_data[1][i], 1);
		}
		if ((i >= 4) && (i <= 7)) {
			g_free(info_data[1][i]);
		}
	}
}
/* }}} */

/* {{{ proto mixed gupnp_service_info_get_introspection(resource proxy[, mixed callback[, mixed arg]])
   Get resource introspection of service or register callback if corresponding parameter was passed. */
PHP_FUNCTION(gupnp_service_info_get_introspection)
{
	zval *zproxy, *zcallback = NULL, *zarg = NULL;
	char *func_name;
	php_gupnp_callback_t *callback;
	php_gupnp_service_proxy_t *sproxy;
	GUPnPServiceIntrospection *introspection;
	php_gupnp_service_introspection_t *sintrospection;
	GError *error = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r|zz", &zproxy, &zcallback, &zarg) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
	
	sintrospection = emalloc(sizeof(php_gupnp_service_introspection_t));
	TSRMLS_SET_CTX(sintrospection->thread_ctx);
	
	if (zcallback) {
		if (!zend_is_callable(zcallback, 0, &func_name)) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%s' is not a valid callback", func_name);
			efree(func_name);
			RETURN_FALSE;
		}
		efree(func_name);
		
		zval_add_ref(&zcallback);
		if (zarg) {
			zval_add_ref(&zarg);
		} else {
			ALLOC_INIT_ZVAL(zarg);
		}
		
		callback = emalloc(sizeof(php_gupnp_callback_t));
		callback->func = zcallback;
		callback->arg = zarg;
		
		sintrospection->callback = callback;
		
		gupnp_service_info_get_introspection_async(GUPNP_SERVICE_INFO(sproxy->proxy), 
			_php_gupnp_service_get_introspection_cb, sintrospection);
		
		RETURN_TRUE;
	} else {
		introspection = gupnp_service_info_get_introspection(GUPNP_SERVICE_INFO(sproxy->proxy), &error);
		if (introspection == NULL) {
			if (error != NULL) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "%s", error->message);
				g_error_free(error);
			}
			RETURN_FALSE;
		}
		sintrospection->callback = NULL;
		sintrospection->introspection = introspection;
		sintrospection->rsrc_id = zend_list_insert(sintrospection, le_service_introspection);
		
		RETURN_RESOURCE(sintrospection->rsrc_id);
	}
}
/* }}} */

/* {{{ proto array gupnp_service_introspection_get_state_variable(resource introspection, string variable_name)
   Returns the state variable data by the name variable_name in this service. */
PHP_FUNCTION(gupnp_service_introspection_get_state_variable)
{
	zval *zintrospection, *z_default_value, *z_minimum, *z_maximum, *z_step;
	php_gupnp_service_introspection_t *sintrospection;
	char *variable_name;
	long variable_name_len;
	const GUPnPServiceStateVariableInfo *variable;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zintrospection, &variable_name, &variable_name_len) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_INTROSPECTION(zintrospection, sintrospection);
	
	variable = gupnp_service_introspection_get_state_variable(sintrospection->introspection, variable_name);
	
	if (!variable) {
		RETURN_FALSE;
	}
	
	array_init(return_value);
	add_assoc_string(return_value, "name", variable->name, 1);
	add_assoc_bool(return_value, "send_events", variable->send_events);
	add_assoc_bool(return_value, "is_numeric", variable->is_numeric);
	add_assoc_string(return_value, "type", _php_gupnp_get_value_type_name(variable->type), 1);
	
	switch (variable->type) {
		case G_TYPE_BOOLEAN:
			z_default_value = _php_gupnp_get_zval_by_gvalue(&(variable->default_value));
			add_assoc_bool(return_value, "default_value", Z_BVAL_P(z_default_value));
			break;
			
		case G_TYPE_INT:
		case G_TYPE_LONG:
			z_default_value = _php_gupnp_get_zval_by_gvalue(&(variable->default_value));
			z_minimum 		= _php_gupnp_get_zval_by_gvalue(&(variable->minimum));
			z_maximum 		= _php_gupnp_get_zval_by_gvalue(&(variable->maximum));
			z_step 			= _php_gupnp_get_zval_by_gvalue(&(variable->step));
			add_assoc_long(return_value, "default_value", Z_LVAL_P(z_default_value));
			add_assoc_long(return_value, "minimum", Z_LVAL_P(z_minimum));
			add_assoc_long(return_value, "maximum", Z_LVAL_P(z_maximum));
			add_assoc_long(return_value, "step", Z_LVAL_P(z_step));
			break;
			
		case G_TYPE_FLOAT:
		case G_TYPE_DOUBLE:
			z_default_value = _php_gupnp_get_zval_by_gvalue(&(variable->default_value));
			z_minimum 		= _php_gupnp_get_zval_by_gvalue(&(variable->minimum));
			z_maximum 		= _php_gupnp_get_zval_by_gvalue(&(variable->maximum));
			z_step 			= _php_gupnp_get_zval_by_gvalue(&(variable->step));
			add_assoc_double(return_value, "default_value", Z_DVAL_P(z_default_value));
			add_assoc_double(return_value, "minimum", Z_DVAL_P(z_minimum));
			add_assoc_double(return_value, "maximum", Z_DVAL_P(z_maximum));
			add_assoc_double(return_value, "step", Z_DVAL_P(z_step));
			break;
			
		case G_TYPE_STRING:
			z_default_value = _php_gupnp_get_zval_by_gvalue(&(variable->default_value));
			add_assoc_string(return_value, "default_value", Z_STRVAL_P(z_default_value), 1);
			break;
	}
	
}
/* }}} */

/* {{{ proto bool gupnp_service_proxy_action_set(resource proxy, string action, string name, mixed value, int type)
   Send action with parameters to the service exposed by proxy synchronously and set value. */
PHP_FUNCTION(gupnp_service_proxy_action_set)
{
	zval *zproxy, *param_val;
	char *action, *param_name;
	int action_len, param_name_len;
	long param_type;
	php_gupnp_service_proxy_t *sproxy;
	GError *error = NULL;
	gboolean result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsszl", 
			&zproxy, &action, &action_len, &param_name, &param_name_len, 
			&param_val, &param_type) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
	
	switch (param_type) {
		case G_TYPE_BOOLEAN:
			if (Z_TYPE_P(param_val) == IS_BOOL) {
				result = gupnp_service_proxy_send_action(sproxy->proxy, action, 
							&error, param_name, param_type, Z_BVAL_P(param_val), NULL, NULL);
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'value' is not boolean");
				return;
			}
			break; 
		
		case G_TYPE_INT:
		case G_TYPE_LONG:
			if (Z_TYPE_P(param_val) == IS_LONG) {
				result = gupnp_service_proxy_send_action(sproxy->proxy, action, 
							&error, param_name, param_type, Z_LVAL_P(param_val), NULL, NULL);
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'value' is not integer");
				return;
			}
			break; 
		
		case G_TYPE_FLOAT: 
		case G_TYPE_DOUBLE: 
			if (Z_TYPE_P(param_val) == IS_DOUBLE) {
				result = gupnp_service_proxy_send_action(sproxy->proxy, action, 
							&error, param_name, param_type, Z_DVAL_P(param_val), NULL, NULL);
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'value' is not float");
				return;
			}
			break; 
			
		case G_TYPE_STRING: 
			if (Z_TYPE_P(param_val) == IS_STRING) {
				result = gupnp_service_proxy_send_action(sproxy->proxy, action, 
							&error, param_name, param_type, Z_STRVAL_P(param_val), NULL, NULL);
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'value' is not string");
				return;
			}
			break; 

		default: 
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "'type' is not correctly defined");
			return;
	}
	
	if (result) {
		RETURN_TRUE;
	} else {
		if (error != NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to send action: %s", error->message);
			g_error_free(error);
		}
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto mixed gupnp_service_proxy_action_set(resource proxy, string action, string name, int type)
   Send action with parameters to the service exposed by proxy synchronously and get value. */
PHP_FUNCTION(gupnp_service_proxy_action_get)
{
	zval *zproxy;
	char *action, *param_name;
	int action_len, param_name_len;
	long param_type;
	php_gupnp_service_proxy_t *sproxy;
	GError *error = NULL;
	gboolean result = 0;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rssl", 
			&zproxy, &action, &action_len, &param_name, &param_name_len, 
			&param_type) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
	
	switch (param_type) {
		case G_TYPE_BOOLEAN:
		{
			gboolean value_gboolean = 0;
			result = gupnp_service_proxy_send_action(sproxy->proxy, action, 
							&error, NULL, param_name, param_type, &value_gboolean, NULL);
			if (result) {
				RETURN_BOOL(value_gboolean);
			}
			break; 
		}
		
		case G_TYPE_INT:
		case G_TYPE_LONG:
		{
			glong value_glong = 0;
			result = gupnp_service_proxy_send_action(sproxy->proxy, action, 
							&error, NULL, param_name, param_type, &value_glong, NULL);
			if (result) {
				RETURN_LONG(value_glong);
			}
			break; 
		}
		
		case G_TYPE_FLOAT:
		case G_TYPE_DOUBLE: 
		{
			gdouble value_gdouble = 0;
			result = gupnp_service_proxy_send_action(sproxy->proxy, action, 
							&error, NULL, param_name, param_type, &value_gdouble, NULL);
			if (result) {
				RETURN_DOUBLE(value_gdouble);
			}
			break; 
		}
			
		case G_TYPE_STRING: 
		{
			gchar *value_gchar = NULL;
			result = gupnp_service_proxy_send_action(sproxy->proxy, action, 
							&error, NULL, param_name, param_type, value_gchar, NULL);
			if (result) {
				RETURN_STRING((char *)value_gchar, 1);
			}
			break; 
		}
			
		default: 
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "'type' is not correctly defined");
			return;
	}
	
	if (error != NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to send action: %s", error->message);
		g_error_free(error);
	}
	RETURN_FALSE;
}
/* }}} */

static GValue *_gupnp_get_new_gvalue(long g_type)
{
	GValue *g_value;
	
	g_value = (GValue *)ecalloc(1, sizeof(GValue));
	//printf("g_value: %p\n", g_value);
	
	g_value_init(g_value, g_type);
	
	return g_value;
}

PHP_FUNCTION(gupnp_service_proxy_send_action_tmp)
{
	zval *z_in_params, *z_out_params;
	zval **entry;
	zval **z_name, **z_type, **z_value = NULL;
	int entry_size, i = 0;
	
	zval *z_proxy;
	php_gupnp_service_proxy_t *sproxy;
	GError *error = NULL;
	
	GHashTable *in_hash, *out_hash;
	GValue *g_in_value;
	GValue *g_out_value;
	

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "raa/", &z_proxy, &z_in_params, &z_out_params) == FAILURE) {
		return;
	}

	printf("[func] gupnp_service_proxy_send_action_tmp\n");
	
	ZVAL_TO_SERVICE_PROXY(z_proxy, sproxy);
	
	in_hash = g_hash_table_new(NULL, NULL);
	out_hash = g_hash_table_new(NULL, NULL);

	for (zend_hash_internal_pointer_reset(Z_ARRVAL_P(z_in_params)), i = 0;
		 zend_hash_get_current_data(Z_ARRVAL_P(z_in_params), (void **)&entry) == SUCCESS;
		 zend_hash_move_forward(Z_ARRVAL_P(z_in_params)), i++) {

		if (Z_TYPE_PP(entry) != IS_ARRAY) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "in_params list entry #%d is not an array", i+1);
			continue;
		}

		entry_size = zend_hash_num_elements(Z_ARRVAL_PP(entry));

		if (entry_size > 1) {
			zend_hash_internal_pointer_reset(Z_ARRVAL_PP(entry));

			/* Check that we have a host */
			if (zend_hash_get_current_data(Z_ARRVAL_PP(entry), (void **)&z_name) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not get server host for entry #%d", i+1);
				continue;
			}

			/* Check that we have a port */
			if (zend_hash_move_forward(Z_ARRVAL_PP(entry)) == FAILURE ||
				zend_hash_get_current_data(Z_ARRVAL_PP(entry), (void **)&z_type) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not get server port for entry #%d", i+1);
				continue;
			}
			
			convert_to_string_ex(z_name);
			convert_to_long_ex(z_type);

			/* Try to get value */
			if (zend_hash_move_forward(Z_ARRVAL_PP(entry)) == FAILURE ||
				zend_hash_get_current_data(Z_ARRVAL_PP(entry), (void **)&z_value) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not get server value for entry #%d", i+1);
				continue;
			}
			
			switch (Z_LVAL_PP(z_type)) {
				/*
				case G_TYPE_BOOLEAN:
					if (Z_TYPE_P(param_val) == IS_BOOL) {
						result = gupnp_service_proxy_send_action(sproxy->proxy, action, 
									&error, param_name, param_type, Z_BVAL_P(param_val), NULL, NULL);
					} else {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "'value' is not boolean");
						return;
					}
					break; 
				
				
				case G_TYPE_INT:
				case G_TYPE_LONG:
					if (Z_TYPE_P(param_val) == IS_LONG) {
						result = gupnp_service_proxy_send_action(sproxy->proxy, action, 
									&error, param_name, param_type, Z_LVAL_P(param_val), NULL, NULL);
					} else {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "'value' is not integer");
						return;
					}
					break; 
				
				case G_TYPE_FLOAT: 
				case G_TYPE_DOUBLE: 
					if (Z_TYPE_P(param_val) == IS_DOUBLE) {
						result = gupnp_service_proxy_send_action(sproxy->proxy, action, 
									&error, param_name, param_type, Z_DVAL_P(param_val), NULL, NULL);
					} else {
						php_error_docref(NULL TSRMLS_CC, E_WARNING, "'value' is not float");
						return;
					}
					break; 
				*/	
				
				case G_TYPE_INT:
				case G_TYPE_LONG:
				{
					g_in_value = _gupnp_get_new_gvalue(G_TYPE_LONG);
					//printf("g_in_value: %p\n", g_in_value);
					convert_to_long_ex(z_value);
					g_value_set_long(g_in_value, Z_LVAL_PP(z_value));
					g_hash_table_insert(in_hash, Z_STRVAL_PP(z_name), g_in_value);
					
					printf("z_name: %s, z_type: %ld, value: %d\n", 
						Z_STRVAL_PP(z_name), Z_LVAL_PP(z_type), Z_LVAL_PP(z_value));
					break; 
				}
				
				case G_TYPE_STRING: 
				{
					g_in_value = _gupnp_get_new_gvalue(G_TYPE_STRING);
					//printf("g_in_value: %p\n", g_in_value);
					convert_to_string_ex(z_value);
					g_value_set_string(g_in_value, Z_STRVAL_PP(z_value));
					g_hash_table_insert(in_hash, Z_STRVAL_PP(z_name), g_in_value);
					
					printf("z_name: %s, z_type: %ld, value: %s\n", 
						Z_STRVAL_PP(z_name), Z_LVAL_PP(z_type), Z_STRVAL_PP(z_value));
					break; 
				}
				
				default: 
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "'type' is not correctly defined");
					continue;
			}
			
			//convert_to_long_ex(z_value);
			//value = Z_LVAL_PP(z_value);
			
			
			

			/*list = memcached_server_list_append_with_value(list, Z_STRVAL_PP(z_name),
															Z_LVAL_PP(z_type), value, &status);*/
			
	
			//g_value_init(g_value_in, G_TYPE_STRING);
			
	
			/*if (php_memc_handle_error(status TSRMLS_CC) == 0) {
				continue;
			}*/
		}

		/* catch-all for all errors */
		//php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not add entry #%d to the server list", i+1);
	}
	
	for (zend_hash_internal_pointer_reset(Z_ARRVAL_P(z_out_params)), i = 0;
		 zend_hash_get_current_data(Z_ARRVAL_P(z_out_params), (void **)&entry) == SUCCESS;
		 zend_hash_move_forward(Z_ARRVAL_P(z_out_params)), i++) {

		if (Z_TYPE_PP(entry) != IS_ARRAY) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "out_params list entry #%d is not an array", i+1);
			continue;
		}

		entry_size = zend_hash_num_elements(Z_ARRVAL_PP(entry));

		if (entry_size > 1) {
			zend_hash_internal_pointer_reset(Z_ARRVAL_PP(entry));

			/* Check that we have a host */
			if (zend_hash_get_current_data(Z_ARRVAL_PP(entry), (void **)&z_name) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not get server host for entry #%d", i+1);
				continue;
			}

			/* Check that we have a port */
			if (zend_hash_move_forward(Z_ARRVAL_PP(entry)) == FAILURE ||
				zend_hash_get_current_data(Z_ARRVAL_PP(entry), (void **)&z_type) == FAILURE) {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not get server port for entry #%d", i+1);
				continue;
			}
			
			convert_to_string_ex(z_name);
			convert_to_long_ex(z_type);

			switch (Z_LVAL_PP(z_type)) {
				case G_TYPE_INT:
				case G_TYPE_LONG:
				{
					g_out_value = _gupnp_get_new_gvalue(G_TYPE_LONG);
					g_hash_table_insert(out_hash, Z_STRVAL_PP(z_name), g_out_value);
					printf("z_name: %s, z_type: %ld\n", 
						Z_STRVAL_PP(z_name), Z_LVAL_PP(z_type));
					break; 
				}
				
				case G_TYPE_STRING: 
				{
					g_out_value = _gupnp_get_new_gvalue(G_TYPE_STRING);
					g_hash_table_insert(out_hash, Z_STRVAL_PP(z_name), g_out_value);
					
					printf("z_name: %s, z_type: %ld\n", 
						Z_STRVAL_PP(z_name), Z_LVAL_PP(z_type));
					break; 
				}
				
				default: 
					php_error_docref(NULL TSRMLS_CC, E_WARNING, "'type' is not correctly defined");
					continue;
			}
		}

		/* catch-all for all errors */
		//php_error_docref(NULL TSRMLS_CC, E_WARNING, "could not add entry #%d to the server list", i+1);
	}

	//status = memcached_server_push(i_obj->memc, list);
	//memcached_server_list_free(list);
	//if (php_memc_handle_error(status TSRMLS_CC) < 0) {
	//	RETURN_FALSE;
	//}
	
	//g_value_init(&g_value_out_1, G_TYPE_STRING);
	//g_hash_table_insert(out_hash, "Result", &g_value_out_1);
	
	gupnp_service_proxy_send_action_hash(sproxy->proxy, "Browse", 
		&error, in_hash, out_hash);
	
	//printf("Result: %s\n", (char *)g_value_get_string(g_out_value));
	
	g_hash_table_destroy(in_hash);
	g_hash_table_destroy(out_hash);
	
	if (error != NULL) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to send action: %s", error->message);
		g_error_free(error);
		RETURN_FALSE;
	}
	
	array_init(return_value);
	add_assoc_string(return_value, "Result", g_value_get_string(g_out_value), 1);
	
	//RETURN_TRUE;
}


/* {{{ proto bool gupnp_service_proxy_set_subscribed(resource proxy, boolean subscribed)
   (Un)subscribes to the service. */
PHP_FUNCTION(gupnp_service_proxy_set_subscribed)
{
	zval *zproxy;
	zend_bool zsubscribed;
	php_gupnp_service_proxy_t *sproxy;
	gboolean subscribed = FALSE;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rb", &zproxy, &zsubscribed) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
		
	if (zsubscribed) { 
		subscribed = TRUE;
	}
	
	gupnp_service_proxy_set_subscribed(sproxy->proxy, subscribed);
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_service_proxy_get_subscribed(resource proxy)
   Check whether subscription is valid to the service. */
PHP_FUNCTION(gupnp_service_proxy_get_subscribed)
{
	zval *zproxy;
	php_gupnp_service_proxy_t *sproxy;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zproxy) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
		
	if (gupnp_service_proxy_get_subscribed(sproxy->proxy)) { 
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto bool gupnp_service_proxy_add_notify(resource proxy, string value, int type, mixed callback[, mixed arg])
   Sets up callback to be called whenever a change notification for variable is recieved. */
PHP_FUNCTION(gupnp_service_proxy_add_notify)
{
	zval *zproxy, *zcallback, *zarg = NULL;
	char *param_val, *func_name;
	int param_val_len;
	long param_type;
	php_gupnp_callback_t *callback, *old_callback;
	php_gupnp_service_proxy_t *sproxy;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rslz|z", &zproxy, &param_val, &param_val_len, &param_type, &zcallback, &zarg) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
		
	if (!zend_is_callable(zcallback, 0, &func_name)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%s' is not a valid callback", func_name);
		efree(func_name);
		RETURN_FALSE;
	}
	efree(func_name);
	
	zval_add_ref(&zcallback);
	if (zarg) {
		zval_add_ref(&zarg);
	} else {
		ALLOC_INIT_ZVAL(zarg);
	}
	
	callback = emalloc(sizeof(php_gupnp_callback_t));
	callback->func = zcallback;
	callback->arg = zarg;
	
	old_callback = sproxy->callback;
	sproxy->callback = callback;
	
	if (old_callback) {
		_php_gupnp_callback_free(old_callback);
	}
	
	if (!gupnp_service_proxy_add_notify(sproxy->proxy, param_val, param_type, _php_gupnp_service_proxy_notify_cb, sproxy)) {
    	RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_service_proxy_remove_notify(resource proxy, string value)
   Cancels the variable change notification. */
PHP_FUNCTION(gupnp_service_proxy_remove_notify)
{
	zval *zproxy;
	char *param_val;
	int param_val_len;
	php_gupnp_service_proxy_t *sproxy;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zproxy, &param_val, &param_val_len) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
		
	if (!gupnp_service_proxy_remove_notify(sproxy->proxy, param_val, _php_gupnp_service_proxy_notify_cb, sproxy)) {
    	RETURN_FALSE;
	}
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_browse_service(resource proxy, int signal, mixed callback[, mixed arg])
   Sets service proxy callback for signal. */
PHP_FUNCTION(gupnp_service_proxy_callback_set)
{
	zval *zproxy, *zcallback, *zarg = NULL;
	char *func_name;
	int signal;
	php_gupnp_callback_t *callback, *old_callback = NULL;
	php_gupnp_service_proxy_t *sproxy;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rlz|z", &zproxy, &signal, &zcallback, &zarg) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
	
	if (!zend_is_callable(zcallback, 0, &func_name)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%s' is not a valid callback", func_name);
		efree(func_name);
		RETURN_FALSE;
	}
	efree(func_name);
	
	zval_add_ref(&zcallback);
	if (zarg) {
		zval_add_ref(&zarg);
	} else {
		ALLOC_INIT_ZVAL(zarg);
	}
	
	if (signal != GUPNP_SIGNAL_SUBSCRIPTION_LOST) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%d' is not a valid signal", signal);
		RETURN_FALSE;
	}
	
	callback = emalloc(sizeof(php_gupnp_callback_t));
	callback->func = zcallback;
	callback->arg = zarg;
	
	old_callback = sproxy->callback_signal;
	sproxy->callback_signal = callback;
	
	g_signal_connect(sproxy->proxy, "subscription-lost", 
		G_CALLBACK(_php_gupnp_subscription_lost_cb), sproxy);
	
	if (old_callback) {
		_php_gupnp_callback_free(old_callback);
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_service_action_set(resource action, string name, int type, mixed value)
   Sets the specified action return values. */
PHP_FUNCTION(gupnp_service_action_set)
{
	zval *zaction, *param_val;
	char *param_name;
	int param_name_len;
	long param_type;
	php_gupnp_service_action_t *service_action;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rslz", 
			&zaction, &param_name, &param_name_len, &param_type, &param_val) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_ACTION(zaction, service_action);
	
	switch (param_type) {
		case G_TYPE_BOOLEAN:
			if (Z_TYPE_P(param_val) == IS_BOOL) {
				gupnp_service_action_set(service_action->action, param_name, param_type, Z_BVAL_P(param_val), NULL);
				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be boolean");
				return;
			}
			break; 
		
		case G_TYPE_INT:
		case G_TYPE_LONG:
			if (Z_TYPE_P(param_val) == IS_LONG) {
				gupnp_service_action_set(service_action->action, param_name, param_type, Z_LVAL_P(param_val), NULL);
				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be integer");
				return;
			}
			break; 
		
		case G_TYPE_FLOAT: 
		case G_TYPE_DOUBLE: 
			if (Z_TYPE_P(param_val) == IS_DOUBLE) {
				gupnp_service_action_set(service_action->action, param_name, param_type, Z_DVAL_P(param_val), NULL);
				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be float");
				return;
			}
			break; 
			
		case G_TYPE_STRING: 
			if (Z_TYPE_P(param_val) == IS_STRING) {
				gupnp_service_action_set(service_action->action, param_name, param_type, Z_STRVAL_P(param_val), NULL);
				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be string");
				return;
			}
			break; 

		default: 
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_type' is not correctly defined");
			return;
	}
}
/* }}} */

/* {{{ proto mixed gupnp_service_action_get(resource action, string name, int type)
   Retrieves the specified action arguments. */
PHP_FUNCTION(gupnp_service_action_get)
{
	zval *zaction;
	char *param_name;
	int param_name_len;
	long param_type;
	php_gupnp_service_action_t *service_action;
	
	GValue g_value = {0};
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsl", 
			&zaction, &param_name, &param_name_len, &param_type) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_ACTION(zaction, service_action);
	
	switch (param_type) {
		case G_TYPE_BOOLEAN:
			g_value_init(&g_value, G_TYPE_BOOLEAN);
			gupnp_service_action_get_value(service_action->action, param_name, &g_value);
			RETURN_BOOL(g_value_get_boolean(&g_value));
		
		case G_TYPE_INT:
		case G_TYPE_LONG:
			g_value_init(&g_value, G_TYPE_LONG);
			gupnp_service_action_get_value(service_action->action, param_name, &g_value);
			RETURN_LONG(g_value_get_long(&g_value));
		
		case G_TYPE_FLOAT:
		case G_TYPE_DOUBLE: 
			g_value_init(&g_value, G_TYPE_DOUBLE);
			gupnp_service_action_get_value(service_action->action, param_name, &g_value);
			RETURN_DOUBLE(g_value_get_double(&g_value));
			
		case G_TYPE_STRING: 
			g_value_init(&g_value, G_TYPE_STRING);
			gupnp_service_action_get_value(service_action->action, param_name, &g_value);
			RETURN_STRING((char *)g_value_get_string(&g_value), 1);

		default: 
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_type' is not correctly defined");
			return;
	}
}
/* }}} */

/* {{{ proto bool gupnp_service_notify(resource service, string name, int type, mixed value)
   Notifies listening clients that the property have changed to the specified values. */
PHP_FUNCTION(gupnp_service_notify)
{
	zval *zservice, *param_val;
	char *param_name;
	int param_name_len;
	long param_type;
	php_gupnp_service_t *service;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rslz", 
			&zservice, &param_name, &param_name_len, &param_type, &param_val) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE(zservice, service);
	
	switch (param_type) {
		case G_TYPE_BOOLEAN:
			if (Z_TYPE_P(param_val) == IS_BOOL) {
				gupnp_service_notify(service->service, param_name, param_type, Z_BVAL_P(param_val), NULL);
				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be boolean");
				return;
			}
			break; 
		
		case G_TYPE_INT:
		case G_TYPE_LONG:
			if (Z_TYPE_P(param_val) == IS_LONG) {
				gupnp_service_notify(service->service, param_name, param_type, Z_LVAL_P(param_val), NULL);
				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be integer");
				return;
			}
			break; 
		
		case G_TYPE_FLOAT:
		case G_TYPE_DOUBLE: 
			if (Z_TYPE_P(param_val) == IS_DOUBLE) {
				gupnp_service_notify(service->service, param_name, param_type, Z_DVAL_P(param_val), NULL);
				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be float");
				return;
			}
			break; 
			
		case G_TYPE_STRING: 
			if (Z_TYPE_P(param_val) == IS_STRING) {
				gupnp_service_notify(service->service, param_name, param_type, Z_STRVAL_P(param_val), NULL);
				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be string");
				return;
			}
			break; 

		default: 
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_type' is not correctly defined");
			return;
	}
}
/* }}} */

/* {{{ proto bool gupnp_service_freeze_notify(resource service)
   Causes new notifications to be queued up until gupnp_service_thaw_notify() is called. */
PHP_FUNCTION(gupnp_service_freeze_notify)
{
	zval *zservice;
	php_gupnp_service_t *service;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zservice) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE(zservice, service);
	
	gupnp_service_freeze_notify(service->service);
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_service_freeze_notify(resource service)
   Sends out any pending notifications, and stops queuing of new ones. */
PHP_FUNCTION(gupnp_service_thaw_notify)
{
	zval *zservice;
	php_gupnp_service_t *service;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zservice) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE(zservice, service);

	gupnp_service_thaw_notify(service->service);
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_service_action_return(resource action)
   Return succesfully. */
PHP_FUNCTION(gupnp_service_action_return)
{
	zval *zaction;
	php_gupnp_service_action_t *service_action;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zaction) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_ACTION(zaction, service_action);
	
	gupnp_service_action_return(service_action->action);
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto bool gupnp_service_action_return_error(resource action, int error_code[, string error_description])
   Return error_code. */
PHP_FUNCTION(gupnp_service_action_return_error)
{
	zval *zaction;
	long error_code;
	char *error_description = NULL;
	int error_description_len;
	php_gupnp_service_action_t *service_action;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rl|s", &zaction, &error_code, &error_description, &error_description_len) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_ACTION(zaction, service_action);
	gupnp_service_action_return_error(service_action->action, error_code, error_description);
	
	RETURN_TRUE;
}
/* }}} */

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_context_new, 0, 0, 0)
	ZEND_ARG_INFO(0, host_ip)
	ZEND_ARG_INFO(0, port)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_context_get_host_ip, 0, 0, 1)
	ZEND_ARG_INFO(0, context)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_context_get_port, 0, 0, 1)
	ZEND_ARG_INFO(0, context)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_context_set_subscription_timeout, 0, 0, 2)
	ZEND_ARG_INFO(0, context)
	ZEND_ARG_INFO(0, timeout)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_context_get_subscription_timeout, 0, 0, 1)
	ZEND_ARG_INFO(0, context)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_context_host_path, 0, 0, 3)
	ZEND_ARG_INFO(0, context)
	ZEND_ARG_INFO(0, local_path)
	ZEND_ARG_INFO(0, server_path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_context_unhost_path, 0, 0, 2)
	ZEND_ARG_INFO(0, context)
	ZEND_ARG_INFO(0, server_path)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_context_timeout_add, 0, 0, 2)
	ZEND_ARG_INFO(0, timeout)
	ZEND_ARG_INFO(0, callback)
	ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_root_device_new, 0, 0, 2)
	ZEND_ARG_INFO(0, context)
	ZEND_ARG_INFO(0, location)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_root_device_start, 0, 0, 1)
	ZEND_ARG_INFO(0, root_device)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_root_device_stop, 0, 0, 1)
	ZEND_ARG_INFO(0, root_device)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_root_device_set_available, 0, 0, 2)
	ZEND_ARG_INFO(0, root_device)
	ZEND_ARG_INFO(0, available)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_root_device_get_available, 0, 0, 1)
	ZEND_ARG_INFO(0, root_device)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_root_device_get_relative_location, 0, 0, 1)
	ZEND_ARG_INFO(0, root_device)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_device_info_get, 0, 0, 1)
	ZEND_ARG_INFO(0, root_device)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_device_info_get_service, 0, 0, 2)
	ZEND_ARG_INFO(0, root_device)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_device_action_callback_set, 0, 0, 4)
	ZEND_ARG_INFO(0, root_device)
	ZEND_ARG_INFO(0, signal)
	ZEND_ARG_INFO(0, action_name)
	ZEND_ARG_INFO(0, callback)
	ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_control_point_new, 0, 0, 2)
	ZEND_ARG_INFO(0, context)
	ZEND_ARG_INFO(0, target)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_control_point_callback_set, 0, 0, 3)
	ZEND_ARG_INFO(0, cpoint)
	ZEND_ARG_INFO(0, signal)
	ZEND_ARG_INFO(0, callback)
	ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_control_point_browse_start, 0, 0, 1)
	ZEND_ARG_INFO(0, cpoint)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_control_point_browse_stop, 0, 0, 1)
	ZEND_ARG_INFO(0, cpoint)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_info_get, 0, 0, 1)
	ZEND_ARG_INFO(0, proxy)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_info_get_introspection, 0, 0, 1)
	ZEND_ARG_INFO(0, proxy)
	ZEND_ARG_INFO(0, callback)
	ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_introspection_get_state_variable, 0, 0, 2)
	ZEND_ARG_INFO(0, introspection)
	ZEND_ARG_INFO(0, variable_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_proxy_action_set, 0, 0, 5)
	ZEND_ARG_INFO(0, proxy)
	ZEND_ARG_INFO(0, action)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_proxy_action_get, 0, 0, 4)
	ZEND_ARG_INFO(0, proxy)
	ZEND_ARG_INFO(0, action)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_proxy_set_subscribed, 0, 0, 2)
	ZEND_ARG_INFO(0, proxy)
	ZEND_ARG_INFO(0, subscribed)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_proxy_get_subscribed, 0, 0, 1)
	ZEND_ARG_INFO(0, proxy)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_proxy_add_notify, 0, 0, 4)
	ZEND_ARG_INFO(0, proxy)
	ZEND_ARG_INFO(0, value)
	ZEND_ARG_INFO(0, type)
	ZEND_ARG_INFO(0, callback)
	ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_proxy_remove_notify, 0, 0, 2)
	ZEND_ARG_INFO(0, proxy)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_proxy_callback_set, 0, 0, 3)
	ZEND_ARG_INFO(0, cpoint)
	ZEND_ARG_INFO(0, signal)
	ZEND_ARG_INFO(0, callback)
	ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_action_set, 0, 0, 4)
	ZEND_ARG_INFO(0, action)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, type)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_action_get, 0, 0, 3)
	ZEND_ARG_INFO(0, action)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_notify, 0, 0, 4)
	ZEND_ARG_INFO(0, service)
	ZEND_ARG_INFO(0, name)
	ZEND_ARG_INFO(0, type)
	ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_freeze_notify, 0, 0, 1)
	ZEND_ARG_INFO(0, service)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_thaw_notify, 0, 0, 1)
	ZEND_ARG_INFO(0, service)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_action_return, 0, 0, 1)
	ZEND_ARG_INFO(0, action)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_gupnp_service_action_return_error, 0, 0, 2)
	ZEND_ARG_INFO(0, action)
	ZEND_ARG_INFO(0, error_code)
	ZEND_ARG_INFO(0, error_description)
ZEND_END_ARG_INFO()

/* {{{ gupnp_functions[]
 *
 * Every user visible function must have an entry in gupnp_functions[].
 */
zend_function_entry gupnp_functions[] = {
	PHP_FE(gupnp_context_new,	arginfo_gupnp_context_new)
	PHP_FE(gupnp_context_get_host_ip,	arginfo_gupnp_context_get_host_ip)
	PHP_FE(gupnp_context_get_port,	arginfo_gupnp_context_get_port)
	PHP_FE(gupnp_context_set_subscription_timeout,	arginfo_gupnp_context_set_subscription_timeout)
	PHP_FE(gupnp_context_get_subscription_timeout,	arginfo_gupnp_context_get_subscription_timeout)
	PHP_FE(gupnp_context_host_path,	arginfo_gupnp_context_host_path)
	PHP_FE(gupnp_context_unhost_path,	arginfo_gupnp_context_unhost_path)
	PHP_FE(gupnp_context_timeout_add,	arginfo_gupnp_context_timeout_add)
	PHP_FE(gupnp_root_device_new,	arginfo_gupnp_root_device_new)
	PHP_FE(gupnp_root_device_start,	arginfo_gupnp_root_device_start)
	PHP_FE(gupnp_root_device_stop,	arginfo_gupnp_root_device_stop)
	PHP_FE(gupnp_root_device_set_available,	arginfo_gupnp_root_device_set_available)
	PHP_FE(gupnp_root_device_get_available,	arginfo_gupnp_root_device_get_available)
	PHP_FE(gupnp_root_device_get_relative_location,	arginfo_gupnp_root_device_get_relative_location)
	PHP_FE(gupnp_device_info_get,	arginfo_gupnp_device_info_get)
	PHP_FE(gupnp_device_info_get_service,	arginfo_gupnp_device_info_get_service)
	PHP_FE(gupnp_device_action_callback_set,	arginfo_gupnp_device_action_callback_set)
	PHP_FE(gupnp_control_point_new,	arginfo_gupnp_control_point_new)
	PHP_FE(gupnp_control_point_callback_set,	arginfo_gupnp_control_point_callback_set)
	PHP_FE(gupnp_control_point_browse_start, 	arginfo_gupnp_control_point_browse_start)
	PHP_FE(gupnp_control_point_browse_stop, 	arginfo_gupnp_control_point_browse_stop)
	PHP_FE(gupnp_service_info_get, 	arginfo_gupnp_service_info_get)
	PHP_FE(gupnp_service_info_get_introspection, 	arginfo_gupnp_service_info_get_introspection)
	PHP_FE(gupnp_service_introspection_get_state_variable, 	arginfo_gupnp_service_introspection_get_state_variable)
	PHP_FE(gupnp_service_proxy_action_set, 	arginfo_gupnp_service_proxy_action_set)
	PHP_FE(gupnp_service_proxy_action_get, 	arginfo_gupnp_service_proxy_action_get)
	
	//PHP_FE(gupnp_service_proxy_begin_action, 	NULL)
	//PHP_FE(gupnp_service_proxy_send_action_hash, 	NULL)
	PHP_FE(gupnp_service_proxy_send_action_tmp, 	NULL)
	
	PHP_FE(gupnp_service_proxy_set_subscribed, 	arginfo_gupnp_service_proxy_set_subscribed)
	PHP_FE(gupnp_service_proxy_get_subscribed, 	arginfo_gupnp_service_proxy_get_subscribed)
	PHP_FE(gupnp_service_proxy_add_notify, 	arginfo_gupnp_service_proxy_add_notify)
	PHP_FE(gupnp_service_proxy_remove_notify, 	arginfo_gupnp_service_proxy_remove_notify)
	PHP_FE(gupnp_service_proxy_callback_set, 	arginfo_gupnp_service_proxy_callback_set)
	PHP_FE(gupnp_service_action_set, 	arginfo_gupnp_service_action_set)
	PHP_FE(gupnp_service_action_get, 	arginfo_gupnp_service_action_get)
	PHP_FE(gupnp_service_notify, 	arginfo_gupnp_service_notify)
	PHP_FE(gupnp_service_freeze_notify, 	arginfo_gupnp_service_freeze_notify)
	PHP_FE(gupnp_service_thaw_notify, 	arginfo_gupnp_service_thaw_notify)
	PHP_FE(gupnp_service_action_return, 	arginfo_gupnp_service_action_return)
	PHP_FE(gupnp_service_action_return_error, 	arginfo_gupnp_service_action_return_error)
	{NULL, NULL, NULL}	/* Must be the last line in gupnp_functions[] */
};
/* }}} */

/* {{{ gupnp_module_entry
 */
zend_module_entry gupnp_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"gupnp",
	gupnp_functions,
	PHP_MINIT(gupnp),
	PHP_MSHUTDOWN(gupnp),
	PHP_RINIT(gupnp),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(gupnp),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(gupnp),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

/*
 
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
