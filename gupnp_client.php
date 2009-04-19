<?php

function switcher_service_cb($proxy, $arg)
{
	echo "[CALLED] switcher_service_cb()\n";

	global $proxy1;

	$proxy1 = $proxy;

	var_dump($proxy);
	var_dump($arg);

	//unset($proxy);
	unset($arg);

	//var_dump($proxy);
	var_dump($arg);

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_info_get($proxy) \n";
	echo "---------------------------------------------------------\n";
	$info = gupnp_service_info_get($proxy);
	echo "[RESULT]: ";
	var_dump($info);
	echo "---------------------------------------------------------\n\n";
	
	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_get($proxy, 'GetStatus', 'ResultStatus', GUPNP_TYPE_BOOLEAN) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_get($proxy, 'GetStatus', 'ResultStatus', GUPNP_TYPE_BOOLEAN);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";
	
	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', true, GUPNP_TYPE_BOOLEAN) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', true, GUPNP_TYPE_BOOLEAN);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_get($proxy, 'GetStatus', 'ResultStatus', GUPNP_TYPE_BOOLEAN) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_get($proxy, 'GetStatus', 'ResultStatus', GUPNP_TYPE_BOOLEAN);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";	

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', false, GUPNP_TYPE_BOOLEAN) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', false, GUPNP_TYPE_BOOLEAN);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', 1, GUPNP_TYPE_LONG) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', 1, GUPNP_TYPE_LONG);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', 0, GUPNP_TYPE_LONG) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', 0, GUPNP_TYPE_LONG);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', 1.1, GUPNP_TYPE_DOUBLE) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', 1.1, GUPNP_TYPE_DOUBLE);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', 0.0, GUPNP_TYPE_DOUBLE)) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', 0.0, GUPNP_TYPE_DOUBLE);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', '2', GUPNP_TYPE_STRING) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', '2', GUPNP_TYPE_STRING);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', '0', GUPNP_TYPE_STRING) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', '0', GUPNP_TYPE_STRING);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', $proxy, GUPNP_TYPE_STRING) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy, 'SetTarget', 'NewTargetValue', $proxy, GUPNP_TYPE_STRING);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy, 'SetTarget2', 'NewTargetValue', true, GUPNP_TYPE_BOOLEAN) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy, 'SetTarget2', 'NewTargetValue', true, GUPNP_TYPE_BOOLEAN);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_get_subscribed() \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_get_subscribed($proxy);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_set_subscribed($proxy, true) \n";
	echo "---------------------------------------------------------\n";
	gupnp_service_proxy_set_subscribed($proxy, true);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_get_subscribed($proxy) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_get_subscribed($proxy);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";
}

function tv_service_cb($proxy, $arg)
{
	echo "[CALLED] tv_service_cb()\n";

	global $proxy1;

	$proxy1 = $proxy;

	var_dump($proxy);
	var_dump($arg);

	//unset($proxy);
	unset($arg);

	//var_dump($proxy);
	var_dump($arg);

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_info_get($proxy) \n";
	echo "---------------------------------------------------------\n";
	$info = gupnp_service_info_get($proxy);
	echo "[RESULT]: ";
	var_dump($info);
	echo "---------------------------------------------------------\n\n";
	
	$value = rand(1, 100);
	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy, 'SetChannel', 'Channel', $value, GUPNP_TYPE_LONG) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy, 'SetChannel', 'Channel', $value, GUPNP_TYPE_LONG);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";
	
	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_get($proxy, 'SetChannel', 'Channel', GUPNP_TYPE_LONG) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_get($proxy, 'SetChannel', 'Channel', GUPNP_TYPE_LONG);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";
	
	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_add_notify($proxy, 'Channel', GUPNP_TYPE_LONG, $cb, $arg) \n";
	echo "---------------------------------------------------------\n";
	$cb = "notify_cb";
	$arg = "notify data, channel";
	$res = gupnp_service_proxy_add_notify($proxy, 'Channel', GUPNP_TYPE_LONG, $cb, $arg);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_add_notify($proxy, 'Power', GUPNP_TYPE_BOOLEAN, $cb, $arg) \n";
	echo "---------------------------------------------------------\n";
	$cb = "notify_cb";
	$arg = "notify data, power";
	$res = gupnp_service_proxy_add_notify($proxy, 'Power', GUPNP_TYPE_BOOLEAN, $cb, $arg);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_get_subscribed() \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_get_subscribed($proxy);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_set_subscribed($proxy, true) \n";
	echo "---------------------------------------------------------\n";
	gupnp_service_proxy_set_subscribed($proxy, true);
	echo "---------------------------------------------------------\n\n";

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_get_subscribed($proxy) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_get_subscribed($proxy);
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";

	
}

function notify_cb($variable, $value, $arg)
{
	echo "=========================================================\n";
	echo "[CALL] notify_cb()\n";
	echo "---------------------------------------------------------\n";

	echo "[VARIABLE]: ";
	var_dump($variable);
	echo "[VALUE]: ";
	var_dump($value);
	echo "[ARG]: ";
	var_dump($arg);
	echo "=========================================================\n\n\n";
}

function remove_notify_cb($variable, $value, $arg)
{
	echo "=========================================================\n";
	echo "[CALL] notify_cb()\n";
	echo "---------------------------------------------------------\n";

	echo "[VARIABLE]: ";
	var_dump($variable);
	echo "[VALUE]: ";
	var_dump($value);
	echo "[ARG]: ";
	var_dump($arg);
	echo "=========================================================\n\n\n";
}

echo "=========================================================\n";
echo "[CALL]: gupnp_control_point_new() \n";
echo "---------------------------------------------------------\n";
$context = gupnp_context_new();
echo "[RESULT]: ";
var_dump($context);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_context_get_host_ip($context) \n";
echo "---------------------------------------------------------\n";
$host_ip = gupnp_context_get_host_ip($context);
echo "[RESULT]: ";
var_dump($host_ip);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_context_get_port($context) \n";
echo "---------------------------------------------------------\n";
$port = gupnp_context_get_port($context);
echo "[RESULT]: ";
var_dump($port);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_context_get_subscription_timeout($context) \n";
echo "---------------------------------------------------------\n";
$timeout = gupnp_context_get_subscription_timeout($context);
echo "[RESULT]: ";
var_dump($timeout);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_context_set_subscription_timeout($context, $timeout) \n";
echo "---------------------------------------------------------\n";
$timeout = 100;
gupnp_context_set_subscription_timeout($context, $timeout);
echo "[RESULT]: set timeout to $timeout \n";
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_context_get_subscription_timeout($context) \n";
echo "---------------------------------------------------------\n";
$timeout = gupnp_context_get_subscription_timeout($context);
echo "[RESULT]: ";
var_dump($timeout);
echo "=========================================================\n\n\n";

//$target = "urn:schemas-upnp-org:service:tvcontrol:1";
//$cb = "tv_service_cb";
$target = "urn:schemas-upnp-org:service:SwitchPower:1";
$cb = "switcher_service_cb";
$arg = "data";

echo "=========================================================\n";
echo "[CALL]: gupnp_control_point_new($context, $target) \n";
echo "---------------------------------------------------------\n";
$cp = gupnp_control_point_new($context, $target);
echo "[RESULT]: ";
var_dump($cp);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_browse_service($cp, $cb, $arg) \n";
echo "---------------------------------------------------------\n";
gupnp_browse_service($cp, $cb, $arg);
echo "=========================================================\n\n\n";

//var_dump($proxy1);

//$location = gupnp_service_info_get($proxy1);
//var_dump($location);

//$res = gupnp_service_proxy_action_set($proxy1, 'SetTarget', 'NewTargetValue', "0");
//var_dump($res);

while(1)
{
	/*
	$value = rand(1, 100);
	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_action_set($proxy1, 'SetChannel', 'Channel', $value, GUPNP_TYPE_LONG) \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_action_set($proxy1, 'SetChannel', 'Channel', $value, GUPNP_TYPE_LONG);
	echo "[RESULT]: ";
	var_dump($res);

	sleep(5);

	echo "---------------------------------------------------------\n";
	echo "[CALL]: gupnp_service_proxy_remove_notify($proxy1, 'Channel') \n";
	echo "---------------------------------------------------------\n";
	$res = gupnp_service_proxy_remove_notify($proxy1, 'Channel');
	echo "[RESULT]: ";
	var_dump($res);
	echo "---------------------------------------------------------\n\n";
	*/
}

?>
