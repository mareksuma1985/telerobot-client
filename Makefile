CFLAGS=`pkg-config --cflags gtk+-3.0 gstreamer-1.0 cairo`
LIBS=`pkg-config --libs gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0 gstreamer-base-1.0 cairo`

all: telerobot_client

telerobot_client: telerobot_client.c
	gcc $(CFLAGS) $< -o $@ $(LIBS) -I "/usr/include/cairo" -I "/usr/include/gtk-3.0" -I "/usr/include/gstreamer-1.0" -lm -w

clean:
	rm -f *.o telerobot_client
