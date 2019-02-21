/* Copyright © Tristan Matthews Montreal, PQ, Canada */
/* Fullscreen video in gstreamer with GTK+ */
/* http://tristanswork.blogspot.com/2008/09/fullscreen-video-in-gstreamer-with-gtk.html */

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/video/videooverlay.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <stdlib.h>

/* tworzy obszar wyświetlania obrazu i jego obramowanie */
GtkWidget *drawing_area;
GtkWidget *drawing_area_frame;

GstStateChangeReturn ret;
GstElement *pipeline, *udp_src, *parser, *decoder, *video_sink;
GstElement *pipeline_audio, *tcpclientsrc, *speexdec, *alsasink;

static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
	GstPad *sinkpad;
	GstElement *decoder = (GstElement *) data;
	g_print("Dynamic pad created, linking\n");
	sinkpad = gst_element_get_static_pad(decoder, "sink");
	gst_pad_link(pad, sinkpad);
	gst_object_unref(sinkpad);
}

gboolean handleBusMsg(GstMessage * message, GtkWidget *widget) {
	/* ignore anything but 'prepare-xwindow-id' element messages */
	if (GST_MESSAGE_TYPE(message) != GST_MESSAGE_ELEMENT)
		return FALSE;
	//g_print("Got prepare-xwindow-id msg\n");
	//gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message)), GDK_WINDOW_XID(gtk_widget_get_parent_window(widget)));
	//gst_video_overlay_set_render_rectangle(GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message)), 346, 7, 320, 320);
	gst_video_overlay_set_window_handle(GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message)), GDK_WINDOW_XID(gtk_widget_get_window(widget)));
	return TRUE;
}

gboolean bus_call(GstBus * bus, GstMessage *msg, gpointer data) {
	GtkWidget *main_window = (GtkWidget*) data;
	switch (GST_MESSAGE_TYPE(msg)) {
	case GST_MESSAGE_ELEMENT: {
		handleBusMsg(msg, drawing_area);
		break;
	}

	default:
		break;
	}
	return TRUE;
}

static void makeWindowBlack(GtkWidget * widget) {
	GdkColor color;
	gdk_color_parse("black", &color);
	gtk_widget_modify_bg(widget, GTK_STATE_NORMAL, &color);
	/* zaczernia obszar wyświetlania */
	/* needed to ensure black background */
	/* Tristan Matthews Montreal, PQ, Canada */
}

/* 
 stare polecenie bash do odbierania obrazu i dźwięku:
 gst-launch-0.10 udpsrc port=5000 ! queue ! smokedec ! queue ! autovideosink tcpclientsrc host=127.0.0.1 port=5001 ! queue ! speexdec ! queue ! alsasink sync=false

 odbieranie samego obrazu:
 gst-launch-0.10 udpsrc port=5000 ! queue ! smokedec ! queue ! autovideosink

 odbieranie samego dźwięku:
 gst-launch-0.10 tcpclientsrc host=127.0.0.1 port=5001 ! queue ! speexdec ! queue ! alsasink sync=false
 */

/* odbieranie samego obrazu:
 gst-launch-1.0 udpsrc port=5000 ! h264parse ! avdec_h264 ! autovideosink
 */

int video_receive() {
	/* create elements */
	pipeline = gst_pipeline_new("pipeline");
	udp_src = gst_element_factory_make("udpsrc", "udp-source");
	g_object_set(G_OBJECT(udp_src), "port", 5000, NULL);

	parser = gst_element_factory_make("h264parse", "parser");

	//decoder = gst_element_factory_make("avdec_h264", "decoder");
	decoder = gst_element_factory_make("vaapih264dec", "decoder");

	video_sink = gst_element_factory_make("xvimagesink", "videosink");
	g_object_set(G_OBJECT(video_sink), "sync", FALSE, NULL);
	g_object_set(G_OBJECT(video_sink), "force-aspect-ratio", TRUE, NULL);
	if (!video_sink) {
		g_print("output could not be found - check your install\n");
	}

	/* assertions */

	g_assert(pipeline != NULL);
	g_assert(udp_src != NULL);
	g_assert(parser != NULL);
	g_assert(decoder != NULL);
	g_assert(video_sink != NULL);

	gst_bus_add_watch(gst_pipeline_get_bus(GST_PIPELINE(pipeline)), (GstBusFunc) bus_call, drawing_area);
	/* źródło obrazu, ustawienie numeru portu */

	if (!g_signal_connect(udp_src, "pad-added", G_CALLBACK(on_pad_added), parser)) {
		g_print("Failed to g_signal_connect udp_src with parser!\n");
		return -1;
	}

	if (!g_signal_connect(parser, "pad-added", G_CALLBACK(on_pad_added), decoder)) {
		g_print("Failed to g_signal_connect parser with the decoder!\n");
		return -1;
	}

	/*
	 łączenie pad-ów
	 gst_pad_connect (gst_element_get_pad (udp_src, "src"), gst_element_get_pad (decoder, "sink"));
	 gst_pad_connect (gst_element_get_pad (decoder, "src"), gst_element_get_pad (video_sink, "sink"));
	 */

	gst_bin_add(GST_BIN(pipeline), udp_src);
	gst_bin_add(GST_BIN(pipeline), parser);
	gst_bin_add(GST_BIN(pipeline), decoder);
	gst_bin_add(GST_BIN(pipeline), video_sink);

	if (!gst_element_link(udp_src, parser)) {
		g_print("Error: Failed to link udp_src with the parser!\n");
		return -1;
	} else {
		g_print("Linked udp_src with parser\n");
	}

	if (!gst_element_link(parser, decoder)) {
		g_print("Error: Failed to link parser with the decoder!\n");
		return -1;
	} else {
		g_print("Linked parser with decoder\n");
	}

	if (!gst_element_link(decoder, video_sink)) {
		g_print("Error: Failed to link the decoder with video_sink!\n");
		return -1;
	} else {
		g_print("Linked decoder with video_sink\n");
	}

	if (gst_element_set_state(pipeline, GST_STATE_PAUSED)) {
		g_print("Video pipeline state set to pause\n");
	} else {
		g_print("Error: Failed to pause video pipeline!\n");
		return -1;
	}

	makeWindowBlack(drawing_area);
	/* zaczernia obraz */

	if (gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING)) {
		g_print("Video pipeline state set to playing\n");
	} else {
		g_print("Error: Failed to start up video pipeline!\n");
		return 1;
	}
	/* Tristan Matthews Montreal, PQ, Canada */
}

/* FIXME: odkomentuj jeden z fragmentów aby włączyć */
int audio_receive() {
	/* z parametrem "sync=false":

	 Ustawianie potoku na PAUZOWANIE...
	 Potok jest PRZEWIJANY...
	 Potok jest PRZEWINIĘTY...
	 Ustawianie potoku na ODTWARZANIE...
	 New clock: GstAudioSinkClock
	 */

	/* bez "sync=false":

	 Ustawianie potoku na PAUZOWANIE...
	 Potok jest PRZEWIJANY...
	 UWAGA: od elementu /GstPipeline:pipeline0/GstAlsaSink:alsasink0: Wewnętrzny problem przepływu danych.
	 Dodatkowe informacje diagnostyczne:
	 gstbasesink.c(3316): gst_base_sink_chain_unlocked (): /GstPipeline:pipeline0/GstAlsaSink:alsasink0:
	 Received buffer without a new-segment. Assuming timestamps start from 0.
	 Potok jest PRZEWINIĘTY...
	 Ustawianie potoku na ODTWARZANIE...
	 New clock: GstAudioSinkClock
	 */

	pipeline_audio = gst_pipeline_new("pipeline-audio");
	//g_object_set(G_OBJECT(pipeline_audio), "sync", TRUE, NULL);
	tcpclientsrc = gst_element_factory_make("tcpclientsrc", "udp-source");
	g_object_set(G_OBJECT(tcpclientsrc), "host", "localhost", NULL);
	g_object_set(G_OBJECT(tcpclientsrc), "port", 5001, NULL);
	speexdec = gst_element_factory_make("speexdec", "audio-decoder");
	alsasink = gst_element_factory_make("alsasink", "audio-output");

	if (!tcpclientsrc) {
		g_print("output could not be found - check your install\n");
	}

	/* assertions */
	/*
	 g_assert (pipeline_audio != NULL);
	 g_assert (tcpclientsrc != NULL);
	 g_assert (speexdec != NULL);
	 g_assert (alsasink != NULL);
	 */

	gst_bin_add(GST_BIN(pipeline_audio), tcpclientsrc);
	gst_bin_add(GST_BIN(pipeline_audio), speexdec);
	gst_bin_add(GST_BIN(pipeline_audio), alsasink);

	if (!g_signal_connect(tcpclientsrc, "pad-added", G_CALLBACK(on_pad_added), speexdec)) {
		g_print("Failed to g_signal_connect tcpclientsrc with speexdec!\n");
		/* return -1; */}

	if (!gst_element_link(tcpclientsrc, speexdec)) {
		g_print("NOGO: Failed to link tcpclientsrc with speexdec!\n");
		/* return -1; */} else {
		g_print("GO: Linked tcpclientsrc with speexdec!\n");
	}

	if (!gst_element_link(speexdec, alsasink)) {
		g_print("NOGO: Failed to link speexdec with alsasink!\n");
		/* return -1; */} else {
		g_print("GO: Linked speexdec with alsasink!\n");
	}

	if (gst_element_set_state(pipeline_audio, GST_STATE_PAUSED)) {
		g_print("GO: Audio pipeline state set to pause.\n");
	} else {
		g_print("NOGO: Failed to pause audio pipeline!\n");
		/* return -1; */}

	if (gst_element_set_state(GST_ELEMENT(pipeline_audio), GST_STATE_PLAYING)) {
		g_print("GO: Audio pipeline state set to playing.\n");
	} else {
		g_print("NOGO: Failed to start up audio pipeline!\n");
		/* return -1; */}

	return FALSE;
	/* jeśli zwraca FALSE to nie jest więcej wykonywana */
	/* jeśli zwraca cokolwiek innego to jest wykonywana ponownie */
}
