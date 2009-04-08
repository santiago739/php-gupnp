<?php

//$target = "urn:schemas-upnp-org:service:SwitchPower:1";

function service_cb($arg)
{
	echo "[CALLED] service_cb()\n";
	var_dump($arg);
	unset($arg);
	var_dump($arg);
}

$target = "urn:schemas-upnp-org:service:tvcontrol:1";
$cb = "service_cb";
$arg = "data";
//gupnp_control_point_new($target, $cb, $arg);
$cp = gupnp_control_point_new($target);
var_dump($cp);
gupnp_browse_service($cp, $cb, $arg);


?>
