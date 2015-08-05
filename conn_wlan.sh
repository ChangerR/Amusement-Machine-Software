#! /bin/bash

ip link set wlan0 up
wpa_supplicant -B -i wlan0 -c /opt/work/slserver/wlan.conf
wpa_cli select_network 0
sleep 1
dhclient wlan0
