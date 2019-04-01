/* Copyright © 2012, Marek Suma

msuma@wp.pl

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation version 2 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

This version of GPL is at http://www.gnu.org/licenses/old-licenses/gpl-2.0.txt

*/

/*
requires:
sudo apt-get install libgstreamer-plugins-base1.0-dev
sudo apt-get install gstreamer1.0-plugins-bad
sudo apt-get install libgtk-3-dev
sudo apt-get install libcairo2-dev
*/

#include "telerobot_client.h"

void zamknij() {
if (tryb_wysylania == 0) {
close(sock);
g_print("Closing UDP socket\n");
}

if (tryb_wysylania == 1) {
(void) shutdown(SocketFD, SHUT_RDWR);
close(SocketFD);
g_print("Closing TCP socket\n");
}

gtk_main_quit();
}

double bytes2double(char arg1, char arg2, char arg3, char arg4, char arg5, char arg6, char arg7, char arg8)
{
char array[8] = {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8};
double value;
memcpy(&value, &array, sizeof(value));
return value;
}

float bytes2float(unsigned char arg1, unsigned char arg2, unsigned char arg3, unsigned char arg4)
{
unsigned char array[4] = {arg1, arg2, arg3, arg4};
float value;
memcpy(&value, &array, sizeof(value));
return value;
}

void feedback_event() {
buffer_in[0] = NULL;
buffer_in[1] = NULL;
buffer_in[2] = NULL;
buffer_in[3] = NULL;
buffer_in[4] = NULL;

switch (tryb_wysylania) {
case 0:
recsize = recvfrom(sock, (void *) buffer_in, 1024, 0, (struct sockaddr *) &sa, &fromlen);
break;
case 1:
recsize = recvfrom(SocketFD, (void *) buffer_in, 1024, 0, (struct sockaddr *) &sa, &fromlen);
break;
case 2:
/* no feedback */
break;
}

if (recsize < 0) {
fprintf(stderr, "%s\n", strerror(errno));
}
else if (recsize == 0)
{}
else
{
int number = buffer_in[0];
/* beginning of feedback switch */
switch (number) {
case 0:
break;

case 1:
{
/** accelerometer or gravity sensor event */
sensor_x = bytes2float(buffer_in[1], buffer_in[2], buffer_in[3], buffer_in[4]);
sensor_y = bytes2float(buffer_in[5], buffer_in[6], buffer_in[7], buffer_in[8]);
sensor_z = bytes2float(buffer_in[9], buffer_in[10], buffer_in[11], buffer_in[12]);

gtk_widget_queue_draw(drawing_area);

roll_rads = atan2((double) sensor_x, (double) sensor_z);
pitch_rads = atan2((double) sensor_y, (double) sensor_z);

sprintf(angle_roll_string, "%.2f", roll_rads * (180.0 / M_PI));
sprintf(angle_pitch_string, "%.2f", pitch_rads * (180.0 / M_PI));

gtk_label_set_text(label_roll, angle_roll_string);
gtk_label_set_text(label_pitch, angle_pitch_string);
}
break;

case 2:
/** GPS location event */
printf("Lat: %f, Lon: %f\n", bytes2double(buffer_in[1], buffer_in[2], buffer_in[3], buffer_in[4], buffer_in[5], buffer_in[6], buffer_in[7], buffer_in[8]), bytes2double(buffer_in[9], buffer_in[10], buffer_in[11], buffer_in[12], buffer_in[13], buffer_in[14], buffer_in[15], buffer_in[16]));
break;

case 3:
sprintf(alt_bar_string, "%.2f", bytes2float(buffer_in[1], buffer_in[2], buffer_in[3], buffer_in[4]));
gtk_label_set_text(label_alt, alt_bar_string);
break;

case 4:
/** video recording feedback */
if (buffer_in[1] == 0x31)
{
//0x31 hexadecimal is a '1' character
isRecording = TRUE;
gtk_widget_hide(record_start_button);
gtk_widget_show(record_stop_button);
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "recording started");
}
if (buffer_in[1] == 0x30)
{
//0x30 hexadecimal is a '0' character
isRecording = FALSE;
gtk_widget_hide(record_stop_button);
gtk_widget_show(record_start_button);
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "recording stopped");
}
break;

case 5:
/** photo being taken feedback */
/*
if (buffer_in[1] == 0x31)
{
printf("migawka wciśnięta\n");
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "shutter pressed");
}
if (buffer_in[1] == 0x30)
{
printf("migawka zwolniona\n");
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "shutter released");
}
*/
break;

case 6:
/*
	if (buffer_in[1] == 0x31)
	{
}
	else
	{
}
*/
break;

case 7:
	if (buffer_in[1] == 0x31)
	{
	//gtk_toggle_button_set_active(check_button_regulator, TRUE);
	gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "ESC enabled");
	printf("ESC załączony\n");
}
	else
	{
	//gtk_toggle_button_set_active(check_button_regulator, FALSE);
	gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "ESC disabled");
	printf("ESC wyłączony\n");
}
break;

case 8:
/** LED status feedback */
printf("LED event: %d, %d\n", recsize, buffer_in[1]);
if (buffer_in[1] == 0x31)
{
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "LED enabled");
printf("lampka załączona\n");
isShining = TRUE;
}
if (buffer_in[1] == 0x30)
{
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "LED disabled");
printf("lampka wyłączona\n");
isShining = FALSE;
}
break;

case 9:
{
/** arrestor hook / ground brake feedback */
if (buffer_in[1] == 0x31)
{
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "Arrestor hook retracted");
printf("hak schowany\n");
}
if (buffer_in[1] == 0x30)
{
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "Arrestor hook extended");
printf("hak wypuszczony\n");
}
}
break;

case 12:
{
/** servo controller feedback */
if (buffer_in[1] == 0x31)
{
gtk_widget_hide(servocontroller_enable_button);
gtk_widget_show(servocontroller_disable_button);
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "USC-16 enabled");
}
if (buffer_in[1] == 0x30)
{
gtk_widget_hide(servocontroller_disable_button);
gtk_widget_show(servocontroller_enable_button);
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "USC-16 disabled");
}
}
break;

case 13:
printf("Target pitch event\n");
/*
sprintf(target_pitch_string, "%.2f", bytes2float(buffer_in[1], buffer_in[2], buffer_in[3], buffer_in[4]));
push_item(statusbar, GINT_TO_POINTER(context_id), target_pitch_string);
*/
break;

case 16:
rudder_trim_value = (short) (buffer_in[1] << 8) + (int) buffer_in[2];
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "rudder trim feedback");
printf("Rudder trim: %d, (%d bytes)\n", rudder_trim_value, recsize);
break;

case 17:
elevator_trim_value = (short) (buffer_in[1] << 8) + (int) buffer_in[2];
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "elevator trim feedback");
printf("Elevator trim: %d, (%d bytes)\n", elevator_trim_value, recsize);
break;

default:
printf("received %d bytes:", recsize);
for (int position = 0; position < recsize; position++)
{
printf(" [%d]", buffer_in[position]);
}
printf("\n");
break;
}

/* end of feedback switch */

/** old, common feedback label */
/*
char* temp_string2;
temp_string2 = malloc(strlen(angle_roll_string) + strlen(angle_pitch_string) + strlen(alt_bar_string) + strlen(elevator_trim_string) + 16);
strcpy(temp_string2, angle_roll_string);
strcat(temp_string2, "\t");
strcat(temp_string2, angle_pitch_string);
strcat(temp_string2, "\t");
strcat(temp_string2, alt_bar_string);
strcat(temp_string2, "\t");
strcat(temp_string2, elevator_trim_string); 
gtk_label_set_text(label_feedback, temp_string2);
free(temp_string2);
*/
}
}

void open_UDP_socket() {
/** https://www.cs.rutgers.edu/~pxk/417/notes/sockets/udp.html */
memset(&sa, 0, sizeof(sa));
sa.sin_family = AF_INET;
//inet_aton("192.168.0.164", &sa.sin_addr.s_addr);
sa.sin_addr.s_addr = htonl(odbiornik_IP);
sa.sin_port = htons(odbiornik_port);
sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

if (-1 == sock) /* if socket failed to initialize, exit */
{
g_print("Error Creating Socket\n");
exit(EXIT_FAILURE);
}

struct sockaddr_in myaddr;
memset(&myaddr, 0, sizeof(myaddr));
myaddr.sin_family = AF_INET;
//inet_aton("192.168.1.100", &myaddr.sin_addr);
myaddr.sin_addr.s_addr = htonl(nadajnik_IP);
myaddr.sin_port = htons(6000);

if (-1 == bind(sock, (struct sockaddr *) &myaddr, sizeof(struct sockaddr)))
{
g_print("error bind failed %d\n", errno);
close(sock);
exit(EXIT_FAILURE);
}
iochan = g_io_channel_unix_new(sock);
int iowatch = -1;
iowatch = g_io_add_watch(iochan, G_IO_IN, feedback_event, NULL);
g_print("UDP socket open\n");
}

/* otwiera socket TCP */
void open_TCP_socket() {
SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
if (-1 == SocketFD) {
perror("cannot create socket");
//FIXME
//exit(EXIT_FAILURE);
}

memset(&stSockAddr, 0, sizeof(stSockAddr));
stSockAddr.sin_family = AF_INET;
stSockAddr.sin_addr.s_addr = htonl(odbiornik_IP);
stSockAddr.sin_port = htons(odbiornik_port);

//FIXME
//http://man7.org/linux/man-pages/man3/inet_pton.3.html
/*
Res = inet_pton(AF_INET, htonl(odbiornik_IP), &stSockAddr.sin_addr);

if (0 > Res) {
perror("error: first parameter is not a valid address family");
printf("error: first parameter is not a valid address family");
close(SocketFD);
} else if (0 == Res) {
perror("char string (second parameter does not contain valid ipaddress)");
printf("char string (second parameter does not contain valid ipaddress)");
close(SocketFD);
}
*/
if (connect(SocketFD, (struct sockaddr *) &stSockAddr, sizeof(stSockAddr)) == -1) {
perror("connect failed");
close(SocketFD);
} else {
printf("TCP socket created\n");
}

iochan = g_io_channel_unix_new(SocketFD);
int iowatch = -1;
iowatch = g_io_add_watch(iochan, G_IO_IN, feedback_event, NULL);
}

void errorTCP(const char *msg) {
perror(msg);
exit(0);
}

#if defined(__linux__) || defined(__FreeBSD__)
unsigned char packet_adres_nadajnika[5];
video_start() {
//TODO: number 27 is equivalent of "button 12 pressed", may cause conflict
packet_adres_nadajnika[0] = 27; /* "start streaming" code */
packet_adres_nadajnika[1] = nadajnik_IP >> 24; /* first byte of IP number (shifted by 24 bits) */
packet_adres_nadajnika[2] = nadajnik_IP >> 16;
packet_adres_nadajnika[3] = nadajnik_IP >> 8;
packet_adres_nadajnika[4] = nadajnik_IP; /* last byte of IP number*/
packet_adres_nadajnika[5] = v4l_device_number; /* camera device number */

bytes_sent = sendto(sock, packet_adres_nadajnika, 6, 0, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
if (bytes_sent < 0) {
printf("Error sending packet (video_start): %s\n", strerror(errno));
}
printf("%2hd△ kod przycisku: %2hd (sygnał rozpoczęcia przekazu)\n",	bytes_sent, packet_adres_nadajnika[0]);
/* push_item(statusbar, GINT_TO_POINTER (context_id), "nadawanie obrazu i dźwięku włączone"); */
}
#else
/* Windows */
#endif

/* https://rosettacode.org/wiki/Array_concatenation#C */
void *array_concat(const void *a, size_t an, const void *b, size_t bn, size_t s)
{
  char *p = malloc(s * (an + bn));
  memcpy(p, a, an*s);
  memcpy(p + an*s, b, bn*s);
  return p;
}

send_point(double lat, double lon, double ele)
{
char header[4] = {52, 0, 0, 0};
char data1[8];
char data2[8];
char data3[8];

memcpy(&data1, &lat, sizeof(double));
memcpy(&data2, &lon, sizeof(double));
memcpy(&data3, &ele, sizeof(double));

char *latlon = array_concat(data1, 8, data2, 8, sizeof(char));
char *latlonele = array_concat(latlon, 16, data3, 8, sizeof(char));
char *message = array_concat(header, 4, latlonele, 24, sizeof(char));

bytes_sent = sendto(sock, message, 28, 0, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
if (bytes_sent < 0) {
printf("Error sending packet: %s\n", strerror(errno));
}
else
{
printf("sendto OK %d\n", sizeof(double));
}
/*
for (int i = 0; i < 28; i++)
{
	if(i == 3 || i == 11 || i == 19 || i == 27)
	{printf("%d\n", message[i]);}
	else
	{printf("%d, ", message[i]);}
}
*/
}

void skip_point()
{
	char message[4] = {53, 0, 0, 0};
	bytes_sent = sendto(sock, message, sizeof(message), 0, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
		if (bytes_sent < 0) {
		printf("Error sending packet: %s\n", strerror(errno));
		}
		else
		{
		printf("sendto OK %d\n", sizeof(double));
		}
}

introduce_yourself_TCP(){
packet_adres_nadajnika[0] = 51;
packet_adres_nadajnika[1] = nadajnik_IP >> 24;
packet_adres_nadajnika[2] = nadajnik_IP >> 16;
packet_adres_nadajnika[3] = nadajnik_IP >> 8;
packet_adres_nadajnika[4] = nadajnik_IP;

if (write(SocketFD, packet_adres_nadajnika, 5) < 0) {
printf("adres nie wysłany przes TCP\n");
} else {
printf("wysłany adres: %2hd.%2hd.%2hd.%2hd\n", packet_adres_nadajnika[1], packet_adres_nadajnika[2], packet_adres_nadajnika[3], packet_adres_nadajnika[4]);
}
}

video_stop() {
packet_adres_nadajnika[0] = 26; /* "button 12 released" code */
bytes_sent = sendto(sock, packet_adres_nadajnika, 1, 0,	(struct sockaddr*) &sa, sizeof(struct sockaddr_in));
if (bytes_sent < 0) {
printf("Error sending packet (video_stop): %s\n", strerror(errno));
}
printf("%2hd△ kod przycisku: %2hd (sygnał zatrzymania przekazu)\n",
bytes_sent, packet_adres_nadajnika[0]);
/* push_item(statusbar, GINT_TO_POINTER (context_id), "nadawanie obrazu i dźwięku wyłączone"); */

makeWindowBlack(drawing_area);
/* zaczernia obszar wyświetlania */
}

void toggle_video_button_callback(GtkWidget *check_button_video,
gpointer data) {
if (tryb_wysylania == 0) {
if (video_running == FALSE) {
video_start();
/* wywołuje funkcję audio_receive z opóźnieniem w milisekundach */
//g_timeout_add(100, audio_receive, NULL);
video_running = TRUE;
} else {
video_stop();
video_running = FALSE;
}
}
if (tryb_wysylania == 1) {
if (video_running == FALSE) {
// PhoneUAV - enable camera
//wyslij_dwustan_TCP(12, 1);
video_running = TRUE;
//FIXME
//push_item(statusbar, GINT_TO_POINTER(context_id), "podgląd włączony");
} else {
// PhoneUAV - disable camera
//wyslij_dwustan_TCP(12, 0);
video_running = FALSE;
//FIXME
//push_item(statusbar, GINT_TO_POINTER(context_id), "podgląd wyłączony");
}
}
}

void switch_Recording()
{
if (isRecording)
{
wyslij_dwustan(4, 0);
}
else
{
wyslij_dwustan(4, 1);
}
}

void switch_Shining()
{
if (isShining)
{
wyslij_dwustan(8, 0);
}
else
{
wyslij_dwustan(8, 1);
}
}

void switch_Expo()
{
if (isExponential)
{
//push_item(statusbar, GINT_TO_POINTER(context_id), "exponential mode disabled");
isExponential = FALSE;
}
else
{
//push_item(statusbar, GINT_TO_POINTER(context_id), "exponential mode enabled");
isExponential = TRUE;
}
}

void toggle_servocontroller (GtkWidget *widget, gpointer data)
{
	if (data == TRUE)
	{
		wyslij_dwustan(12 , 1);
	}
	else
	{
		wyslij_dwustan(12 , 0);
	}
}

/* przełącza stan checkboxa */
void toggle_ESC_callback(GtkWidget *check_button_regulator, gpointer data) {
// enables or disables PWM thread
if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_button_regulator))) {
wyslij_dwustan(7, 1);
} else {
wyslij_dwustan(7, 0);
}
}

/* enables ESC when analog trigger is pulled */
/*
check_ESC_once() {
if (axis_r2_ever_moved == FALSE) {

wyslij_dwustan(7, 1);
axis_r2_ever_moved = TRUE;
gtk_toggle_button_set_active(check_button_regulator, TRUE);
}
}
*/

// enables ESC whenever analog trigger is pulled
check_ESC(gboolean value) {
if (value)
{wyslij_dwustan(7, 1);
gtk_toggle_button_set_active(check_button_regulator, TRUE);}
else
{wyslij_dwustan(7, 0);
gtk_toggle_button_set_active(check_button_regulator, FALSE);}
}

/* nie jestem pewien czy ta funkcja powinna być w tym miejscu */
void zamknij_joystick() {
/* conflicting types */
g_io_channel_flush(ioch, &jserror);
g_io_channel_shutdown(ioch, TRUE, &jserror);
g_io_channel_unref(ioch);
/* FIXME: dodać zerowanie osi i wyłączanie przycisków */
close_rumble_fd();
}

/* przycisk wykrywający i wyłączający joystick, czasami sprawia problemy */
void toggle_joystick_button_callback(GtkWidget *check_button_joystick,
gpointer data) {
if (gtk_toggle_button_get_active(
GTK_TOGGLE_BUTTON(check_button_joystick))) {
wykryj_joystick();
} else {
zamknij_joystick();
}
}

void klawisz(GtkWidget *widget, GdkEventKey *event, int stan) { /* parametr stan jest przekazywany do funkcji i przyjmuje wartości 1 lub 0 */

uint klawisz;
klawisz = ((GdkEventKey*) event)->keyval;
uint klawisz_hardware;
klawisz_hardware = ((GdkEventKey*) event)->hardware_keycode;
uint czas;
czas = ((GdkEventKey*) event)->time;

/*pressing keys*/
if (stan == 1) {
printf("klawisz wcisnięty: %6X,%6X,%8hd\n", klawisz, klawisz_hardware,
czas);
if (klawisz == 0xff51) /* strzalka w lewo */
{
stick_x = -32768;
sprintf(wychylenie_x, "%d", stick_x);
gtk_label_set_text(label_x1, wychylenie_x);
wyslij_stick(0, stick_x);
}
if (klawisz == 0xff53) /* strzalka w prawo */
{
stick_x = 32767;
sprintf(wychylenie_x, "%d", stick_x);
gtk_label_set_text(label_x1, wychylenie_x);
wyslij_stick(0, stick_x);
}
if (klawisz == 0xff52) /* strzalka w górę */
{
/*
stick_y = -32768;
sprintf(wychylenie_y, "%d", stick_y);
gtk_label_set_text(label_y1, wychylenie_y);
wyslij_stick(1, stick_y);
*/

// half speed
throttle = 16384;
sprintf(wychylenie_y, "%d", throttle);
gtk_label_set_text(label_y1, wychylenie_y);
wyslij_stick(4, throttle);
}
if (klawisz == 0xff54) /* strzalka w dół */
{
/*
stick_y = 32767;
sprintf(wychylenie_y, "%d", stick_y);
gtk_label_set_text(label_y1, wychylenie_y);
wyslij_stick(1, stick_y);
*/

// quarter speed
throttle = 8192;
sprintf(wychylenie_y, "%d", throttle);
gtk_label_set_text(label_y1, wychylenie_y);
wyslij_stick(4, throttle);
}

/* numpad */
if (klawisz == 0xff96 || klawisz == 0xffb4) /* numpad 4 */
{
//push_item(statusbar, GINT_TO_POINTER(context_id), "numpad 4 pressed");
wyslij_stick(8, -32768);
}

if (klawisz == 0xff98 || klawisz == 0xffb6) /* numpad 6 */
{
//push_item(statusbar, GINT_TO_POINTER(context_id), "numpad 6 pressed");
wyslij_stick(8, 32767);
}

if (klawisz == 0xff97 || klawisz == 0xffb8) /* numpad 8 */
{
//push_item(statusbar, GINT_TO_POINTER(context_id), "numpad 8 pressed");
wyslij_stick(9, -32768);
}

if (klawisz == 0xff99 || klawisz == 0xffb2) /* numpad 2 */
{
//push_item(statusbar, GINT_TO_POINTER(context_id), "numpad 2 pressed");
wyslij_stick(9, 32767);
}
/* end of numpad */

if (klawisz == 'e')
{
switch_Expo();
}

if (klawisz == 'f')
{
toggle_Fullscreen();
}

/** PID */
if (klawisz == 'q')
/* increase proportional */
{wyslij_dwustan(13, 1);}
if (klawisz == 'a')
/* decrease proportional */
{wyslij_dwustan(13, 0);}
if (klawisz == 'w')
/* increase integral */
{wyslij_dwustan(14, 1);}
if (klawisz == 's')
/* decrease integral */
{wyslij_dwustan(14, 0);}
if (klawisz == 'e')
/* increase derivative */
{wyslij_dwustan(15, 1);}
if (klawisz == 'd')
/* decrease derivative */
{wyslij_dwustan(15, 0);}
if (klawisz == 0x002C)
{/* ',' */
wyslij_stick(6, 32767);
}

if (klawisz == 0x002E)
{/* '.' */
wyslij_stick(6, -32768);
}

if (klawisz == '=')
{
wyslij_stick(7, -32768);
}

if (klawisz == '-')
{
wyslij_stick(7, 32767);
}

if (klawisz == '[')
{
printf("\'[\' - waypoint action No 1\n");
push_item(statusbar, GINT_TO_POINTER(context_id), "\'[\' - waypoint action No 1");
//TODO: include sending waypoints
}

if (klawisz == ']')
{
printf("\']\' - waypoint action No 2\n");
push_item(statusbar, GINT_TO_POINTER(context_id), "\']\' - waypoint action No 2 (skip waypoint)");
skip_point();
}
if (klawisz == 'i')
{
printf("sending introduction\n");
//push_item(statusbar, GINT_TO_POINTER(context_id), "sending introduction");
}

if (klawisz == 'h')
{
wyslij_dwustan(9, 1);
}

if (klawisz == 'k')
{
if (synchronization)
	{
synchronization = FALSE;
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "synchro OFF");
	}
else
	{
synchronization = TRUE;
gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "synchro ON");
	}
}

if (klawisz == 'l')
{
switch_Shining();
}
}

/*releasing keys*/
if (stan == 0) {
printf("klawisz zwolniony: %6X,%6X,%8hd\n", klawisz, klawisz_hardware,
czas);
if (klawisz == 0xff51) /* strzalka w lewo zwolniona */
{
stick_x = 0;
sprintf(wychylenie_x, "%d", stick_x);
gtk_label_set_text(label_x1, wychylenie_x);
wyslij_stick(0, stick_x);
}
if (klawisz == 0xff53) /* strzalka w prawo zwolniona */
{
stick_x = 0;
sprintf(wychylenie_x, "%d", stick_x);
gtk_label_set_text(label_x1, wychylenie_x);
wyslij_stick(0, stick_x);
}
if (klawisz == 0xff52) /* strzalka w górę zwolniona */
{
/*
stick_y = 0;
sprintf(wychylenie_y, "%d", stick_y);
gtk_label_set_text(label_y1, wychylenie_y);
wyslij_stick(1, stick_y);
*/

throttle = 0;
sprintf(wychylenie_y, "%d", throttle);
gtk_label_set_text(label_y1, wychylenie_y);
wyslij_stick(4, throttle);
}
if (klawisz == 0xff54) /* strzalka w dół zwolniona */
{
/*
stick_y = 0;
sprintf(wychylenie_y, "%d", stick_y);
gtk_label_set_text(label_y1, wychylenie_y);
wyslij_stick(1, stick_y);
*/

throttle = 0;
sprintf(wychylenie_y, "%d", throttle);
gtk_label_set_text(label_y1, wychylenie_y);
wyslij_stick(4, throttle);
}

if (klawisz == 0x002C)
{/* ',' */
wyslij_stick(6, 0);
}

if (klawisz == 0x002E)
{/* '.' */
wyslij_stick(6, 0);
}

if (klawisz == '=')
{
wyslij_stick(7, 0);
}

if (klawisz == '-')
{
wyslij_stick(7, 0);
}

if (klawisz == 'h')
{
wyslij_dwustan(9, 0);
}

/* numpad */
if (klawisz == 0xff96 || klawisz == 0xffb4) /* numpad 4 */
{
wyslij_stick(8, 0);
}

if (klawisz == 0xff98 || klawisz == 0xffb6) /* numpad 6 */
{
wyslij_stick(8, 0);
}

if (klawisz == 0xff97 || klawisz == 0xffb8) /* numpad 8 */
{
wyslij_stick(9, 0);
}

if (klawisz == 0xff99 || klawisz == 0xffb2) /* numpad 2 */
{
wyslij_stick(9, 0);
}
/* end of numpad */
}
}

int open_joystick(char *joystick_device) {
/* parameter added */
joystick_fd = open(joystick_descriptor, O_RDWR | O_NONBLOCK | O_NOCTTY);
//joystick_fd = open(joystick_descriptor, O_RDONLY | O_NONBLOCK | O_NOCTTY);
/* joystick_fd to port joysticka */
if (joystick_fd < 0)
return joystick_fd;
/* TODO: maybe ioctls to interrogate features here? */
return joystick_fd;
}

int read_joystick_event(struct js_event *jse) {
int bytes;
bytes = read(joystick_fd, jse, sizeof(*jse));
if (bytes == -1)
return 0;
if (bytes == sizeof(*jse))
return 1;
printf("Unexpected bytes from joystick:%d\n", bytes);
return -1;
}

void gtk_markup(int number, char *string) {
char *markup;
if (number == 0 || number == 3) {
markup = g_markup_printf_escaped("X: <span color=\"red\">%s</span>", string);
gtk_label_set_markup(label_x1, markup);
} else if (number == 1 || number == 4) {
markup = g_markup_printf_escaped("Y: <span color=\"red\">%s</span>", string);
gtk_label_set_markup(label_y1, markup);
}
else {
markup = g_markup_printf_escaped(number +": <span color=\"red\">%s</span>", string);
gtk_label_set_markup(label_y1, markup);
}
g_free(markup);
}

/*
double pow(double input, double power)
{
double output = input;
for (int i = 1; i < power; i++)
{output *= input;}
return output;
}
*/

int expo(int input)
{
double output = 0;
output = (pow(input + 0.5, 3)/1073741824) - 0.5;

/* show values on progressbars */
//gtk_progress_bar_set_fraction(progress3, input * 0.000015259 + 0.5);
//gtk_progress_bar_set_fraction(progress4, output * 0.000015259 + 0.5);
return (int) output;
}

void zczytuj_joystick() {
rc = read_joystick_event(&jse);
usleep(100);
if (rc == 1) {
/*
Mode 1 – Kierunek / Wysokość	Lotki / Gaz
Mode 2 – Kierunek / Gaz		Lotki / Wysokość
Mode 3 – Lotki / Wysokość		Kierunek / Gaz
Mode 4 – Lotki / Gaz			Kierunek / Wysokość
*/
if (jse.type == JS_EVENT_AXIS) {
if (jse.number == axis[0]) {
/* rudder */
if (abs(jse.value) > deadzone_x1) {
stick_x = jse.value;
wyslij_stick(0, stick_x);
printf("oś: %d wychylenie %d\n", jse.number, stick_x);
}

else {
if (abs(stick_x) < deadzone_x1) {
stick_x = 0;
wyslij_stick(0, stick_x);
printf("oś: %d wychylenie %d (dead zone)\n", jse.number, stick_x);
}
}
sprintf(wychylenie_x, "%+06d", stick_x);
gtk_markup(jse.number, wychylenie_x);
}

else if (jse.number == axis[1]) {
if (r2_throttle == TRUE) {
	/* right analog button works as throttle */
	if (abs(jse.value) > deadzone_y1) {
	stick_y = jse.value;
	//wyslij_stick(5, stick_y);
	//printf("oś: %d wychylenie (does nothing)%d\n", jse.number, jse.value);
	}
	else {
	if (abs(stick_y) < deadzone_y1) {
	stick_y = 0;
	//wyslij_stick(5, stick_y);
	//printf("oś: %d wychylenie %d (does nothing, dead zone)\n", jse.number, jse.value);
	}
	}
	sprintf(wychylenie_y, "%+06d", stick_y);
	gtk_markup(jse.number, wychylenie_y);
}
	else
	{
	/* right analog button doesn't work as throttle */
	stick_y = jse.value;
	if (stick_y > 0)
	{
	/* pull stick closer: half range */
	throttle = stick_y - 32768;
	}
	else
	{
	/* push stick away: full range */
	throttle = -stick_y * 2 - 32768;
	}

	sprintf(wychylenie_y, "%+06d", throttle);
	gtk_markup(jse.number, wychylenie_y);

	wyslij_stick(4, throttle);
	gtk_progress_bar_set_fraction(progress2, throttle * 0.000015259 + 0.5);
	}
}

else if (jse.number == axis[3]) {
/* aileron */
if (isExponential)
{
stick_x2 = expo((double) jse.value);
wyslij_stick(2, stick_x2);
sprintf(wychylenie_x2, "X: %+06d", stick_x2);
printf("oś: %d wychylenie %d (expo: %d)\n", jse.number, jse.value, stick_x2);
}
else
{
/* linear mode */
stick_x2 = jse.value;
wyslij_stick(2, stick_x2);
sprintf(wychylenie_x2, "X: %+06d", stick_x2);
printf("oś: %d wychylenie %d\n", jse.number, jse.value);
}
gtk_label_set_text(label_x2, wychylenie_x2);
}

else if (jse.number == 2) {
	/* left analog trigger (flaps) */
	stick_l2 = jse.value;
	printf("oś: %d wychylenie %d\n", jse.number, jse.value);
	wyslij_stick(3, jse.value);
	gtk_progress_bar_set_fraction(progress1, jse.value * 0.000015259 + 0.5);
}

else if (jse.number == axis[4]) {
/* elevator */
if (isExponential)
{
/* exponential mode */
stick_y2 = expo((double) jse.value);
wyslij_stick(1, stick_y2);
sprintf(wychylenie_y2, "Y: %+06d", stick_y2);
printf("oś: %d wychylenie %d (expo: %d)\n", jse.number, jse.value, stick_y2);
}
else
{
/* linear mode */
stick_y2 = jse.value;
wyslij_stick(1, stick_y2);
sprintf(wychylenie_y2, "Y: %+06d", stick_y2);
printf("oś: %d wychylenie %d\n", jse.number, jse.value);
}
gtk_label_set_text(label_y2, wychylenie_y2);
}

else if (jse.number == 5) {
/* right analog trigger */
if (r2_throttle && r2_pressed == TRUE) {
	stick_r2 = jse.value;
	throttle = jse.value;
	printf("oś: %d wychylenie %d\n", jse.number, throttle);
	wyslij_stick(4, throttle);
	gtk_progress_bar_set_fraction(progress2, jse.value * 0.000015259 + 0.5);
}
}

else if (jse.number == axis[6]) {
/* mapping depends on connection type (USB/Bluetooth) */
stick_6 = jse.value;
if(stick_6 == 0)
{
//push_item(statusbar, GINT_TO_POINTER(context_id), "");
}
else if(stick_6 > 0)
{
//printf("rudder trim increased\n");
wyslij_stick(6, -stick_6);
}
else if(stick_6 < 0)
{
//printf("rudder trim decreased\n");
wyslij_stick(6, -stick_6);
}
}

else if (jse.number == axis[7]) {
/* mapping depends on connection type (USB/Bluetooth) */
stick_7 = jse.value;
if(stick_7 == 0)
{
//push_item(statusbar, GINT_TO_POINTER(context_id), "");
}
else if(stick_7 > 0)
{
//printf("elevator trim +10\n");
wyslij_stick(7, stick_7);
}
else if(stick_7 < 0)
{
//printf("elevator trim -10\n");
wyslij_stick(7, stick_7);
}
}
}

if (jse.type == JS_EVENT_BUTTON) {
/* button pressed */
if (jse.value == 1)
{
if (jse.number == button[0])
{/* DS4 "X" */
/* pitch stabilization disabled */
wyslij_dwustan(0, 1);
//play_rumble_effect(RUMBLE_STRONG_RUMBLE_EFFECT);
}
else if (jse.number == button[1])
{/* DS4 "circle" */
/* roll stabilization disabled */
wyslij_dwustan(1, 1);
//play_rumble_effect(RUMBLE_STRONG_RUMBLE_EFFECT);
}
else if (jse.number == button[2])
{/* DS4 "triangle" */
/* roll stabilization enabled */
wyslij_dwustan(2, 1);
//play_rumble_effect(RUMBLE_STRONG_RUMBLE_EFFECT);	
}
else if (jse.number == button[3])
{/* DS4 "square" */
/* pitch stabilization enabled */
wyslij_dwustan(3, 1);
//play_rumble_effect(RUMBLE_STRONG_RUMBLE_EFFECT);
}
else if (jse.number == button[4])
{/* DS4 L1 */
switch_Recording();
//play_rumble_effect(RUMBLE_STRONG_RUMBLE_EFFECT);
}
else if (jse.number == button[5])
{/* DS4 R1 */
wyslij_dwustan(5, 1);
//play_rumble_effect(RUMBLE_STRONG_RUMBLE_EFFECT);
}
else if (jse.number == button[6])
{/* DS4 L2 "button" */}
else if (jse.number == button[7])
{/* DS4 R2 "button" */}
else if (jse.number == button[8])
{/* DualShock 3: Select button, DualShock 4: Share button */
switch_Shining();
//wyslij_dwustan(8, 1);
}
else if (jse.number == button[9])
{/* DualShock 3: Start button, DualShock 4: Options button */
wyslij_dwustan(9, 1);
}
else if (jse.number == button[10])
{printf("PlayStation button pressed\n");}
else if (jse.number == button[11])
{/* left knob press */
printf("left knob pressed\n");
/*
if (r2_throttle == TRUE)
{
	r2_throttle = FALSE;
	gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "throttle controlled by stick's vertical axis");
}
*/
}
else if (jse.number == button[12])
{/* right knob press */
printf("right knob pressed\n");
/*
if (r2_throttle == FALSE)
{
	r2_throttle = TRUE;
	gtk_statusbar_push(statusbar, GINT_TO_POINTER(context_id), "throttle controlled by R2 analog trigger");
}
*/
}
else
{printf("button %u pressed\n", jse.number);}

if (buttons_ever_pressed == FALSE)
{buttons_ever_pressed = TRUE;}
}

/* button released */
if (jse.value == 0 && buttons_ever_pressed) {
if (jse.number == button[0])
{
wyslij_dwustan(0, 0);
//stop_all_rumble_effects();
}
else if (jse.number == button[1])
{
wyslij_dwustan(1, 0);
//stop_all_rumble_effects();
}
else if (jse.number == button[2])
{
wyslij_dwustan(2, 0);
//stop_all_rumble_effects();
}
else if (jse.number == button[3])
{
wyslij_dwustan(3, 0);
//stop_all_rumble_effects();
}
else if (jse.number == button[4])
{
//stop_all_rumble_effects();
}
else if (jse.number == button[5])
{
wyslij_dwustan(5, 0);
//stop_all_rumble_effects();
}
else if (jse.number == button[6])
{}
else if (jse.number == button[7])
{
//stop_all_rumble_effects();
}
else if (jse.number == button[8])
{
/* DualShock 3: Select button, DualShock 4: Share button */
//wyslij_dwustan(8, 0);
}
else if (jse.number == button[9])
{/* DualShock 3: Start button, DualShock 4: Options button */
wyslij_dwustan(9, 0);
}
else if (jse.number == button[10])
{printf("PlayStation button released\n");}
else if (jse.number == button[11])
{printf("left knob released\n");}
else if (jse.number == button[12])
{printf("right knob released\n");}
else
{printf("button %u released\n", jse.number);}
}
}

if (jse.type == JS_EVENT_INIT) {
printf("Stan początkowy joysticka.\n");
}
}
}

int wykryj_joystick() {
joystick_fd = open_joystick(joystick_descriptor);
if (joystick_fd < 0) {
printf("Nie wykryto joysticka %s\n", joystick_descriptor);
if (pierwsze_wykrywanie_joysticka == TRUE) {
gtk_toggle_button_set_active(check_button_joystick, FALSE);
g_signal_connect(check_button_joystick, "clicked",
G_CALLBACK(toggle_joystick_button_callback), NULL);
/* FIXME */
}
} else {
printf("Wykryto joystick %s\n", joystick_descriptor);
if (pierwsze_wykrywanie_joysticka == TRUE) {
gtk_toggle_button_set_active(check_button_joystick, TRUE);
g_signal_connect(check_button_joystick, "clicked",
G_CALLBACK(toggle_joystick_button_callback), NULL);
}
ioch = g_io_channel_unix_new(joystick_fd);
int iowatch = -1;
iowatch = g_io_add_watch(ioch, G_IO_IN, zczytuj_joystick, NULL);

if (get_ready_to_rumble(rumbledevice) != 0) {
printf("No rumble...\n");
}
}
pierwsze_wykrywanie_joysticka = FALSE;
}

int main(int argc, char *argv[]) {
/*
int argi;
for (argi=1; argi < argc; argi++)
{printf("parameter: %s\n", argv[argi]);}
*/
gst_init(&argc, &argv);
gtk_init(&argc, &argv);

start_GUI();

g_signal_connect(G_OBJECT(drawing_area), "draw", G_CALLBACK(on_draw_event), NULL);
g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
g_signal_connect(main_window, "key_press_event", G_CALLBACK(klawisz), 1);
g_signal_connect(main_window, "key_release_event", G_CALLBACK(klawisz), 0);

g_signal_connect(servocontroller_enable_button, "clicked", G_CALLBACK(toggle_servocontroller), TRUE);
g_signal_connect(servocontroller_disable_button, "clicked", G_CALLBACK(toggle_servocontroller), FALSE);
g_signal_connect(check_button_regulator, "clicked", G_CALLBACK(toggle_ESC_callback), NULL);
g_signal_connect(record_start_button, "clicked", G_CALLBACK(switch_Recording), NULL);
g_signal_connect(record_stop_button, "clicked", G_CALLBACK(switch_Recording), NULL);
g_signal_connect(check_button_video, "clicked", G_CALLBACK(toggle_video_button_callback), NULL);
g_signal_connect_swapped(G_OBJECT(main_window), "destroy", G_CALLBACK(zamknij), NULL);

switch (tryb_wysylania) {
case 1:
g_signal_connect(reconnect_button, "clicked", G_CALLBACK(open_TCP_socket), NULL);
break;
default:
//TODO: close socket, reopen socket
break;
}

/* wczytuje wartości zmiennych z pliku config.cfg */
open_config(argv[1]);

/* distinguishes between USB and Bluetooth connection */
//TODO: https://linux.die.net/man/8/udevadm
if (0 == access("/dev/input/by-id/usb-Sony_Computer_Entertainment_Wireless_Controller-joystick", 0)) { 
g_print("Joystick file exists - USB mapping\n");

gtk_progress_bar_set_text(progress3, "touchpad X:");
gtk_progress_bar_set_text(progress4, "touchpad Y:");
gtk_progress_bar_set_text(progress5, "axis 8:");
}
else if (access("/dev/input/by-id/usb-Microsoft_Controller_0000000000000000000000000000-joystick", 0) == 0) {
g_print("Xbox One controller - USB mapping\n");

button[2] = 3; // "triangle" - "Y"
button[3] = 2; // "square" - "X"
button[8] = 6; // share - view
button[9] = 7; // options - menu
button[10] = 8; // PlayStation - Xbox
button[11] = 9; // left stick press
button[12] = 10; // right stick press
}
else {
g_print("Joystick file doesn't exist - Bluetooth mapping\n");

axis[6] = 6;
axis[7] = 7;

axis[9] = 9;
axis[10] = 10;
axis[11] = 11;

gtk_progress_bar_set_text(progress3, "gyro X?");
gtk_progress_bar_set_text(progress4, "gyro Y?");
gtk_progress_bar_set_text(progress5, "gyro Z?");
}

/* wyświetla wszystkie elementy okna */
gtk_widget_show_all(main_window);

// UDP
if (tryb_wysylania == 0) {
open_UDP_socket();
video_start();
video_receive();
}

// TCP
if (tryb_wysylania == 1) {
// connect on startup
open_TCP_socket();
/* wysyła pakiet z numerem kamery i adresem IP na jaki ma być streamingowany obraz z kamery */
introduce_yourself_TCP();
}

// UDP, old
if (tryb_wysylania == 2) {
open_UDP_socket();
/* wysyła pakiet z numerem kamery i adresem IP na jaki ma być streamowany obraz z kamery */
video_start();
/* rozpoczyna wyświetlanie obrazu */
video_receive();
}

/* uruchamia "audio_receive" z opóźnieniem 100 milisekund (właściwie co 100 milisekund) wymaga tego specyfika protokołu TCP */
g_timeout_add(100, audio_receive, NULL);
/* FIXME: są problemy z dźwiękiem, chyba chodzi o synchronizację dźwięku z obrazem */

/* wykrywanie joysticka przy starcie programu */
wykryj_joystick();

/* hides one of fullscreen buttons and recording stop button */
if (isFullscreen)
{
gtk_widget_hide(fullscreen_enable_button);
gtk_window_fullscreen(GTK_WINDOW(main_window));
}
else
{
gtk_widget_hide(fullscreen_disable_button);
/* gtk_window_unfullscreen(GTK_WINDOW(main_window)); */
}

gtk_widget_hide(servocontroller_enable_button);
gtk_widget_hide(record_stop_button);

gtk_main();
/* Out of the main loop, clean up nicely */
g_print("Stopping playback\n");
gst_element_set_state(pipeline, GST_STATE_NULL);

g_print("Deleting pipeline\n");
gst_object_unref(GST_OBJECT(pipeline));

return 0;
}
