/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2007 The PHP Group                                |
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

/* $Id: header,v 1.16.2.1.2.1 2007/01/01 19:32:09 iliaa Exp $ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_gupnp.h"

ZEND_DECLARE_MODULE_GLOBALS(gupnp)

typedef struct _php_gupnp_callback_t { /* {{{ */
    zval *func;
    zval *arg;
} php_gupnp_callback_t;
/* }}} */

typedef struct _php_gupnp_context_t { /* {{{ */
	GUPnPContext *context;
	int rsrc_id;
} php_gupnp_context_t;
/* }}} */

typedef struct _php_gupnp_cpoint_t { /* {{{ */
	GUPnPControlPoint *cp;
	int rsrc_id;
	php_gupnp_callback_t *callback;
	GMainLoop *main_loop;
#ifdef ZTS
	void ***thread_ctx;
#endif
} php_gupnp_cpoint_t;
/* }}} */

typedef struct _php_gupnp_rdevice_t { /* {{{ */
	GUPnPRootDevice *rd;
	int rsrc_id;
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
#ifdef ZTS
	void ***thread_ctx;
#endif
} php_gupnp_service_proxy_t;
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

typedef struct _php_gupnp_service_action_t { /* {{{ */
	GUPnPServiceAction *action;
	int rsrc_id;
	php_gupnp_callback_t *callback;
#ifdef ZTS
	void ***thread_ctx;
#endif
} php_gupnp_service_action_t;
/* }}} */

/* True global resources - no need for thread safety here */
static int le_context;
static int le_cpoint;
static int le_rdevice;
static int le_service;
static int le_service_proxy;
static int le_service_info;
static int le_service_action;

GMainLoop *main_loop; // TO DELETE

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

#define ZVAL_TO_SERVICE_INFO(zval, service_info) \
		ZEND_FETCH_RESOURCE(service_info, php_gupnp_service_info_t *, &zval, -1, "service info", le_service_info)

#define ZVAL_TO_SERVICE_ACTION(zval, service_action) \
		ZEND_FETCH_RESOURCE(service_action, php_gupnp_service_action_t *, &zval, -1, "service action", le_service_action)


/* {{{ gupnp_functions[]
 *
 * Every user visible function must have an entry in gupnp_functions[].
 */
zend_function_entry gupnp_functions[] = {
	PHP_FE(gupnp_context_new,	NULL)
	PHP_FE(gupnp_context_get_host_ip,	NULL)
	PHP_FE(gupnp_context_get_port,	NULL)
	PHP_FE(gupnp_context_set_subscription_timeout,	NULL)
	PHP_FE(gupnp_context_get_subscription_timeout,	NULL)
	PHP_FE(gupnp_context_host_path,	NULL)
	PHP_FE(gupnp_context_unhost_path,	NULL)
	PHP_FE(gupnp_control_point_new,	NULL)
	PHP_FE(gupnp_root_device_new,	NULL)
	PHP_FE(gupnp_root_device_set_available,	NULL)
	PHP_FE(gupnp_root_device_get_available,	NULL)
	PHP_FE(gupnp_root_device_get_relative_location,	NULL)
	PHP_FE(gupnp_device_info_get,	NULL)
	PHP_FE(gupnp_device_info_get_service,	NULL)
	PHP_FE(gupnp_device_action_callback_set,	NULL)
	PHP_FE(gupnp_main_loop_run,	NULL)
	PHP_FE(gupnp_main_loop_stop,	NULL)
	PHP_FE(gupnp_browse_service, 	NULL)
	PHP_FE(gupnp_service_info_get, 	NULL)
	PHP_FE(gupnp_service_proxy_action_set, 	NULL)
	PHP_FE(gupnp_service_proxy_action_get, 	NULL)
	PHP_FE(gupnp_service_proxy_set_subscribed, 	NULL)
	PHP_FE(gupnp_service_proxy_get_subscribed, 	NULL)
	PHP_FE(gupnp_service_proxy_add_notify, 	NULL)
	PHP_FE(gupnp_service_proxy_remove_notify, 	NULL)
	PHP_FE(gupnp_service_action_set, 	NULL)
	PHP_FE(gupnp_service_action_get, 	NULL)
	PHP_FE(gupnp_service_notify, 	NULL)
	PHP_FE(gupnp_service_action_return, 	NULL)
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

#ifdef COMPILE_DL_GUPNP
ZEND_GET_MODULE(gupnp)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("gupnp.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_gupnp_globals, gupnp_globals)
    STD_PHP_INI_ENTRY("gupnp.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_gupnp_globals, gupnp_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_gupnp_init_globals
 */
/*static void php_gupnp_init_globals(zend_gupnp_globals *gupnp_globals)
{
	gupnp_globals->main_loop = NULL;
}*/
/* }}} */

/* {{{ _php_gupnp_callback_free
 */
static inline void _php_gupnp_callback_free(php_gupnp_callback_t *callback) /* {{{ */
{
	if (!callback) {
		return;
	}

	zval_ptr_dtor(&callback->func);
	if (callback->arg) {
		zval_ptr_dtor(&callback->arg);
	}
	efree(callback);
}
/* }}} */

/* {{{ _php_gupnp_cpoint_dtor
 */
static void _php_gupnp_cpoint_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_cpoint_t *cpoint = (php_gupnp_cpoint_t *)rsrc->ptr;
	
	_php_gupnp_callback_free(cpoint->callback);
	g_object_unref(cpoint->cp);
	efree(cpoint);
}
/* }}} */

/* {{{ _php_gupnp_rdevice_dtor
 */
static void _php_gupnp_rdevice_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_rdevice_t *rdevice = (php_gupnp_rdevice_t *)rsrc->ptr;
	
	if (rdevice->rd) {
		g_object_unref(rdevice->rd);
	}
	efree(rdevice);
}
/* }}} */

/* {{{ _php_gupnp_service_dtor
 */
static void _php_gupnp_service_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_service_t *service = (php_gupnp_service_t *)rsrc->ptr;
	
	efree(service);
}
/* }}} */

/* {{{ _php_gupnp_service_proxy_dtor
 */
static void _php_gupnp_service_proxy_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_service_proxy_t *sproxy = (php_gupnp_service_proxy_t *)rsrc->ptr;
	
	_php_gupnp_callback_free(sproxy->callback);
	efree(sproxy);
}
/* }}} */

/* {{{ _php_gupnp_context_dtor
 */
static void _php_gupnp_context_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_context_t *context = (php_gupnp_context_t *)rsrc->ptr;
	
	g_object_unref(context->context);
	efree(context);
}
/* }}} */

/* {{{ _php_gupnp_service_info_dtor
 */
static void _php_gupnp_service_info_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_service_info_t *service = (php_gupnp_service_info_t *)rsrc->ptr;
	
	g_object_unref(service->service_info);
	_php_gupnp_callback_free(service->callback);
	efree(service);
}
/* }}} */

/* {{{ _php_gupnp_service_action_dtor
 */
static void _php_gupnp_service_action_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_service_action_t *service_action = (php_gupnp_service_action_t *)rsrc->ptr;
	
	_php_gupnp_callback_free(service_action->callback);
	efree(service_action);
}
/* }}} */

/* {{{ _php_gupnp_service_proxy_available_cb
 */
static void _php_gupnp_service_proxy_available_cb(GUPnPControlPoint *cp, GUPnPServiceProxy *proxy, gpointer userdata)
{
	zval *args[2];
	php_gupnp_cpoint_t *cpoint = (php_gupnp_cpoint_t *)userdata;
	php_gupnp_callback_t *callback;
	php_gupnp_service_proxy_t *sproxy;
	zval retval;
	TSRMLS_FETCH_FROM_CTX(cpoint ? cpoint->thread_ctx : NULL);
	
	if (!cpoint || !cpoint->callback) {
		return;
	}
	
	sproxy = emalloc(sizeof(php_gupnp_service_proxy_t));
	sproxy->proxy = proxy;
	sproxy->callback = NULL;
	TSRMLS_SET_CTX(sproxy->thread_ctx);
	sproxy->rsrc_id = zend_list_insert(sproxy, le_service_proxy);
	
	MAKE_STD_ZVAL(args[0]);
	ZVAL_RESOURCE(args[0], sproxy->rsrc_id);
	zend_list_addref(sproxy->rsrc_id);
	
	callback = cpoint->callback;
	args[1] = callback->arg;
	args[1]->refcount++;
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 2, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	zval_ptr_dtor(&(args[1]));

	g_main_loop_quit(cpoint->main_loop);
	
	return;
}
/* }}} */

/* {{{ _php_gupnp_service_proxy_notify_cb
 */
static void _php_gupnp_service_proxy_notify_cb(GUPnPServiceProxy *proxy, const char *variable, GValue *value, gpointer userdata)
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
		case G_TYPE_LONG: 
			ZVAL_LONG(args[1], g_value_get_long(value));
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
	args[2]->refcount++;
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 3, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	zval_ptr_dtor(&(args[1]));
	zval_ptr_dtor(&(args[2]));
	
	return;
}
/* }}} */

/* {{{ _php_gupnp_device_action_cb
 */
static void _php_gupnp_device_action_cb(GUPnPService *gupnp_service, GUPnPServiceAction *gupnp_action, gpointer userdata) {
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
	service = emalloc(sizeof(php_gupnp_service_t));
	service->service = gupnp_service;
	service->rsrc_id = zend_list_insert(service, le_service);
	ZVAL_RESOURCE(args[0], service->rsrc_id);
	zend_list_addref(service->rsrc_id);
	
	MAKE_STD_ZVAL(args[1]);
	service_action->action = gupnp_action;
	ZVAL_RESOURCE(args[1], service_action->rsrc_id);
	zend_list_addref(service_action->rsrc_id);
	
	callback = service_action->callback;
	args[2] = callback->arg;
	args[2]->refcount++;
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 3, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	zval_ptr_dtor(&(args[1]));
	zval_ptr_dtor(&(args[2]));
}
/* }}} */

/* {{{ _php_gupnp_service_proxy_send_action
 */
static gboolean _php_gupnp_service_proxy_send_action(GUPnPServiceProxy *proxy, 
			const char *action, GError **error, const char *p_name, long p_type, GValue *p_value, long action_type) 
{
	gboolean result = 0;
	
	switch (p_type) {
		case G_TYPE_BOOLEAN:
			if (action_type == GUPNP_ACTION_GET) {
				gboolean value_boolean = 0;
				
				result = gupnp_service_proxy_send_action(proxy, action, error, 
							NULL, p_name, G_TYPE_BOOLEAN, &value_boolean, NULL);
				if (result) {
					g_value_init(p_value, G_TYPE_BOOLEAN);
					g_value_set_boolean(p_value, value_boolean);
				}
			} else {
				result = gupnp_service_proxy_send_action(proxy, action, error, 
							p_name, G_TYPE_BOOLEAN, g_value_get_boolean(p_value), NULL, NULL);
			}
			break;
		case G_TYPE_LONG:
			if (action_type == GUPNP_ACTION_GET) {
				glong value_long = 0;
				
				result = gupnp_service_proxy_send_action(proxy, action, error, 
							NULL, p_name, G_TYPE_LONG, &value_long, NULL);
				if (result) {
					g_value_init(p_value, G_TYPE_LONG);
					g_value_set_long(p_value, value_long);
				}
			} else {
				result = gupnp_service_proxy_send_action(proxy, action, error, 
							p_name, G_TYPE_LONG, g_value_get_long(p_value), NULL, NULL);
			}
			break;
			
		case G_TYPE_DOUBLE:
			if (action_type == GUPNP_ACTION_GET) {
				glong value_double = 0;
				
				result = gupnp_service_proxy_send_action(proxy, action, error, 
							NULL, p_name, G_TYPE_DOUBLE, &value_double, NULL);
				if (result) {
					g_value_init(p_value, G_TYPE_DOUBLE);
					g_value_set_double(p_value, value_double);
				}
			} else {
				result = gupnp_service_proxy_send_action(proxy, action, error, 
							p_name, G_TYPE_DOUBLE, g_value_get_double(p_value), NULL, NULL);
			}
			break;
			
		case G_TYPE_STRING:
			if (action_type == GUPNP_ACTION_GET) {
				gchar *value_char = NULL;
				result = gupnp_service_proxy_send_action(proxy, action, error, 
							NULL, p_name, G_TYPE_STRING, value_char, NULL);
				if (result) {
					g_value_init(p_value, G_TYPE_STRING);
					g_value_set_string(p_value, value_char);
				}
			} else {
				result = gupnp_service_proxy_send_action(proxy, action, error, 
							p_name, G_TYPE_STRING, g_value_get_string(p_value), NULL, NULL);
			}
			break;
	}
	
	return result;
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(gupnp)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	
	REGISTER_LONG_CONSTANT("GUPNP_TYPE_BOOLEAN", G_TYPE_BOOLEAN,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_TYPE_LONG", G_TYPE_LONG,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_TYPE_DOUBLE", G_TYPE_DOUBLE,  CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("GUPNP_TYPE_STRING", G_TYPE_STRING,  CONST_CS | CONST_PERSISTENT);
	
	le_context = zend_register_list_destructors_ex(_php_gupnp_context_dtor, NULL, "context", module_number);
	le_cpoint = zend_register_list_destructors_ex(_php_gupnp_cpoint_dtor, NULL, "control point", module_number);
	le_rdevice = zend_register_list_destructors_ex(_php_gupnp_rdevice_dtor, NULL, "root device", module_number);
	le_service = zend_register_list_destructors_ex(_php_gupnp_service_dtor, NULL, "service", module_number);
	le_service_proxy = zend_register_list_destructors_ex(_php_gupnp_service_proxy_dtor, NULL, "service proxy", module_number);
	le_service_info = zend_register_list_destructors_ex(_php_gupnp_service_info_dtor, NULL, "service info", module_number);
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
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	
	/* Clean up */
	if (GUPNP_G(main_loop)) {
		g_main_loop_unref(GUPNP_G(main_loop));
		
	}
	/*if (GUPNP_G(context)) {
		g_object_unref(GUPNP_G(context));
	}*/
	
	
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(gupnp)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
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

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
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
   Sets the event subscription timeout to timeout. 
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

/* {{{ proto resource gupnp_control_point_new(resource context, string target)
   Create a new Control point with the specified target */
PHP_FUNCTION(gupnp_control_point_new)
{
	char *target = NULL;
	int target_len;
	zval *zcontext;
	php_gupnp_cpoint_t *cpoint = NULL;
	php_gupnp_context_t *context = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rs", &zcontext, &target, &target_len) == FAILURE) {
		return;
	}
	
	ZVAL_TO_CONTEXT(zcontext, context);
		
	cpoint = emalloc(sizeof(php_gupnp_cpoint_t));
	cpoint->cp = gupnp_control_point_new(context->context, target);
	cpoint->callback = NULL;
	cpoint->main_loop = NULL;
	TSRMLS_SET_CTX(cpoint->thread_ctx);
	cpoint->rsrc_id = zend_list_insert(cpoint, le_cpoint);
	
	RETURN_RESOURCE(cpoint->rsrc_id);
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
	
	if (rdevice->rd) {
		rdevice->rsrc_id = zend_list_insert(rdevice, le_rdevice);
		RETURN_RESOURCE(rdevice->rsrc_id);
	}
	efree(rdevice);
	RETURN_FALSE;
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

/* {{{ proto bool gupnp_root_device_get_available(resource root_device)
   Get whether or not root_device is available (announcing its presence). */
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
   Get info of the device. */
PHP_FUNCTION(gupnp_device_info_get)
{
	zval *zrdevice;
	php_gupnp_rdevice_t *rdevice = NULL;
	char *info_data[2][14];
	int i;
	SoupURI* url_base;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zrdevice) == FAILURE) {
		return;
	}
	
	ZVAL_TO_RDEVICE(zrdevice, rdevice);
	
	url_base = gupnp_device_info_get_url_base(GUPNP_DEVICE_INFO(rdevice->rd));
	
	info_data[0][0] = "location";
	info_data[1][0] = (char *)gupnp_device_info_get_location(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][1] = "url_base";
	info_data[1][1] = soup_uri_to_string(url_base, 1);
	info_data[0][2] = "udn";
	info_data[1][2] = (char *)gupnp_device_info_get_udn(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][3] = "device_type";
	info_data[1][3] = (char *)gupnp_device_info_get_device_type(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][4] = "friendly_name";
	info_data[1][4] = gupnp_device_info_get_friendly_name(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][5] = "manufacturer";
	info_data[1][5] = gupnp_device_info_get_manufacturer(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][6] = "manufacturer_url";
	info_data[1][6] = gupnp_device_info_get_manufacturer_url(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][7] = "model_description";
	info_data[1][7] = gupnp_device_info_get_model_description(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][8] = "model_name";
	info_data[1][8] = gupnp_device_info_get_model_name(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][9] = "model_number";
	info_data[1][9] = gupnp_device_info_get_model_number(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][10] = "model_url";
	info_data[1][10] = gupnp_device_info_get_model_url(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][11] = "serial_number";
	info_data[1][11] = gupnp_device_info_get_serial_number(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][12] = "presentation_url";
	info_data[1][12] = gupnp_device_info_get_presentation_url(GUPNP_DEVICE_INFO(rdevice->rd));
	info_data[0][13] = "upc";
	info_data[1][13] = gupnp_device_info_get_upc(GUPNP_DEVICE_INFO(rdevice->rd));
	
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

/* {{{ proto resource gupnp_device_info_get_service(resource root_device, string type)
   Get the service with type or false if no such device was found.  */
PHP_FUNCTION(gupnp_device_action_callback_set)
{
	zval *zservice, *zcallback, *zarg = NULL;
	char *func_name, *action_name;
	int action_name_len;
	php_gupnp_callback_t *callback, *old_callback;
	php_gupnp_service_info_t *service;
	php_gupnp_service_action_t *service_action;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsz|z", &zservice, &action_name, &action_name_len, &zcallback, &zarg) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_INFO(zservice, service);
	
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
	
	service_action = emalloc(sizeof(php_gupnp_service_action_t));
	service_action->action = NULL;
	service_action->callback = callback;
	service_action->rsrc_id = zend_list_insert(service_action, le_service_action);
	
	//old_callback = service->callback;
	//service->callback = callback;
	
	g_signal_connect(service->service_info, action_name, G_CALLBACK(_php_gupnp_device_action_cb), service_action);
	
	/*
	if (old_callback) {
		_php_gupnp_callback_free(old_callback);
	}
	*/

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto string gupnp_context_get_host_ip(resource context)
   Get the IP address we advertise ourselves as using. */
PHP_FUNCTION(gupnp_main_loop_run)
{
	main_loop = g_main_loop_new(NULL, FALSE);
  	g_main_loop_run(main_loop);
}
/* }}} */

/* {{{ proto string gupnp_context_get_host_ip(resource context)
   Get the IP address we advertise ourselves as using. */
PHP_FUNCTION(gupnp_main_loop_stop)
{
	g_main_loop_unref(main_loop);
}
/* }}} */

/* {{{ proto bool gupnp_browse_service(resource cpoint, mixed callback[, mixed arg])
   Starts the search and calls user-defined callback. */
PHP_FUNCTION(gupnp_browse_service)
{
	zval *zcpoint, *zcallback, *zarg = NULL;
	char *func_name;
	php_gupnp_callback_t *callback, *old_callback;
	php_gupnp_cpoint_t *cpoint = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz|z", &zcpoint, &zcallback, &zarg) == FAILURE) {
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
	
	callback = emalloc(sizeof(php_gupnp_callback_t));
	callback->func = zcallback;
	callback->arg = zarg;
	
	old_callback = cpoint->callback;
	cpoint->callback = callback;
	
	cpoint->main_loop = g_main_loop_new(NULL, FALSE);
	
	g_signal_connect(cpoint->cp, "service-proxy-available", 
					 G_CALLBACK(_php_gupnp_service_proxy_available_cb), cpoint);
	
	/* Tell the Control Point to start searching */
	gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(cpoint->cp), TRUE);
  
	g_main_loop_run(cpoint->main_loop);
	
	if (old_callback) {
		_php_gupnp_callback_free(old_callback);
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto array gupnp_service_info_get(resource proxy)
   Get info of the service. */
PHP_FUNCTION(gupnp_service_info_get)
{
	zval *zproxy;
	php_gupnp_service_proxy_t *sproxy;
	char *info_data[2][8];
	int i;
	SoupURI* url_base;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zproxy) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
	
	url_base = gupnp_service_info_get_url_base(GUPNP_SERVICE_INFO(sproxy->proxy));
	
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

/* {{{ proto bool gupnp_service_proxy_action_set(resource proxy, string action, string name, mixed value, int type)
   Sends action with parameters to the service exposed by proxy synchronously and set value. */
PHP_FUNCTION(gupnp_service_proxy_action_set)
{
	zval *zproxy, *param_val;
	char *action, *param_name;
	int action_len, param_name_len;
	long param_type;
	php_gupnp_service_proxy_t *sproxy;
	GError *error = NULL;
	gboolean result = 0;
	GValue value = {0};
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsszl", 
			&zproxy, &action, &action_len, &param_name, &param_name_len, 
			&param_val, &param_type) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
	
	switch (param_type) {
		case G_TYPE_BOOLEAN:
			if (Z_TYPE_P(param_val) == IS_BOOL) {
				g_value_init(&value, G_TYPE_BOOLEAN);
				g_value_set_boolean(&value, Z_BVAL_P(param_val));
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be boolean");
				return;
			}
			break; 
		
		case G_TYPE_LONG:
			if (Z_TYPE_P(param_val) == IS_LONG) {
				g_value_init(&value, G_TYPE_LONG);
				g_value_set_long(&value, Z_LVAL_P(param_val));
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be integer");
				return;
			}
			break; 
		
		case G_TYPE_DOUBLE: 
			if (Z_TYPE_P(param_val) == IS_DOUBLE) {
				g_value_init(&value, G_TYPE_DOUBLE);
				g_value_set_double(&value, Z_DVAL_P(param_val));
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be float");
				return;
			}
			break; 
			
		case G_TYPE_STRING: 
			if (Z_TYPE_P(param_val) == IS_STRING) {
				g_value_init(&value, G_TYPE_STRING);
				g_value_set_string(&value, Z_STRVAL_P(param_val));
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be string");
				return;
			}
			break; 
		
		default: 
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_type' is not correctly defined");
			return;
	}
	
	result = _php_gupnp_service_proxy_send_action(sproxy->proxy, action, &error, 
				param_name, param_type, &value, GUPNP_ACTION_SET);
	
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

/* {{{ proto bool gupnp_service_proxy_action_set(resource proxy, string action, string name, int type)
   Sends action with parameters to the service exposed by proxy synchronously and get value. */
PHP_FUNCTION(gupnp_service_proxy_action_get)
{
	zval *zproxy;
	char *action, *param_name;
	int action_len, param_name_len;
	long param_type;
	php_gupnp_service_proxy_t *sproxy;
	GError *error = NULL;
	gboolean result = 0;
	GValue value = {0};
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rssl", 
			&zproxy, &action, &action_len, &param_name, &param_name_len, 
			&param_type) == FAILURE) {
		return;
	}
	
	ZVAL_TO_SERVICE_PROXY(zproxy, sproxy);
	
	result = _php_gupnp_service_proxy_send_action (sproxy->proxy, action, &error, 
				param_name, param_type, &value, GUPNP_ACTION_GET);
	
	if (!result) {
		if (error != NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable to send action: %s", error->message);
			g_error_free(error);
		}
		RETURN_FALSE;
	}
	
	switch (param_type) {
		case G_TYPE_BOOLEAN:
			RETURN_BOOL(g_value_get_boolean(&value));
		case G_TYPE_LONG:
			RETURN_LONG(g_value_get_long(&value));
		case G_TYPE_DOUBLE: 
			RETURN_DOUBLE(g_value_get_double(&value));
		case G_TYPE_STRING: 
			RETURN_STRING((char *)g_value_get_string(&value), 1);
		default: 
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_type' is not correctly defined");
			RETURN_FALSE;
	}

}
/* }}} */

/* {{{ proto void gupnp_service_proxy_set_subscribed(resource proxy, boolean subscribed)
   (Un)subscribes to this service. */
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
}
/* }}} */

/* {{{ proto bool gupnp_service_proxy_get_subscribed(resource proxy)
   Returns true if we are subscribed to this service. */
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

/* {{{ proto bool gupnp_service_proxy_action_set(resource proxy, string action, string name, mixed value, int type)
   Sends action with parameters to the service exposed by proxy synchronously and set value. */
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
		
		case G_TYPE_LONG:
			if (Z_TYPE_P(param_val) == IS_LONG) {
				gupnp_service_action_set(service_action->action, param_name, param_type, Z_LVAL_P(param_val), NULL);
				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be integer");
				return;
			}
			break; 
		
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

/* {{{ proto bool gupnp_service_proxy_action_set(resource proxy, string action, string name, mixed value, int type)
   Sends action with parameters to the service exposed by proxy synchronously and set value. */
PHP_FUNCTION(gupnp_service_action_get)
{
	zval *zaction, *param_val;
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
	
	gboolean value_bool;
	switch (param_type) {
		case G_TYPE_BOOLEAN:
			
			gupnp_service_action_get(service_action->action, param_name, param_type, &value_bool, NULL);
			RETURN_BOOL(value_bool);
			//gupnp_service_action_get_value(service_action->action, param_name, &g_value);
			//RETURN_BOOL(g_value_get_boolean(&g_value));
			break; 
		/*
		case G_TYPE_LONG:
			if (Z_TYPE_P(param_val) == IS_LONG) {
				gupnp_service_action_set(service_action->action, param_name, param_type, Z_LVAL_P(param_val), NULL);
				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be integer");
				return;
			}
			break; 
		
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
		*/
		default: 
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_type' is not correctly defined");
			return;
	}
}
/* }}} */

/* {{{ proto bool gupnp_service_proxy_action_set(resource proxy, string action, string name, mixed value, int type)
   Sends action with parameters to the service exposed by proxy synchronously and set value. */
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
		
		case G_TYPE_LONG:
			if (Z_TYPE_P(param_val) == IS_LONG) {
				gupnp_service_notify(service->service, param_name, param_type, Z_LVAL_P(param_val), NULL);
				RETURN_TRUE;
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be integer");
				return;
			}
			break; 
		
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


/* {{{ proto bool gupnp_service_proxy_action_set(resource proxy, string action, string name, mixed value, int type)
   Sends action with parameters to the service exposed by proxy synchronously and set value. */
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



/*
 
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
