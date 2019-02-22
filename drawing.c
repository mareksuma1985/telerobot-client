#include <cairo.h>
#include <gtk/gtk.h>
#include <math.h>

float sensor_x = 0;
float sensor_y = 0;
float sensor_z = 10;

double roll_rads;
double pitch_rads;

static void draw_mask(cairo_t *cr, int width, int height)
{
GdkPixbuf *mask = gdk_pixbuf_new_from_file ("mask.svg", NULL);
GdkPixbuf *mask_scaled = gdk_pixbuf_scale_simple(mask, width, height, GDK_INTERP_BILINEAR);
gdk_cairo_set_source_pixbuf(cr, mask_scaled, 0, 0);
cairo_rectangle(cr, 0, 0, width, height);
cairo_fill(cr);
}

static void draw_ground(cairo_t *cr, GtkWidget *widget) {
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);

	/* paint background blue */
	cairo_set_source_rgb(cr, 0.16, 0.466, 0.937);
	cairo_paint(cr);

	/* draw ground */
	cairo_set_source_rgb(cr, 0.537, 0.278, 0);
	cairo_set_line_width(cr, 1);

	cairo_move_to(cr, 0, height / 2 + (sensor_y / sensor_z) * width / 2 - (sensor_x / sensor_z) * width / 2);
	cairo_line_to(cr, width, height / 2 + (sensor_y / sensor_z) * width / 2 + (sensor_x / sensor_z) * width / 2);
	cairo_line_to(cr, width, height);
	cairo_line_to(cr, 0, height);
	cairo_close_path(cr);
	cairo_stroke_preserve(cr);
	cairo_fill(cr);

	draw_mask(cr, width, height);
}

static void draw_nivel(cairo_t *cr, GtkWidget *widget) {
	int width = gtk_widget_get_allocated_width(widget);
	int height = gtk_widget_get_allocated_height(widget);

	/* draw aircraft attitude symbol */
	cairo_set_source_rgb(cr, 1, 1, 1);
	cairo_move_to(cr, width / 8, height / 2);
	cairo_line_to(cr, 7 * width / 8, height / 2);
	cairo_set_line_width(cr, 4);
	cairo_stroke(cr);
}

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
	draw_ground(cr, widget);
	draw_nivel(cr, widget);
	return FALSE;
}
