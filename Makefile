CFLAGS=`pkg-config --cflags gtk+-2.0 gdk-pixbuf-2.0 cairo gstreamer-0.10 gstreamer-interfaces-0.10`
LIBS=`pkg-config --libs gtk+-2.0 gdk-pixbuf-2.0 cairo gstreamer-0.10 gstreamer-interfaces-0.10`


all: telerobot_client

telerobot_client: telerobot_client.c
	gcc $(CFLAGS) $< -o $@ $(LIBS)

clean:
	rm -f *.o telerobot_client
