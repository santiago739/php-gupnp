<?php

function usage()
{
	printf("Usage: test-client.php [param=value] ...\n");
}

function tvcontrol_service_proxy_available_cb($proxy, $arg)
{
	printf("Service is available\n");

	/* Get service info */
	$info = gupnp_service_info_get($proxy);
	foreach ($info as $key=>$value) {
		printf("\t%-30s: %s\n", $key, $value);
	}
	printf("\n");

	/* Subscibe to service proxy */
	$subscribed = gupnp_service_proxy_get_subscribed($proxy);
	printf("Check subscribed:\n");
	printf("\tstatus: %s\n", $subscribed ? 'true' : 'false');
	if (!$subscribed) {
		printf("Set subscribed\n");
		gupnp_service_proxy_set_subscribed($proxy, true);
	}
	/* Add notify if channel will be changed */
	if (!gupnp_service_proxy_add_notify ($proxy, "Channel", 
			GUPNP_TYPE_LONG, "tvcontrol_channel_changed_cb", NULL)) {
		printf("Failed to add notify\n");
	}
	printf("\n");

	if (isset($arg['args']['power'])) {
		tvcontrol_set_power($proxy, $arg['args']['power']);
	}
	if (isset($arg['args']['channel'])) {
		tvcontrol_set_channel($proxy, $arg['args']['channel']);
	}
}

function tvcontrol_service_proxy_unavailable_cb($proxy, $arg)
{
	printf("Service is no longer available\n");
	gupnp_control_point_browse_stop($arg['cp']);
}

function tvcontrol_device_proxy_available_cb($proxy, $arg)
{
	printf("Device is available\n");

	/* Get device info */
	$info = gupnp_device_info_get($proxy);
	foreach ($info as $key=>$value) {
		printf("\t%-30s: %s\n", $key, $value);
	}
	printf("\n");

	gupnp_control_point_browse_stop($arg['cp']);
}

function tvcontrol_device_proxy_unavailable_cb($proxy, $arg)
{
	printf("Device is no longer available\n");
	gupnp_control_point_browse_stop($arg['cp']);
}

function tvcontrol_set_power($proxy, $arg)
{
	$target = ($arg == 'on') ? true : false;
	$action = ($arg == 'on') ? 'PowerOn' : 'PowerOff';
	
	/* Set the target */
	if (!gupnp_service_proxy_action_set($proxy, $action, 'Power', $target, GUPNP_TYPE_BOOLEAN)) {
		printf("Cannot set power\n");
	} else {
		printf("Set power to %s.\n", $target ? "ON" : "OFF");
	}
	printf("\n");
}

function tvcontrol_set_channel($proxy, $arg)
{
	$channel_new = (int)$arg;
	
	/* Set the target */
	if (!gupnp_service_proxy_action_set($proxy, 'SetChannel', 'Channel', $channel_new, GUPNP_TYPE_LONG)) {
		printf("Cannot set channel\n");
	} else {
		printf("Set channel to %d.\n", $channel_new);
	}
	printf("\n");
}

function tvcontrol_channel_changed_cb($variable, $value, $arg)
{
	printf("Channel has been changed\n");
	printf("\tvariable name: %s\n", $variable);
	printf("\tvalue: %s\n", $value);
	printf("\n");
}

/* Check and parse command line arguments */
if (count($argv) == 1) {
	usage();
	exit(-1);
}

$args = array();
for ($i=1; $i<count($argv); $i++) {
	$tmp_args = explode('=', $argv[$i]);
	$args[$tmp_args[0]] = $tmp_args[1];
}

/* Create the UPnP context */
$context = gupnp_context_new();
if (!$context) {
	printf("Error creating the GUPnP context\n");
	exit(-1);
}

/* Create the control point, searching for SwitchPower services */
$cp_tvdevice = gupnp_control_point_new($context, "urn:schemas-upnp-org:device:tvdevice:1");
$cp_tvcontrol = gupnp_control_point_new($context, "urn:schemas-upnp-org:service:tvcontrol:1");

/* Connect to the service-found callback */
$arg = array('args' => $args, 'cp' => $cp_tvdevice);

$cb = "tvcontrol_device_proxy_available_cb";
gupnp_control_point_callback_set($cp_tvdevice, GUPNP_SIGNAL_DEVICE_PROXY_AVAILABLE, $cb, $arg);

$cb = "tvcontrol_device_proxy_unavailable_cb";
gupnp_control_point_callback_set($cp_tvdevice, GUPNP_SIGNAL_DEVICE_PROXY_UNAVAILABLE, $cb, $arg);


$arg = array('args' => $args, 'cp' => $cp_tvcontrol);

$cb = "tvcontrol_service_proxy_available_cb";
gupnp_control_point_callback_set($cp_tvcontrol, GUPNP_SIGNAL_SERVICE_PROXY_AVAILABLE, $cb, $arg);

$cb = "tvcontrol_service_proxy_unavailable_cb";
gupnp_control_point_callback_set($cp_tvcontrol, GUPNP_SIGNAL_SERVICE_PROXY_UNAVAILABLE, $cb, $arg);

/* Start for browsing */
gupnp_control_point_browse_start($cp_tvdevice);
gupnp_control_point_browse_start($cp_tvcontrol);
