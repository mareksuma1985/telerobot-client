#!/bin/bash
touch ./telerobot_client.c
gcc `pkg-config --cflags gtk+-2.0 gstreamer-0.10 gstreamer-interfaces-0.10` telerobot_client.c -o telerobot_client `pkg-config --libs gtk+-2.0 gstreamer-0.10 gstreamer-interfaces-0.10`
read x
