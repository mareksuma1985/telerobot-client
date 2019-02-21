#include <gtk/gtk.h>

GtkWidget *main_window;
GtkWidget *vbox; /* vertical division into main area and statusbar */
GtkWidget *main_table; /* three column table */
GtkWidget *statusbar;
GtkWidget *left_table; /* table inside leftmost cell of the main table */
GtkWidget *left_frame;
GtkWidget *right_table; /* (table or grid) inside rightmost cell of the main table */
GtkWidget *right_frame;

/* icons */
GtkWidget *reconnect_icon, *fullscreen_enable_icon, *fullscreen_disable_icon, *video_icon, *joystick_icon, *record_start_icon, *record_stop_icon;
/* buttons */
GtkWidget *reconnect_button, *fullscreen_enable_button, *fullscreen_disable_button, *record_start_button, *record_stop_button,
		*servocontroller_enable_button, *servocontroller_disable_button;
/* checkboxes */
GtkToggleButton *check_button_video, *check_button_joystick, *check_button_regulator;
/* FIXME: warning shows up regardless if it's GtkToggleButton or GtkWidget */

/* axis values written as strings */
char wychylenie_x[6];
char wychylenie_y[6];
char wychylenie_2[6];
char wychylenie_x2[6];
char wychylenie_y2[6];
char wychylenie_5[6];
char wychylenie_6[6];
char wychylenie_7[6];

char alt_bar_string[7];

short angles_feedback[3] = { 0, 0, 0 };

char angle_roll_string[6];
char angle_pitch_string[6];
char target_pitch_string[6];
char elevator_trim_string[6];
char rudder_trim_string[6];

GtkLabel *label_x1, *label_y1, *label_x2, *label_y2, *label_roll, *label_pitch, *label_alt;
GtkWidget *label_box;
GtkProgressBar *progress1, *progress2, *progress3, *progress4, *progress5;

/* required for statusbar */
gint context_id;
/* statusbar content */
char* message;

/* CSS */
GtkCssProvider *provider;
GdkDisplay *display;
GdkScreen *screen;

void disable_Fullscreen();
void enable_Fullscreen();
void toggle_Fullscreen();
void start_GUI();
