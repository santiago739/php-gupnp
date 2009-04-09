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
  | Author:                                                              |
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

/* True global resources - no need for thread safety here */
static int le_cpoint;

/* {{{ gupnp_functions[]
 *
 * Every user visible function must have an entry in gupnp_functions[].
 */
zend_function_entry gupnp_functions[] = {
	PHP_FE(gupnp_control_point_new,	NULL)
	PHP_FE(gupnp_browse_service, 	NULL)
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

/* {{{ _php_gupnp_cpoint_callback_free
 */
static inline void _php_gupnp_cpoint_callback_free(php_gupnp_callback_t *callback) /* {{{ */
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
	
	_php_gupnp_cpoint_callback_free(cpoint->callback);
	g_object_unref(cpoint->cp);
	efree(cpoint);
}
/* }}} */

/* {{{ _php_gupnp_service_proxy_available_cb
 */
static void _php_gupnp_service_proxy_available_cb(GUPnPControlPoint *cp, GUPnPServiceProxy *proxy, gpointer userdata)
{
	zval *args[1];
	php_gupnp_cpoint_t *cpoint = (php_gupnp_cpoint_t *)userdata;
	php_gupnp_callback_t *callback;
	zval retval;
	TSRMLS_FETCH_FROM_CTX(cpoint ? cpoint->thread_ctx : NULL);
	
	if (!cpoint || !cpoint->callback) {
		return;
	}
	
	callback = cpoint->callback;
	
	args[0] = callback->arg;
	args[0]->refcount++;
	
	if (call_user_function(EG(function_table), NULL, callback->func, &retval, 1, args TSRMLS_CC) == SUCCESS) {
		zval_dtor(&retval);
	}
	zval_ptr_dtor(&(args[0]));
	
	g_main_loop_quit(GUPNP_G(main_loop));
	
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
	
	le_cpoint = zend_register_list_destructors_ex(_php_gupnp_cpoint_dtor, NULL, "control point", module_number);
	
	/* Required initialisation */
	g_thread_init(NULL);
	g_type_init();
	
	GUPNP_G(context) = gupnp_context_new(NULL, NULL, 0, NULL);
	
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
	if (GUPNP_G(context)) {
		g_object_unref(GUPNP_G(context));
	}
	
	
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

/* {{{ proto string confirm_gupnp_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(gupnp_control_point_new)
{
	char *target = NULL;
	int target_len;
	php_gupnp_cpoint_t *cpoint = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &target, &target_len) == FAILURE) {
		return;
	}
	
	cpoint = emalloc(sizeof(php_gupnp_cpoint_t));
	cpoint->cp = gupnp_control_point_new(GUPNP_G(context), target);
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
	php_gupnp_callback_t *callback;
	php_gupnp_cpoint_t *cpoint = NULL;
	
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "rz|z", &zcpoint, &zcallback, &zarg) == FAILURE) {
		return;
	}
	
	ZEND_FETCH_RESOURCE(cpoint, php_gupnp_cpoint_t *, &zcpoint, -1, "control point", le_cpoint)
	
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
	cpoint->callback = callback;
	
	g_signal_connect(cpoint->cp, "service-proxy-available", 
					 G_CALLBACK(_php_gupnp_service_proxy_available_cb), cpoint);

	/* Tell the Control Point to start searching */
	gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(cpoint->cp), TRUE);
  
	/* Enter the main loop. This will start the search and result in callbacks to
	   gupnp_service_proxy_available_cb. */
	GUPNP_G(main_loop) = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(GUPNP_G(main_loop));

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
