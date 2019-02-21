#include "window.h"

void disable_Fullscreen() {
	gtk_widget_hide(fullscreen_disable_button);
	gtk_widget_show(fullscreen_enable_button);
	g_print("drawing \"fullscreen_enable_button\"\n");
	gtk_window_unfullscreen(GTK_WINDOW(main_window));
	isFullscreen = FALSE;
}
void enable_Fullscreen() {
	gtk_widget_hide(fullscreen_enable_button);
	gtk_widget_show(fullscreen_disable_button);
	g_print("drawing \"fullscreen_disable_button\"\n");
	gtk_window_fullscreen(GTK_WINDOW(main_window));
	isFullscreen = TRUE;
}

/* switches between windowed and fullscreen mode */
void toggle_Fullscreen() {
	if (isFullscreen) {
		disable_Fullscreen();
	} else {
		enable_Fullscreen();
	}
}

void start_GUI() {
	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(main_window), "telerobot client");
	gtk_window_set_icon_from_file(GTK_WINDOW(main_window), "icons/PlayStation_3_gamepad.svg", NULL);

	/* gdk_window_set_icon_list(GTK_WINDOW(main_window), NULL); */
	gtk_window_set_default_size(GTK_WINDOW(main_window), 973, 309); /* actually 1024, 254 */
	gtk_window_set_position(GTK_WINDOW(main_window), GTK_WIN_POS_CENTER);
	/* gtk_widget_show(main_window); */
	left_table = gtk_table_new(6, 3, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(left_table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(left_table), 2);

	provider = gtk_css_provider_new();
	/* gtk_css_provider_load_from_path(cssProvider,"./style.css", NULL); */
	gtk_css_provider_load_from_data(GTK_CSS_PROVIDER(provider), "GtkProgressBar {\n"
			"min-height: 30px;\n"
			"}\n", -1, NULL);

	display = gdk_display_get_default();
	screen = gdk_display_get_default_screen(display);
	gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	g_object_unref(provider);

	progress1 = gtk_progress_bar_new();
//gtk_progress_bar_set_inverted(progress1, TRUE);
	/* this would invert progressbar direction */
	gtk_progress_bar_set_text(progress1, "L2:");
	gtk_progress_bar_set_show_text(progress1, TRUE);
	/*g_object_set(progress1, "min-height", 30, NULL);*/
	gtk_table_attach(GTK_TABLE(left_table), progress1, 0, 3, 0, 1, GTK_FILL, GTK_EXPAND, 0, 0);
	gtk_widget_set_valign(GTK_WIDGET(progress1), GTK_ALIGN_CENTER);

	progress2 = gtk_progress_bar_new();
	gtk_progress_bar_set_text(progress2, "R2:");
	gtk_progress_bar_set_show_text(progress2, TRUE);
	/*g_object_set(progress2, "min-height", 30, NULL);*/
	gtk_table_attach(GTK_TABLE(left_table), progress2, 0, 3, 1, 2, GTK_FILL, GTK_EXPAND, 0, 0);
	gtk_widget_set_valign(GTK_WIDGET(progress2), GTK_ALIGN_CENTER);

	progress3 = gtk_progress_bar_new();
	gtk_progress_bar_set_show_text(progress3, TRUE);
	gtk_table_attach(GTK_TABLE(left_table), progress3, 0, 3, 2, 3, GTK_FILL, GTK_EXPAND, 0, 0);
	gtk_widget_set_valign(GTK_WIDGET(progress3), GTK_ALIGN_CENTER);

	progress4 = gtk_progress_bar_new();
	gtk_progress_bar_set_show_text(progress4, TRUE);
	gtk_table_attach(GTK_TABLE(left_table), progress4, 0, 3, 3, 4, GTK_FILL, GTK_EXPAND, 0, 0);
	gtk_widget_set_valign(GTK_WIDGET(progress4), GTK_ALIGN_CENTER);

	progress5 = gtk_progress_bar_new();
	gtk_progress_bar_set_show_text(progress5, TRUE);
	gtk_table_attach(GTK_TABLE(left_table), progress5, 0, 3, 4, 5, GTK_FILL, GTK_EXPAND, 0, 0);
	gtk_widget_set_valign(GTK_WIDGET(progress5), GTK_ALIGN_CENTER);

	/* feedback labels */
	label_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

	label_roll = gtk_label_new("roll (\u00B0)");
	gtk_box_pack_start(GTK_WIDGET(label_box), label_roll, FALSE, FALSE, 10);

	label_pitch = gtk_label_new("pitch (\u00B0)");
	gtk_box_pack_start(GTK_WIDGET(label_box), label_pitch, FALSE, FALSE, 10);

	label_alt = gtk_label_new("altitude (m)");
	gtk_box_pack_start(GTK_WIDGET(label_box), label_alt, FALSE, FALSE, 10);

	gtk_table_attach(GTK_TABLE(left_table), label_box, 0, 3, 5, 6, GTK_FILL, GTK_EXPAND, 0, 0);
	gtk_widget_set_valign(GTK_WIDGET(label_box), GTK_ALIGN_CENTER);

	left_frame = gtk_aspect_frame_new(NULL, /* label */
	0.5, /* horizontal position */
	0.0, /* vertical position */
	1, /* xsize/ysize = 1 */
	FALSE /* ignore child's aspect */);

	drawing_area_frame = gtk_aspect_frame_new(NULL, 0.5, 0.0, 1, TRUE /* ignore child's aspect */);
	gtk_frame_set_shadow_type(GTK_FRAME(drawing_area_frame), GTK_SHADOW_ETCHED_IN);

	GtkWidget *text_area_frame;
	text_area_frame = gtk_aspect_frame_new(NULL, 0.5, 0.0, 1, TRUE);
	gtk_frame_set_shadow_type(GTK_FRAME(text_area_frame), GTK_SHADOW_ETCHED_IN);

	drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawing_area, 320, 320);
	gtk_container_add(GTK_CONTAINER(drawing_area_frame), drawing_area);

	g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(on_draw_event), NULL);
	g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	gtk_frame_set_shadow_type(GTK_FRAME(left_frame), GTK_SHADOW_ETCHED_IN);
	/* draws a frame */
	gtk_container_add(GTK_CONTAINER(left_frame), left_table);
	/* places the widget inside frame */
	main_table = gtk_table_new(1, 3, FALSE);
	/* creates three column table */
	gtk_table_attach_defaults(GTK_TABLE(main_table), left_frame, 0, 1, 0, 1);
	/* places the widget inside leftmost cell of the table */
	gtk_table_attach_defaults(GTK_TABLE(main_table), drawing_area_frame, 1, 2, 0, 1);
	/* places the video/artificial horizon widget inside middle cell */

	/* prawa komórka głównej tabeli */
// grid method
// right_table = gtk_grid_new();
// table method
	right_table = gtk_table_new(3, 3, TRUE);

	/* prawa ramka */
	right_frame = gtk_aspect_frame_new(NULL, /* label */
	0.5, /* wyrównanie w poziomie */
	0.0, /* wyrównanie w pionie */
	1, /* xsize/ysize = 1 */
	FALSE /* ignore child's aspect */);

	/* zamiast "0" może być NULL - wtedy pole jest puste */
	label_x1 = gtk_label_new("");
	label_y1 = gtk_label_new("");
	label_x2 = gtk_label_new("");
	label_y2 = gtk_label_new("");

	/* 0 - pierwsza kolumna, 1 - druga kolumna następna (numerowane od zera) */

	/* umieszcza wskaźniki w polach tabeli */
	gtk_label_set_justify(label_x1, GTK_JUSTIFY_RIGHT);
	gtk_label_set_justify(label_y1, GTK_JUSTIFY_RIGHT);
	/* "0,2,Y,Y" oznacza że zajmują dwie komórki tabeli */

	/* typ obwiedni */
	gtk_frame_set_shadow_type(GTK_FRAME(right_frame), GTK_SHADOW_ETCHED_IN);
	/* umieszcza right_table w wskazniki */
	gtk_container_add(GTK_CONTAINER(right_frame), right_table);

	gtk_table_attach_defaults(GTK_TABLE(main_table), right_frame, 2, 3, 0, 1);
	/* umieszcza ramkę wskazniki w prawej (2,3) komórce głównej tabeli */

	/* tworzy przycisk wyłączający serwer */
	/*
	 GtkWidget *quit_icon;
	 GtkWidget *quit_button;

	 quit_icon = gtk_image_new_from_file("icons/system-shutdown.svg");
	 quit_button = gtk_button_new();
	 gtk_container_add(GTK_CONTAINER(quit_button), quit_icon);
	 g_signal_connect(quit_button, "pressed", G_CALLBACK(wyslij_przycisk_13_on), NULL);
	 g_signal_connect(quit_button, "released", G_CALLBACK(wyslij_przycisk_13_off), NULL);
	 gtk_button_set_relief(quit_button, GTK_RELIEF_NONE);
	 */

	/* Sets whether the button will grab focus when it is clicked with the mouse.
	 Making mouse clicks not grab focus is useful in places like toolbars
	 where you don't want the keyboard focus removed from the main area of the application. */
	/* gtk_button_set_focus_on_click(quit_button, 0); */

	/* regulator & servo controller buttons */
	servocontroller_enable_button = gtk_button_new_with_label("enable\nUSC-16");
	gtk_button_set_relief(servocontroller_enable_button, GTK_RELIEF_NONE);
	gtk_table_attach_defaults(GTK_TABLE(right_table), servocontroller_enable_button, 0, 1, 2, 3);

	servocontroller_disable_button = gtk_button_new_with_label("disable\nUSC-16");
	gtk_button_set_relief(servocontroller_disable_button, GTK_RELIEF_NONE);
	gtk_table_attach_defaults(GTK_TABLE(right_table), servocontroller_disable_button, 0, 1, 2, 3);

	check_button_regulator = gtk_check_button_new_with_label("50 Hz");
	gtk_toggle_button_set_active(check_button_regulator, FALSE);
	gtk_table_attach_defaults(GTK_TABLE(right_table), check_button_regulator, 1, 2, 2, 3);

	/* record buttons */
	record_start_icon = gtk_image_new_from_file("icons/media-record.svg");
	record_start_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(record_start_button), record_start_icon);
	gtk_table_attach_defaults(GTK_TABLE(right_table), record_start_button, 2, 3, 1, 2);
	gtk_button_set_relief(GTK_BUTTON(record_start_button), GTK_RELIEF_NONE);

	record_stop_icon = gtk_image_new_from_file("icons/media-playback-stop.svg");
	record_stop_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(record_stop_button), record_stop_icon);
	gtk_table_attach_defaults(GTK_TABLE(right_table), record_stop_button, 2, 3, 1, 2);
	gtk_button_set_relief(GTK_BUTTON(record_stop_button), GTK_RELIEF_NONE);

	/* fullscreen buttons */
	fullscreen_enable_icon = gtk_image_new_from_file("icons/view-fullscreen.svg");
	fullscreen_enable_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(fullscreen_enable_button), fullscreen_enable_icon);
	g_signal_connect(fullscreen_enable_button, "clicked", G_CALLBACK(enable_Fullscreen), NULL);
	gtk_button_set_relief(fullscreen_enable_button, GTK_RELIEF_NONE);

	fullscreen_disable_icon = gtk_image_new_from_file("icons/view-windowed.svg");
	fullscreen_disable_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(fullscreen_disable_button), fullscreen_disable_icon);
	g_signal_connect(fullscreen_disable_button, "clicked", G_CALLBACK(disable_Fullscreen), NULL);
	gtk_button_set_relief(fullscreen_disable_button, GTK_RELIEF_NONE);

	/* joystick button */
// https://commons.wikimedia.org/wiki/File:PlayStation_3_gamepad.svg
	joystick_icon = gtk_image_new_from_file("icons/PlayStation_3_gamepad.svg");
	check_button_joystick = gtk_check_button_new();
	/* gtk_container_add (GTK_CONTAINER (check_button_joystick), joystick_icon); */
	/* łączenie sygnału przeniesione za pierwsze wykrycie joysticka */

	/* video button */
	video_icon = gtk_image_new_from_file("icons/camera-web.svg");
	check_button_video = gtk_check_button_new();
	gtk_toggle_button_set_active(check_button_video, TRUE);

	/* reconnect button */
	reconnect_icon = gtk_image_new_from_file("icons/gnome-modem.svg");
	reconnect_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(reconnect_button), reconnect_icon);
	gtk_button_set_relief(reconnect_button, GTK_RELIEF_NONE);

	/* table method */
	gtk_table_attach_defaults(GTK_TABLE(right_table), label_x1, 0, 1, 0, 1);
	gtk_table_attach(GTK_TABLE(right_table), label_y1, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 20);
	gtk_table_attach_defaults(GTK_TABLE(right_table), label_x2, 1, 2, 0, 1);
	gtk_table_attach(GTK_TABLE(right_table), label_y2, 1, 2, 1, 2, GTK_FILL, GTK_FILL, 0, 20);
	gtk_table_attach_defaults(GTK_TABLE(right_table), reconnect_button, 2, 3, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(right_table), fullscreen_enable_button, 2, 3, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(right_table), fullscreen_disable_button, 2, 3, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(right_table), video_icon, 1, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(right_table), check_button_video, 1, 2, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(right_table), joystick_icon, 0, 1, 3, 4);
	gtk_table_attach_defaults(GTK_TABLE(right_table), check_button_joystick, 0, 1, 3, 4);

	/* grid method */
// FIXME: works only with GTK+ 3.0
	/*
	 gtk_grid_attach(GTK_GRID(right_table), label_x, 0, 0, 2, 1);
	 gtk_grid_attach(GTK_GRID(right_table), label_y, 0, 1, 2, 1);
	 gtk_grid_attach(GTK_GRID(right_table), label_x2, 1, 0, 2, 1);
	 gtk_grid_attach(GTK_GRID(right_table), label_y2, 1, 1, 2, 1);
	 gtk_grid_attach(GTK_GRID(right_table), fullscreen_enable_button, 2, 2, 1, 1);
	 gtk_grid_attach(GTK_GRID(right_table), fullscreen_disable_button, 2, 2, 1, 1);
	 gtk_grid_attach(GTK_GRID(right_table), video_icon, 1, 3, 1, 1);
	 gtk_grid_attach(GTK_GRID(right_table), check_button_video, 1, 3, 1, 1);
	 gtk_grid_attach(GTK_GRID(right_table), joystick_icon, 0, 3, 1, 1);
	 gtk_grid_attach(GTK_GRID(right_table), check_button_joystick, 0, 3, 1, 1);
	 gtk_grid_attach(GTK_GRID(right_table), check_button_serwokontroler, 0, 2, 1, 1);
	 gtk_grid_attach(GTK_GRID(right_table), check_button_regulator, 1, 2, 1, 1);
	 gtk_grid_attach(GTK_GRID(right_table), quit_button, 2, 3, 1, 1);
	 */

	statusbar = gtk_statusbar_new();
	context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar), "Statusbar example");

	/* pakowanie głównych elementów okna programu: */
	vbox = gtk_vbox_new(FALSE, 2);

	/* ustawianie szerokości marginesu wokół elementu */
	gtk_box_pack_start(GTK_BOX(vbox), main_table, TRUE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, TRUE, 1);

	gtk_container_set_border_width(GTK_CONTAINER(main_table), 5);

	gtk_container_add(GTK_CONTAINER(main_window), vbox);
}

