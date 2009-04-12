<?php

function service_cb($proxy, $arg)
{
	echo "[CALLED] service_cb()\n";

	global $proxy1;

	$proxy1 = $proxy;

	var_dump($proxy);
	var_dump($arg);

	//unset($proxy);
	unset($arg);

	//var_dump($proxy);
	var_dump($arg);

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_info_get() \n";
	echo "---------------------------------------------------------\n";
	$info = gupnp_service_info_get($proxy);
	echo "[RESULT]: ";
	var_dump($info);
	echo "---------------------------------------------------------\n\n";
	
	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_send_action() \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_send_action($proxy, "SetTarget", "NewTargetValue", true, GUPNP_TYPE_BOOLEAN);
	echo "[RESULT]: ";
	var_dump($res);
	$res = gupnp_service_proxy_send_action($proxy, "SetTarget", "NewTargetValue", false, GUPNP_TYPE_BOOLEAN);
	echo "[RESULT]: ";
	var_dump($res);
	$res = gupnp_service_proxy_send_action($proxy, "SetTarget", "NewTargetValue", 1, GUPNP_TYPE_LONG);
	echo "[RESULT]: ";
	var_dump($res);
	$res = gupnp_service_proxy_send_action($proxy, "SetTarget", "NewTargetValue", 0, GUPNP_TYPE_LONG);
	echo "[RESULT]: ";
	var_dump($res);
	$res = gupnp_service_proxy_send_action($proxy, "SetTarget", "NewTargetValue", 1.1, GUPNP_TYPE_DOUBLE);
	echo "[RESULT]: ";
	var_dump($res);
	$res = gupnp_service_proxy_send_action($proxy, "SetTarget", "NewTargetValue", 0.0, GUPNP_TYPE_DOUBLE);
	echo "[RESULT]: ";
	var_dump($res);
	$res = gupnp_service_proxy_send_action($proxy, "SetTarget", "NewTargetValue", "2", GUPNP_TYPE_STRING);
	echo "[RESULT]: ";
	var_dump($res);
	$res = gupnp_service_proxy_send_action($proxy, "SetTarget", "NewTargetValue", "0", GUPNP_TYPE_STRING);
	echo "[RESULT]: ";
	var_dump($res);
	$res = gupnp_service_proxy_send_action($proxy, "SetTarget", "NewTargetValue", $proxy, GUPNP_TYPE_STRING);
	echo "[RESULT]: ";
	var_dump($res);
	$res = gupnp_service_proxy_send_action($proxy, "SetTarget2", "NewTargetValue", true, GUPNP_TYPE_BOOLEAN);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";
}

$context = gupnp_context_new();
var_dump($context);

//$target = "urn:schemas-upnp-org:service:tvcontrol:1";
$target = "urn:schemas-upnp-org:service:SwitchPower:1";
$cb = "service_cb";
$arg = "data";

echo "=========================================================\n";
echo "[CALL]: gupnp_control_point_new() \n";
echo "---------------------------------------------------------\n";
$cp = gupnp_control_point_new($context, $target);
echo "[RESULT]: ";
var_dump($cp);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_browse_service() \n";
echo "---------------------------------------------------------\n";
gupnp_browse_service($cp, $cb, $arg);
echo "=========================================================\n\n\n";

//var_dump($proxy1);

//$location = gupnp_service_info_get($proxy1);
//var_dump($location);

//$res = gupnp_service_proxy_send_action($proxy1, "SetTarget", "NewTargetValue", "0");
//var_dump($res);

?>
