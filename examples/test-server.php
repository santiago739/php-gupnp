<?php
/* PowerOn */
function power_on_cb($service, $action, $arg)
{
	gupnp_service_action_set($action, 'Power', GUPNP_TYPE_BOOLEAN, true);
	gupnp_service_action_return($action);
}

/* PowerOff */
function power_off_cb($service, $action, $arg)
{
	gupnp_service_action_set($action, 'Power', GUPNP_TYPE_BOOLEAN, false);
	gupnp_service_action_return($action);
}

/* By default the tv is off */
$GLOBALS['power'] = false;
printf("The tv is now %s.\n", $GLOBALS['power'] ? "on" : "off");

/* Create the UPnP context */
$context = gupnp_context_new();
if (!$context) {
	printf("Error creating the GUPnP context\n");
	exit(-1);
}

printf("Running server:\n");
printf("\thost/ip: %s\n", gupnp_context_get_host_ip($context));
printf("\tport: %d\n", gupnp_context_get_port($context));

/* Host current directory */
gupnp_context_host_path($context, "./web", "");

/* Create root device */
$dev = gupnp_root_device_new($context, "/tvdevicedesc.xml");

/* Get relative location */
$rel_location = gupnp_root_device_get_relative_location($dev);
printf("\trelative location: %s\n", $rel_location);

/* Subscription timeout */
$subs_timeout = gupnp_context_get_subscription_timeout($context);
printf("\tsubscription timeout: %d\n\n", $subs_timeout);

printf("Change timeout:\n");
gupnp_context_set_subscription_timeout($context, $subs_timeout + 100);
printf("\tsubscription timeout: %d\n\n", gupnp_context_get_subscription_timeout($context));

if (!gupnp_root_device_get_available) {
	gupnp_root_device_set_available($dev, true);
}

/* Get the service from the root device */
$service_tvcontrol = "urn:schemas-upnp-org:service:tvcontrol:1";
$service = gupnp_device_info_get_service($dev, $service_tvcontrol);
if (!$service) {
	printf("Cannot get %s service\n", $service_tvcontrol);
	exit(-1);
}

/* Set callback for action PowerOn*/
gupnp_device_action_callback_set($service, GUPNP_SIGNAL_ACTION_INVOKED, "PowerOn", 
	"power_on_cb", "action data, PowerOn");

/* Set callback for action PowerOff*/
gupnp_device_action_callback_set($service, GUPNP_SIGNAL_ACTION_INVOKED, "PowerOff", 
	"power_off_cb", "action data, PowerOff");

/* Run the main loop */
gupnp_root_device_start($dev);
