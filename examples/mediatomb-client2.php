<?php

function gupnp_service_proxy_action_set_uri($proxy)
{
	$action = "SetAVTransportURI";
	$in_params = array(
		array("InstanceID", GUPNP_TYPE_INT, "0"),
		array("CurrentURI", GUPNP_TYPE_STRING, 
			"http://80.68.253.110:49153/content/media/object_id=22843&res_id=0&ext=.mp3"),
		array("CurrentURIMetaData", GUPNP_TYPE_STRING, ""),
	);
	$out_params = array();

	$result = gupnp_service_proxy_send_action($proxy, $action, $in_params, $out_params);
	printf("Result, action '%s':\n", $action);
	var_dump($result);

	printf("\n");
}

function gupnp_service_proxy_action_play($proxy)
{
	$action = "Play";
	$in_params = array(
		array("InstanceID", GUPNP_TYPE_INT, "0"),
		array("Speed", GUPNP_TYPE_STRING, "1"),
	);
	$out_params = array();

	$result = gupnp_service_proxy_send_action($proxy, $action, $in_params, $out_params);
	printf("Result, action '%s':\n", $action);
	var_dump($result);

	printf("\n");
}

function gupnp_service_proxy_action_stop($proxy)
{
	$action = "Stop";
	$in_params = array(
		array("InstanceID", GUPNP_TYPE_INT, "0"),
	);
	$out_params = array();

	$result = gupnp_service_proxy_send_action($proxy, $action, $in_params, $out_params);
	printf("Result, action '%s':\n", $action);
	var_dump($result);

	printf("\n");
}

function gupnp_service_proxy_available_cb($proxy, $arg)
{
	/* Get service info */
	$info = gupnp_service_info_get($proxy);
	
	printf("Service is available\n");
	foreach ($info as $key=>$value) {
		printf("\t%-30s: %s\n", $key, $value);
	}
	printf("\n");

	gupnp_service_proxy_action_set_uri($proxy);
	gupnp_service_proxy_action_play($proxy);
	sleep(10);
	gupnp_service_proxy_action_stop($proxy);
	gupnp_control_point_browse_stop($arg['cp_service']);
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
$cp_service = gupnp_control_point_new($context, "urn:schemas-upnp-org:service:AVTransport:1");

/* Connect to the service-found callback */
$arg_device = array('cp_device' => $cp_device, 'cp_service' => $cp_service);
$cb_device = "gupnp_device_proxy_available_cb";
gupnp_control_point_callback_set($cp_device, GUPNP_SIGNAL_DEVICE_PROXY_AVAILABLE, $cb_device, $arg_device);


/* Start for browsing */
gupnp_control_point_browse_start($cp_device);
