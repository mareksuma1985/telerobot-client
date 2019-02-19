#include <cairo.h>
#include <gtk/gtk.h>
#include <math.h>

double roll_rads;
int pitch_pixels;

static void draw_ground(cairo_t *cr, GtkWidget *widget)
{
int width, height;
width = gtk_widget_get_allocated_width(widget);
height = gtk_widget_get_allocated_height(widget);

/* paint background blue */
 cairo_set_source_rgb(cr, 0.16, 0.47, 0.94);
 cairo_paint (cr);
  
/* draw brown ground */  
  cairo_set_source_rgb(cr, 0.54, 0.28, 0);
  cairo_set_line_width(cr, 1);

  cairo_move_to(cr, 0, height/2 + pitch_pixels - roll_rads * 90); // upper left verticle
  cairo_line_to(cr, width, height/2 + pitch_pixels + roll_rads * 90); // upper right verticle
  cairo_line_to(cr, width, height); // lower right corner
  cairo_line_to(cr, 0, height); // lower left corner
  cairo_close_path(cr);
  cairo_stroke_preserve(cr);
  cairo_fill(cr);
}

static void draw_symbol(cairo_t *cr, GtkWidget *widget)
{
int width, height;
width = gtk_widget_get_allocated_width(widget);
height = gtk_widget_get_allocated_height(widget);

/* draw aircaft attitude symbol */
cairo_set_source_rgb (cr, 1, 1, 1);
cairo_move_to (cr, width/8, height/2);
cairo_line_to (cr, 7*width/8, height/2);
cairo_set_line_width (cr, 4);
cairo_stroke (cr);
}

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
  draw_ground(cr, widget);
  draw_symbol(cr, widget);
  return FALSE;
}
