#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include "config.c"
#include "video_receive.c"
#include "rumble.h"
#include "rumble.c"
#include "drawing.c"
#include "network_client.c"
#include "window.c"

#define JS_EVENT_BUTTON         0x01    /* button pressed/released	*/
#define JS_EVENT_AXIS           0x02    /* joystick moved			*/
#define JS_EVENT_INIT           0x80    /* initial state of device	*/

int joystick_fd, rc;

struct js_event {
/** http://www.psdevwiki.com/ps4/DS4-USB */
unsigned int time; /* event timestamp in milliseconds */
short value; /* value */
unsigned char type; /* event type */
unsigned char number; /* axis or button number */
};

gboolean buttons_ever_pressed = FALSE;
gboolean axis_r2_ever_moved = FALSE;
gboolean video_running = TRUE;

gboolean pierwsze_wykrywanie_joysticka = TRUE;
gboolean r2_pressed = FALSE;
struct js_event jse;

int stick_x = 0; /*left stick, horizontal*/
int stick_y = 0; /*left stick, vertical*/
int stick_l2 = 0; /*left analog trigger*/
int stick_x2 = 0; /*right stick, horizontal*/
int stick_y2 = 0; /*right stick, vertical*/
int stick_r2 = 0; /*right analog trigger*/
int stick_6 = 0; /*D-pad horizontal*/
int stick_7 = 0; /*D-pad vertical*/

int throttle = -32768;
short elevator_trim_value = 0;
short rudder_trim_value = 0;

char rumbledevicestring[PATH_MAX];
char *rumbledevice = NULL;

int poz_serwo_01;
int poz_serwo_02;

gboolean isRecording = FALSE;
gboolean isShining = FALSE;
gboolean isExponential = FALSE;
