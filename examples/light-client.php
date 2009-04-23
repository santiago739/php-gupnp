<?php

function usage()
{
    printf("Usage: light-client.php [on|off|toggle]\n");
}

function service_proxy_available_cb($proxy, $arg)
{
	$mode = $arg;

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
$arg = $mode;
gupnp_browse_service($cp, $cb, $arg);
