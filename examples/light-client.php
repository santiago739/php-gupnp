<?php

function usage()
{
    printf("Usage: light-client.php [on|off|toggle]\n");
}

function service_proxy_available_cb($proxy, $arg)
{
	$mode = $arg['mode'];

	printf("Set subscribed\n");
	gupnp_service_proxy_set_subscribed($proxy, true);

	/* Add notify if status will be changed */
	if (!gupnp_service_proxy_add_notify($proxy, "Status", 
			GUPNP_TYPE_BOOLEAN, "status_changed_cb", $arg)) {
		printf("Failed to add notify\n");
	}
	
	$introspection = gupnp_service_info_get_introspection($proxy);
	$status_val = gupnp_service_introspection_get_state_variable($introspection, 'Status');
	printf("Status state value:\n");
	var_dump($status_val);

	if ($mode == 'TOGGLE') {
		/* We're toggling, so first fetch the current status */
		$target = gupnp_service_proxy_action_get($proxy, 'GetStatus', 'ResultStatus', GUPNP_TYPE_BOOLEAN);

		/* And then toggle it */
		$target = ! $target;
	} else {
		/* Mode is a boolean, so the target is the mode thanks to our well chosen
	   	enumeration values. */
		$target = ($mode == 'ON') ? true : false;
	}

	/* Set the target */
	if (!gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', $target, GUPNP_TYPE_BOOLEAN)) {
		printf("Cannot set switch\n");
	} else {
		printf("Set switch to %s.\n", $target ? "on" : "off");
	}
}

function status_changed_cb($variable, $value, $arg)
{
	printf("Status has been changed\n");
	printf("\tvariable name: %s\n", $variable);
	printf("\tvalue: %s\n", (int)$value);
	printf("\n");
	gupnp_control_point_browse_stop($arg['cp']);
}

/* Check and parse command line arguments */
if (count($argv) != 2) {
	usage();
	exit(-1);
}

if ($argv[1] == "on") {
	$mode = 'ON';
} elseif ($argv[1] == "off") {
	$mode = 'OFF';
} elseif ($argv[1] == "toggle") {
	$mode = 'TOGGLE';
} else {
	usage ();
	exit(-1);
}

/* Create the UPnP context */
$context = gupnp_context_new();
if (!$context) {
	printf("Error creating the GUPnP context\n");
	exit(-1);
}

/* Create the control point, searching for SwitchPower services */
$cp = gupnp_control_point_new ($context,
		"urn:schemas-upnp-org:service:SwitchPower:1");

/* Connect to the service-found callback */
$cb = "service_proxy_available_cb";
$arg = array('mode' => $mode, 'cp' => $cp);
gupnp_control_point_callback_set($cp, GUPNP_SIGNAL_SERVICE_PROXY_AVAILABLE, $cb, $arg);

/* Start for browsing */
gupnp_control_point_browse_start($cp);
