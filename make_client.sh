#!/bin/bash
touch ./telerobot_client.c
gcc `pkg-config --cflags gtk+-3.0 gstreamer-1.0 cairo` telerobot_client.c -o client `pkg-config --libs gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0 gstreamer-base-1.0 cairo` -I "/usr/include/cairo" -I "/usr/include/gtk-3.0" -I "/usr/include/gstreamer-1.0" -lm -w
read x
