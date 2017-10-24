#!/bin/sh

uci set wireless.sta.ssid=ASUS_24G
uci set wireless.sta.encryption=psk2
uci set wireless.sta.key=00000000
uci set wireless.sta.disabled=0
uci commit
wifi

