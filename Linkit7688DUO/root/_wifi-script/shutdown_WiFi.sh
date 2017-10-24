#!/bin/sh
uci set wireless.@wifi-device[0].disabled=1
uci commit wireless && wifi

