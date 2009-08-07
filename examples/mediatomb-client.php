<?php

function gupnp_service_proxy_action_browse($proxy)
{
	//gupnp_service_proxy_action_set($proxy, 'Browse', 'ObjectID', "0", GUPNP_TYPE_STRING);
	
	//$result = gupnp_service_proxy_action_get($proxy, 'Browse', 'Result', GUPNP_TYPE_STRING);
	//$result = gupnp_service_proxy_action_get($proxy, 'Browse', 'Result', GUPNP_TYPE_STRING);
	//$result = gupnp_service_proxy_action_get($proxy, 'GetSearchCapabilities', 'SearchCaps', GUPNP_TYPE_STRING);
	//var_dump($result);
	
	//gupnp_service_proxy_begin_action($proxy);
	//gupnp_service_proxy_send_action_hash($proxy);
	
	$result = gupnp_service_proxy_send_action_tmp(
		$proxy,
		array(
			array("ObjectID", GUPNP_TYPE_STRING, "0"),
			array("BrowseFlag", GUPNP_TYPE_STRING, "BrowseDirectChildren"),
			array("Filter", GUPNP_TYPE_STRING, "*"),
			array("StartingIndex", GUPNP_TYPE_INT, 1),
			array("RequestedCount", GUPNP_TYPE_INT, 10),
			array("SortCriteria", GUPNP_TYPE_STRING, "")
		),
		array(
			array("Result", GUPNP_TYPE_STRING)
		)
	);
	var_dump($result);
	//sleep(3);

	printf("\n");
}

function gupnp_service_proxy_available_cb($proxy, $arg)
{
	/* Get service info */
	$info = gupnp_service_info_get($proxy);
	
	if ($info['udn'] == $arg['udn']) {
		printf("Service is available\n");
		foreach ($info as $key=>$value) {
			printf("\t%-30s: %s\n", $key, $value);
		}
		printf("\n");
	
		gupnp_service_proxy_action_browse($proxy);
		
		gupnp_control_point_browse_stop($arg['cp_service']);
		
	}
}

function gupnp_device_proxy_available_cb($proxy, $arg)
{
	/* Get device info */
	$info = gupnp_device_info_get($proxy);
	
	if ($info['friendly_name'] == 'MediaTomb') {
		printf("Device is available:\n");
	
		foreach ($info as $key=>$value) {
			printf("\t%-30s: %s\n", $key, $value);
		}
		printf("\n");
		
		gupnp_control_point_browse_stop($arg['cp_device']);
		
		$arg_service = array('udn' => $info['udn'], 'cp_service' => $arg['cp_service']);
		$cb_service = "gupnp_service_proxy_available_cb";
		gupnp_control_point_callback_set($arg['cp_service'], 
			GUPNP_SIGNAL_SERVICE_PROXY_AVAILABLE, $cb_service, $arg_service);
		
		gupnp_control_point_browse_start($arg['cp_service']);
	}
}

/* Create the UPnP context */
$context = gupnp_context_new();
if (!$context) {
	printf("Error creating the GUPnP context\n");
	exit(-1);
}

/* Create the control point, searching for MediaTomb */
$cp_device = gupnp_control_point_new($context, "urn:schemas-upnp-org:device:MediaServer:1");
$cp_service = gupnp_control_point_new($context, "urn:schemas-upnp-org:service:ContentDirectory:1");

/* Connect to the service-found callback */
$arg_device = array('cp_device' => $cp_device, 'cp_service' => $cp_service);
$cb_device = "gupnp_device_proxy_available_cb";
gupnp_control_point_callback_set($cp_device, GUPNP_SIGNAL_DEVICE_PROXY_AVAILABLE, $cb_device, $arg_device);


//$arg_service = array('cp_service' => $cp_service);
//$cb_service = "gupnp_service_proxy_available_cb";
//gupnp_control_point_callback_set($cp_service, 
//	GUPNP_SIGNAL_SERVICE_PROXY_AVAILABLE, $cb_service, $arg_service);

/* Start for browsing */
gupnp_control_point_browse_start($cp_device);

//gupnp_control_point_browse_start($cp_service);
