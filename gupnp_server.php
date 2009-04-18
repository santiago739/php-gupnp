<?php

//phpinfo();

function action_cb($variable, $value, $arg)
{
	echo "=========================================================\n";
	echo "[CALL] action_cb()\n";
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
echo "[CALL]: gupnp_context_new() \n";
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
$timeout = 1000;
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

$local_path_1 = $_SERVER["PWD"] . "/ext/gupnp/examples/BinaryLight1.xml";
$server_path_1 = "/BinaryLight1.xml";

echo "=========================================================\n";
echo "[CALL]: gupnp_context_host_path($context, $local_path_1, $server_path_1) \n";
echo "---------------------------------------------------------\n";
$result = gupnp_context_host_path($context, $local_path_1, $server_path_1);
echo "[RESULT]: ";
var_dump($result);
echo "=========================================================\n\n\n";

$local_path_2 = $_SERVER["PWD"] . "/ext/gupnp/examples/SwitchPower1.xml";
$server_path_2 = "/SwitchPower1.xml";

echo "=========================================================\n";
echo "[CALL]: gupnp_context_host_path($context, $local_path_2, $server_path_2) \n";
echo "---------------------------------------------------------\n";
$result = gupnp_context_host_path($context, $local_path_2, $server_path_2);
echo "[RESULT]: ";
var_dump($result);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_root_device_new($context, $server_path_1) \n";
echo "---------------------------------------------------------\n";
$device = gupnp_root_device_new($context, $server_path_1);
echo "[RESULT]: ";
var_dump($device);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_root_device_get_available($device) \n";
echo "---------------------------------------------------------\n";
$result = gupnp_root_device_get_available($device);
echo "[RESULT]: ";
var_dump($result);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_root_device_set_available($device, true) \n";
echo "---------------------------------------------------------\n";
$result = gupnp_root_device_set_available($device, true);
echo "[RESULT]: ";
var_dump($result);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_root_device_get_available($device) \n";
echo "---------------------------------------------------------\n";
$result = gupnp_root_device_get_available($device);
echo "[RESULT]: ";
var_dump($result);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_root_device_get_relative_location($device) \n";
echo "---------------------------------------------------------\n";
$result = gupnp_root_device_get_relative_location($device);
echo "[RESULT]: ";
var_dump($result);
echo "=========================================================\n\n\n";

echo "=========================================================\n";
echo "[CALL]: gupnp_device_info_get($device) \n";
echo "---------------------------------------------------------\n";
$result = gupnp_device_info_get($device);
echo "[RESULT]: ";
var_dump($result);
echo "=========================================================\n\n\n";

$service_type = "urn:schemas-upnp-org:service:SwitchPower:1";
echo "=========================================================\n";
echo "[CALL]: gupnp_device_info_get_service($device, $service_type) \n";
echo "---------------------------------------------------------\n";
$service_info = gupnp_device_info_get_service($device, $service_type);
echo "[RESULT]: ";
var_dump($service_info);
echo "=========================================================\n\n\n";

$cb = "action_cb";
$arg = "action data";
$action_name = "action-invoked::GetStatus";
echo "=========================================================\n";
echo "[CALL]: gupnp_device_action_callback_set($service_info, $action_name, $cb, $arg) \n";
echo "---------------------------------------------------------\n";
$result = gupnp_device_action_callback_set($service_info, $action_name, $cb, $arg);
echo "[RESULT]: ";
var_dump($result);
echo "=========================================================\n\n\n";


gupnp_main_loop_run();
sleep(5);
gupnp_main_loop_stop();

?>
