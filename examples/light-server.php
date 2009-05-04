<?php

/* SetTarget */
function set_target_cb($service, $action, $arg)
{
	/* Get the new target value */
	$target = gupnp_service_action_get($action, 'NewTargetValue', GUPNP_TYPE_BOOLEAN);

	/* If the new target doesn't match the current status, change the status and
	   emit a notification that the status has changed. */
	if ($target != $GLOBALS['status']) {
		$GLOBALS['status'] = $target;
		gupnp_service_notify($service, 'Status', GUPNP_TYPE_BOOLEAN, $GLOBALS['status']);
		printf("The light is now %s.\n", $GLOBALS['status'] ? "on" : "off");
	}

	/* Return success to the client */
	gupnp_service_action_return($action);
}

/* GetTarget */
function get_target_cb($service, $action, $arg)
{
	gupnp_service_action_set($action, 'RetTargetValue', GUPNP_TYPE_BOOLEAN, $GLOBALS['status']);
	gupnp_service_action_return($action);
}

/* GetStatus */
function get_status_cb($service, $action, $arg)
{
	gupnp_service_action_set($action, 'ResultStatus', GUPNP_TYPE_BOOLEAN, $GLOBALS['status']);
	gupnp_service_action_return($action);
}

/*
 * State Variable query handlers
 */

/* Target */
function query_target_cb($service, $variable, $value, $arg)
{
  //g_value_init (value, G_TYPE_BOOLEAN);
  //g_value_set_boolean (value, status);
	printf("[CALL] query_target_cb");
}

/* Status */
function query_status_cb($service, $variable, $value, $arg)
{
  //g_value_init (value, G_TYPE_BOOLEAN);
  //g_value_set_boolean (value, status);
	printf("[CALL] query_status_cb");
}


/* By default the light is off */
$GLOBALS['status'] = false;
printf("The light is now %s.\n", $GLOBALS['status'] ? "on" : "off");

/* Create the UPnP context */
$context = gupnp_context_new();
if (!$context) {
	printf("Error creating the GUPnP context\n");
	exit(-1);
}

/* Host the device and service description files */
$local_path_1 = "./BinaryLight1.xml";
$local_path_2 = "./SwitchPower1.xml";
$server_path_1 = "/BinaryLight1.xml";
$server_path_2 = "/SwitchPower1.xml";
gupnp_context_host_path($context, $local_path_1, $server_path_1);
gupnp_context_host_path($context, $local_path_2, $server_path_2);

/* Create root device */
$dev = gupnp_root_device_new($context, $server_path_1);
gupnp_root_device_set_available($dev, true);

/* Get the switch service from the root device */
$service_type = "urn:schemas-upnp-org:service:SwitchPower:1";
$service = gupnp_device_info_get_service($dev, $service_type);
if (!$service) {
	die("Cannot get SwitchPower1 service\n");
}

/* Set callback for action GetStatus */
gupnp_device_action_callback_set($service, GUPNP_SIGNAL_ACTION_INVOKED, "GetStatus", 
	"get_status_cb", "action data, GetStatus");

/* Set callback for action GetTarget */
gupnp_device_action_callback_set($service, GUPNP_SIGNAL_ACTION_INVOKED, "GetTarget", 
	"get_target_cb", "action data, GetTarget");

/* Set callback for action SetTarget */
gupnp_device_action_callback_set($service, GUPNP_SIGNAL_ACTION_INVOKED, "SetTarget", 
	"set_target_cb", "action data, SetTarget");

/* Run the main loop */
gupnp_root_device_start($dev);

