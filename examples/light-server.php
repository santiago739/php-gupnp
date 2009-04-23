<?php

/* By default the light is off */
$status = false;
printf("The light is now %s.\n", $status ? "on" : "off");



/* Create the UPnP context */
$context = gupnp_context_new();
if (!$context) {
	printf("Error creating the GUPnP context\n");
	exit(-1);
}

/* Host the device and service description files */
$local_path_1 = $_SERVER["PWD"] . "/ext/gupnp/examples/BinaryLight1.xml";
$local_path_2 = $_SERVER["PWD"] . "/ext/gupnp/examples/SwitchPower1.xml";
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
