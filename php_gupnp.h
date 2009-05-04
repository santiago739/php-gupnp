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

#ifndef PHP_GUPNP_H
#define PHP_GUPNP_H

extern zend_module_entry gupnp_module_entry;
#define phpext_gupnp_ptr &gupnp_module_entry

#ifdef PHP_WIN32
#define PHP_GUPNP_API __declspec(dllexport)
#else
#define PHP_GUPNP_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#include <libgupnp/gupnp.h>

PHP_MINIT_FUNCTION(gupnp);
PHP_MSHUTDOWN_FUNCTION(gupnp);
PHP_RINIT_FUNCTION(gupnp);
PHP_RSHUTDOWN_FUNCTION(gupnp);
PHP_MINFO_FUNCTION(gupnp);

PHP_FUNCTION(gupnp_context_new);
PHP_FUNCTION(gupnp_context_get_host_ip);
PHP_FUNCTION(gupnp_context_get_port);
PHP_FUNCTION(gupnp_context_set_subscription_timeout);
PHP_FUNCTION(gupnp_context_get_subscription_timeout);
PHP_FUNCTION(gupnp_context_host_path);
PHP_FUNCTION(gupnp_context_unhost_path);
PHP_FUNCTION(gupnp_root_device_new);
PHP_FUNCTION(gupnp_root_device_start);
PHP_FUNCTION(gupnp_root_device_stop);
PHP_FUNCTION(gupnp_root_device_set_available);
PHP_FUNCTION(gupnp_root_device_get_available);
PHP_FUNCTION(gupnp_root_device_get_relative_location);
PHP_FUNCTION(gupnp_device_info_get);
PHP_FUNCTION(gupnp_device_info_get_service);
PHP_FUNCTION(gupnp_device_action_callback_set);
PHP_FUNCTION(gupnp_control_point_new);
PHP_FUNCTION(gupnp_control_point_callback_set);
PHP_FUNCTION(gupnp_control_point_browse_start);
PHP_FUNCTION(gupnp_control_point_browse_stop);
PHP_FUNCTION(gupnp_service_info_get);
PHP_FUNCTION(gupnp_service_info_get_introspection);
PHP_FUNCTION(gupnp_service_introspection_get_state_variable);
PHP_FUNCTION(gupnp_service_proxy_action_set);
PHP_FUNCTION(gupnp_service_proxy_action_get);
PHP_FUNCTION(gupnp_service_proxy_set_subscribed);
PHP_FUNCTION(gupnp_service_proxy_get_subscribed);
PHP_FUNCTION(gupnp_service_proxy_add_notify);
PHP_FUNCTION(gupnp_service_proxy_remove_notify);
PHP_FUNCTION(gupnp_service_proxy_callback_set);
PHP_FUNCTION(gupnp_service_action_set);
PHP_FUNCTION(gupnp_service_action_get);
PHP_FUNCTION(gupnp_service_notify);
PHP_FUNCTION(gupnp_service_freeze_notify);
PHP_FUNCTION(gupnp_service_thaw_notify);
PHP_FUNCTION(gupnp_service_action_return);

ZEND_BEGIN_MODULE_GLOBALS(gupnp)
	GMainLoop *main_loop;
ZEND_END_MODULE_GLOBALS(gupnp)

#define GUPNP_ACTION_SET 0
#define GUPNP_ACTION_GET 1

#define GUPNP_SIGNAL_DEVICE_PROXY_AVAILABLE 0
#define GUPNP_SIGNAL_DEVICE_PROXY_UNAVAILABLE 1
#define GUPNP_SIGNAL_SERVICE_PROXY_AVAILABLE 2
#define GUPNP_SIGNAL_SERVICE_PROXY_UNAVAILABLE 3
#define GUPNP_SIGNAL_ACTION_INVOKED 4
#define GUPNP_SIGNAL_NOTIFY_FAILED 5
#define GUPNP_SIGNAL_SUBSCRIPTION_LOST 6


#ifdef ZTS
#define GUPNP_G(v) TSRMG(gupnp_globals_id, zend_gupnp_globals *, v)
#else
#define GUPNP_G(v) (gupnp_globals.v)
#endif

#endif	/* PHP_GUPNP_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
