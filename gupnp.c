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

typedef struct _php_gupnp_cpoint_t { /* {{{ */
	GUPnPControlPoint *cp;
	int rsrc_id;
	php_gupnp_callback_t *callback;
#ifdef ZTS
	void ***thread_ctx;
#endif
} php_gupnp_cpoint_t;
/* }}} */

typedef struct _php_gupnp_service_proxy_t { /* {{{ */
	GUPnPServiceProxy *proxy;
	int rsrc_id;
} php_gupnp_service_proxy_t;
/* }}} */

typedef struct _php_gupnp_context_t { /* {{{ */
	GUPnPContext *context;
	int rsrc_id;
} php_gupnp_context_t;
/* }}} */

/* True global resources - no need for thread safety here */
static int le_context;
static int le_cpoint;
static int le_proxy;

#define ZVAL_TO_CONTEXT(zval, context) \
		ZEND_FETCH_RESOURCE(context, php_gupnp_context_t *, &zval, -1, "context", le_context)

#define ZVAL_TO_PROXY(zval, proxy) \
		ZEND_FETCH_RESOURCE(proxy, php_gupnp_service_proxy_t *, &zval, -1, "proxy", le_proxy)


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
	PHP_FE(gupnp_control_point_new,	NULL)
	PHP_FE(gupnp_browse_service, 	NULL)
	PHP_FE(gupnp_service_info_get, 	NULL)
	PHP_FE(gupnp_service_proxy_send_action, 	NULL)
	PHP_FE(gupnp_service_proxy_set_subscribed, 	NULL)
	PHP_FE(gupnp_service_proxy_get_subscribed, 	NULL)
	PHP_FE(gupnp_service_proxy_add_notify, 	NULL)
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
static void php_gupnp_init_globals(zend_gupnp_globals *gupnp_globals)
{
	gupnp_globals->main_loop = NULL;
	gupnp_globals->context = NULL;
}
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

/* {{{ _php_gupnp_proxy_dtor
 */
static void _php_gupnp_proxy_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) /* {{{ */
{
	php_gupnp_service_proxy_t *sproxy = (php_gupnp_service_proxy_t *)rsrc->ptr;
	
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
	sproxy->rsrc_id = zend_list_insert(sproxy, le_proxy);
	
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
	
	g_main_loop_quit(GUPNP_G(main_loop));
	
	return;
}
/* }}} */

/* {{{ _php_gupnp_service_proxy_notify_cb
 */
static void _php_gupnp_service_proxy_notify_cb(GUPnPServiceProxy *proxy, const char *variable, GValue *value, gpointer userdata)
{
	printf("Value changed: %s\n", g_value_get_string(value));
	
	/*
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
	sproxy->rsrc_id = zend_list_insert(sproxy, le_proxy);
	
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
	
	g_main_loop_quit(GUPNP_G(main_loop));
	*/
	
	return;
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
	le_proxy = zend_register_list_destructors_ex(_php_gupnp_proxy_dtor, NULL, "proxy", module_number);
	
	/* Required initialisation */
	g_thread_init(NULL);
	g_type_init();
	
	//GUPNP_G(context) = gupnp_context_new(NULL, NULL, 0, NULL);
	
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

/* {{{ proto resource gupnp_control_point_new(string target)
   Create a new Control point with the specified target */
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
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable create context: %s", error->message);
			g_error_free (error);
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

/* {{{ proto string gupnp_context_get_port(resource context)
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

/* {{{ proto string gupnp_context_set_subscription_timeout(resource context, long timeout)
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

/* {{{ proto long gupnp_context_get_subscription_timeout(resource context)
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

/* {{{ proto resource gupnp_control_point_new(string target)
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
	TSRMLS_SET_CTX(cpoint->thread_ctx);
	cpoint->rsrc_id = zend_list_insert(cpoint, le_cpoint);
	
	RETURN_RESOURCE(cpoint->rsrc_id);
}
/* }}} */

/* {{{ proto string confirm_gupnp_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(gupnp_browse_service)
{
	zval *zcpoint, *zcallback, *zarg = NULL;
	char *func_name;
	php_gupnp_callback_t *callback, *old_callback;
	php_gupnp_cpoint_t *cpoint = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz|z", &zcpoint, &zcallback, &zarg) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(cpoint, php_gupnp_cpoint_t *, &zcpoint, -1, "control point", le_cpoint);
	
	if (!zend_is_callable(zcallback, 0, &func_name TSRMLS_CC)) {
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
	
	g_signal_connect(cpoint->cp, "service-proxy-available", 
					 G_CALLBACK(_php_gupnp_service_proxy_available_cb), cpoint);

	/* Tell the Control Point to start searching */
	gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(cpoint->cp), TRUE);
  
	/* Enter the main loop. This will start the search and result in callbacks to
	   gupnp_service_proxy_available_cb. */
	GUPNP_G(main_loop) = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(GUPNP_G(main_loop));
	
	if (old_callback) {
		_php_gupnp_callback_free(old_callback);
	}

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto array gupnp_service_info_get(resource proxy)
   Get info of this service. */
PHP_FUNCTION(gupnp_service_info_get)
{
	zval *zproxy;
	char *id, *scpd_url, *control_url, *event_subscription_url;
	php_gupnp_service_proxy_t *sproxy;
	SoupURI* url_base;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zproxy) == FAILURE) {
		return;
	}
	
	//ZEND_FETCH_RESOURCE(sproxy, php_gupnp_service_proxy_t *, &zproxy, -1, "proxy", le_proxy);
	ZVAL_TO_PROXY(zproxy, sproxy);
	
	id = gupnp_service_info_get_id(GUPNP_SERVICE_INFO(sproxy->proxy));
	scpd_url = gupnp_service_info_get_scpd_url(GUPNP_SERVICE_INFO(sproxy->proxy));
	control_url = gupnp_service_info_get_control_url(GUPNP_SERVICE_INFO(sproxy->proxy));
	event_subscription_url = gupnp_service_info_get_event_subscription_url(GUPNP_SERVICE_INFO(sproxy->proxy));
	url_base = gupnp_service_info_get_url_base(GUPNP_SERVICE_INFO(sproxy->proxy));
		
	array_init(return_value);
	add_assoc_string(return_value, "location", gupnp_service_info_get_location(GUPNP_SERVICE_INFO(sproxy->proxy)), 1);
	add_assoc_string(return_value, "url_base", soup_uri_to_string(url_base, 1), 1);
	add_assoc_string(return_value, "udn", gupnp_service_info_get_udn(GUPNP_SERVICE_INFO(sproxy->proxy)), 1); 
	add_assoc_string(return_value, "service_type", gupnp_service_info_get_service_type(GUPNP_SERVICE_INFO(sproxy->proxy)), 1);
	add_assoc_string(return_value, "id", id, 1);
	add_assoc_string(return_value, "scpd_url", scpd_url, 1);
	add_assoc_string(return_value, "control_url", control_url, 1);
	add_assoc_string(return_value, "event_subscription_url", event_subscription_url, 1);
	
	g_free(id);
	g_free(scpd_url);
	g_free(control_url);
	g_free(event_subscription_url);
}
/* }}} */

/* {{{ proto boolean gupnp_service_proxy_send_action(resource proxy, string action, string param_name, zval param_val)
   Sends action with parameters to the service exposed by proxy synchronously. */
PHP_FUNCTION(gupnp_service_proxy_send_action)
{
	zval *zproxy, *param_val;
	char *action, *param_name;
	int action_len, param_name_len;
	long param_type;
	php_gupnp_service_proxy_t *sproxy;
	GError *error = NULL;
	
	gboolean target_gbool = 0, result = 0;
	glong target_glong = 0;
	gdouble target_gdouble = 0;
	gchar *target_gchar = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rsszl", 
			&zproxy, &action, &action_len, &param_name, &param_name_len, 
			&param_val, &param_type) == FAILURE) {
		return;
	}
	
	ZVAL_TO_PROXY(zproxy, sproxy);
	
	switch (param_type) {         
		case G_TYPE_BOOLEAN: 
			if (Z_BVAL_P(param_val)) {
				target_gbool = 1;
            } 
			result = gupnp_service_proxy_send_action (sproxy->proxy, action, &error, 
						param_name, G_TYPE_BOOLEAN, target_gbool, NULL, NULL);
			break; 
		case G_TYPE_LONG: 
			target_glong = (glong)Z_LVAL_P(param_val);
			result = gupnp_service_proxy_send_action (sproxy->proxy, action, &error, 
						param_name, G_TYPE_LONG, target_glong, NULL, NULL);
			break; 
		case G_TYPE_DOUBLE: 
			target_gdouble = (gdouble)Z_LVAL_P(param_val);
			result = gupnp_service_proxy_send_action (sproxy->proxy, action, &error, 
						param_name, G_TYPE_DOUBLE, target_gdouble, NULL, NULL);
			break; 
		case G_TYPE_STRING: 
			if (Z_TYPE_P(param_val) == IS_STRING) {
				target_gchar = (gchar *)Z_LVAL_P(param_val);
				result = gupnp_service_proxy_send_action (sproxy->proxy, action, &error, 
							param_name, G_TYPE_STRING, target_gchar, NULL, NULL);
			} else {
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_val' must be string");
			}
				
			break; 
		default: 
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "'param_type' is not correctly defined");
			break; 
	}
	
	if (result) {
		RETURN_TRUE;
	} else {
		if (error != NULL) {
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "Unable send action: %s", error->message);
			g_error_free (error);
		}
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
	
	ZVAL_TO_PROXY(zproxy, sproxy);
		
	if (zsubscribed) { 
		subscribed = TRUE;
	}
	
	gupnp_service_proxy_set_subscribed(sproxy->proxy, subscribed);
}
/* }}} */

/* {{{ proto boolean gupnp_service_proxy_get_subscribed(resource proxy)
   Returns true if we are subscribed to this service. */
PHP_FUNCTION(gupnp_service_proxy_get_subscribed)
{
	zval *zproxy;
	php_gupnp_service_proxy_t *sproxy;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "r", &zproxy) == FAILURE) {
		return;
	}
	
	ZVAL_TO_PROXY(zproxy, sproxy);
		
	if (gupnp_service_proxy_get_subscribed(sproxy->proxy)) { 
		RETURN_TRUE;
	}
	RETURN_FALSE;
}
/* }}} */

/* {{{ proto boolean gupnp_service_proxy_get_subscribed(resource proxy)
   Returns true if we are subscribed to this service. */
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
	
	ZVAL_TO_PROXY(zproxy, sproxy);
		
	if (!zend_is_callable(zcallback, 0, &func_name TSRMLS_CC)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "'%s' is not a valid callback", func_name);
		efree(func_name);
		RETURN_FALSE;
	}
	efree(func_name);
	
	/*
	zval_add_ref(&zcallback);
	if (zarg) {
		zval_add_ref(&zarg);
	} else {
		ALLOC_INIT_ZVAL(zarg);
	}
	*/
	
	/*
	callback = emalloc(sizeof(php_gupnp_callback_t));
	callback->func = zcallback;
	callback->arg = zarg;
	
	old_callback = cpoint->callback;
	cpoint->callback = callback;
	
	if (old_callback) {
		_php_gupnp_callback_free(old_callback);
	}
	*/
	
	if (!gupnp_service_proxy_add_notify(sproxy->proxy, param_val, G_TYPE_STRING, _php_gupnp_service_proxy_notify_cb, NULL)) {
    	RETURN_FALSE;
	}
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
