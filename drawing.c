#include <cairo.h>
#include <gdk/gdk.h>

gboolean draw_callback(GtkWidget *widget, cairo_t *cr, gpointer data) {
	guint width, height;
	GdkRGBA color;

	width = gtk_widget_get_allocated_width(widget);
	height = gtk_widget_get_allocated_height(widget);
	cairo_arc(cr, width / 2.0, height / 2.0, MIN(width, height) / 2.0, 0, 2 * G_PI);

	gtk_style_context_get_color(gtk_widget_get_style_context(widget), 0, &color);
	gdk_cairo_set_source_rgba(cr, &color);

	cairo_fill(cr);

	return FALSE;
}
