/*
requires:
libgstreamer-plugins-base0.10-dev libgtk2.0-dev

installation:
sudo apt-get install libgstreamer-plugins-base0.10-dev
sudo apt-get install libgtk2.0-dev
*/

#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>  /* "There is some amount of overlap with sys/types.h as known by inet code" */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "config.c"
#include "rs232.h"
#include "rs232.c"
#include "video_receive.c"
#include "rumble.h"
#include "rumble.c"
//#include "drawing.c"

#define JS_EVENT_BUTTON         0x01    /* button pressed/released	*/
#define JS_EVENT_AXIS           0x02    /* joystick moved			*/
#define JS_EVENT_INIT           0x80    /* initial state of device	*/

struct js_event {
	unsigned int time; /* event timestamp in milliseconds */
	short value; /* value */
	unsigned char type; /* event type */
	unsigned char number; /* axis/button number */
};

gboolean virgin_buttons;
gboolean pierwsze_wykrywanie_joysticka = TRUE;
struct js_event jse;

struct wwvi_js_event {
	int button[11];
	int stick_x;
	int stick_y;
};

/* wychylenia osi jako liczby */
int stick_x = 0;
int stick_y = 0;
int stick_3 = 0;
int stick_4 = 0;

/* wychylenia osi jako ciągi znaków */
char wychylenie_x[6];
char wychylenie_y[6];
char wychylenie_3[6];
char wychylenie_4[6];

GtkLabel *label_x;
GtkLabel *label_y;

//UDP
int sock;
struct sockaddr_in sa;
int bytes_sent, buffer_length;
char buffer[200];

//TCP
struct sockaddr_in stSockAddr;
int Res;
int SocketFD;

/* kanał wejścia joysticka */
GIOChannel *ioch;
GError *jserror;

char rumbledevicestring[PATH_MAX];
char *rumbledevice = NULL;

int poz_serwo_01;
int poz_serwo_02;

/* tworzy elementy okna programu */
GtkWidget *main_window;
GtkWidget *vbox; /* podział w pionie na główną część i statusbar		*/
GtkWidget *main_table; /* tabela z trzema kolumnami						*/
GtkWidget *statusbar; /* pasek stanu u dołu okna							*/
GtkWidget *left_table; /* tabela 3 na 3 w lewej komórce głównej tabeli		*/
GtkWidget *tank_controlls;
GtkWidget *right_table; /* tabela 3 na 3 w prawej komórce głównej tabeli	*/
GtkWidget *wskazniki;

/* tworzy ikonki */
GtkWidget *quit_icon, *fullscreen_enable_icon, *fullscreen_disable_icon,
		*video_icon, *joystick_icon;
/* tworzy przyciski */
GtkWidget *quit_button, *fullscreen_enable_button, *fullscreen_disable_button;
/* tworzy checkboxy */
GtkWidget *check_button_video, *check_button_joystick,
		*check_button_serwokontroler, *check_button_regulator;
/* FIXME: ostrzeżenie pojawia się obojętnie czy są to GtkToggleButton czy GtkWidget */

/* potrzebne do statusbar */
gint context_id;
/* treść komunikatu wyświetlanego na pasku stanu */
char* message;

int zamknij() {

	// FIXME zdublowane
	if (tryb_wysylania == 1) {
		close(sock);
		g_print("Closing UDP socket\n");
	}

	if (tryb_wysylania == 2) {
		(void) shutdown(SocketFD, SHUT_RDWR);
		close(SocketFD);
		g_print("Closing TCP socket\n");
	}

	gtk_main_quit();
	return 0;
}

/* włącza i wyłącza tryb pełnoekranowy */
void switch_Fullscreen() {
	printf("zmiana trybu fullscreen\n");
	if (isFullscreen == 1) {
		isFullscreen = 0;
		gtk_widget_hide(fullscreen_disable_button);
		gtk_widget_show(fullscreen_enable_button);
		g_print("rysuje przycisk \"fullscreen_enable_button\"\n");
		gtk_window_unfullscreen(GTK_WINDOW(main_window));
	} else {
		isFullscreen = 1;
		gtk_widget_hide(fullscreen_enable_button);
		gtk_widget_show(fullscreen_disable_button);
		g_print("rysuje przycisk \"fullscreen_disable_button\"\n");
		gtk_window_fullscreen(GTK_WINDOW(main_window));
	}
}
/* otwiera socket UDP */
void otworz_UDP() {
	/* tylko jeśli program był uruchomiony w trybie wysyłania 1 */
	if (tryb_wysylania == 1) {
		sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (-1 == sock) /* if socket failed to initialize, exit */
		{
			printf("Error Creating Socket");
			exit(EXIT_FAILURE);
		}

		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		sa.sin_addr.s_addr = htonl(odbiornik_IP);
		/* 0x7F000001 to adres 127.0.0.1 czyli loopback */
		sa.sin_port = htons(7654);
	}
}

void errorTCP(const char *msg) {
	perror(msg);
	exit(0);
}

/* otwiera socket TCP */
/*void otworz_TCP()
 {
 if (tryb_wysylania == 2)
 {
 }
 }
 */
unsigned char packet_adres_nadajnika[5];
video_start() {
	packet_adres_nadajnika[0] = 27; /* rozpoznawczy bajt pakietu włączającego przesyłanie	*/
	packet_adres_nadajnika[1] = v4l_device_number; /* numer kamery											*/
	packet_adres_nadajnika[2] = nadajnik_IP >> 24; /* pierwszy bajt numeru IP (przesunięcie o 24 bity)		*/
	packet_adres_nadajnika[3] = nadajnik_IP >> 16;
	packet_adres_nadajnika[4] = nadajnik_IP >> 8;
	packet_adres_nadajnika[5] = nadajnik_IP; /* ostatni bajt numeru IP								*/

	if (tryb_wysylania == 1) {
		bytes_sent = sendto(sock, packet_adres_nadajnika, 6, 0,
				(struct sockaddr*) &sa, sizeof(struct sockaddr_in));
		if (bytes_sent < 0) {
			printf("Error sending packet (video_start): %s\n", strerror(errno));
		}
		printf("%2hd△ kod przycisku: %2hd (sygnał rozpoczęcia przekazu)\n",
				bytes_sent, packet_adres_nadajnika[0]);
		/* push_item(statusbar, GINT_TO_POINTER (context_id), "nadawanie obrazu i dźwięku włączone"); */
	}
	// TODO TCP
}

video_stop() {
	packet_adres_nadajnika[0] = 26; /* przycisk 12 zwolniony */

	if (tryb_wysylania == 1) {
		bytes_sent = sendto(sock, packet_adres_nadajnika, 1, 0,
				(struct sockaddr*) &sa, sizeof(struct sockaddr_in));
		if (bytes_sent < 0) {
			printf("Error sending packet (video_stop): %s\n", strerror(errno));
		}
		printf("%2hd△ kod przycisku: %2hd (sygnał zatrzymania przekazu)\n",
				bytes_sent, packet_adres_nadajnika[0]);
		/* push_item(statusbar, GINT_TO_POINTER (context_id), "nadawanie obrazu i dźwięku wyłączone"); */
	}
	// TODO TCP
	makeWindowBlack(drawing_area);
	/* zaczernia obszar wyświetlania */
}

gboolean video_running = TRUE;
toggle_video() {
	if (video_running == FALSE) {
		video_start();
		/* wywołuje funkcję audio_receive z opóźnieniem 500 milisekund */
		g_timeout_add(500, audio_receive, NULL);
		video_running = TRUE;
	} else {
		video_stop();
		video_running = FALSE;
	}
}

/* było char */
unsigned char packet_stick_UDP[2];
void wyslij_stick_UDP(unsigned int number, short value) {
	/*
	 pierwszy bajt: jedynka, trzybitowy kod osi, cztery bity wychylenia
	 drugi bajt: jedynka, siedem bitów wychylenia
	 */
	if (number == 0) {
		/* jeśli zdarzenie dotyczy osi X to wartości trzeba przeskalować do mniejszego zakresu
		 bo kąt wychylenia serwomechanizmu odpowiadającego za skręt kół jest ograniczony */
		value = (65.536 - value) / 178 + 500;
		/* zakres od 251 do 749
		 żeby zwiększyć zakres trzeba zmniejszyć mianownik, na przykład przy mianowniku 110 zakres wynosi od 203 do 798 */
		packet_stick_UDP[0] = 128 + number * 16 + value / 64;
		packet_stick_UDP[1] = 128 + (value % 64);

		/* FIXME: zmienić kodowanie na oparte na działaniach "<<",">>" oraz "|"
		 http://pl.wikipedia.org/wiki/Przesunięcie_bitowe
		 http://guidecpp.cal.pl/cplus,operators-bits
		 */
	} else {
		/* pozostałe osie */
		value = (value + 65.536) / 65.536 + 500;
		packet_stick_UDP[0] = 128 + number * 16 + value / 64;
		packet_stick_UDP[1] = 128 + (value % 64);

		/* FIXME: zmienić kodowanie */

		/*
		 przeskalowuje na zakres 1-1000
		 tracę część rozdzielczości ale mieszczę się w dwóch bajtach
		 przedział przy 10 pozycjach wynosi 1024, czyli i tak więcej niż rozdzielczość serwokontrolera wynosząca 1000 pozycji
		 */
	}
	bytes_sent = sendto(sock, packet_stick_UDP, 2, 0, (struct sockaddr*) &sa,
			sizeof(struct sockaddr_in));
	if (bytes_sent < 0) {
		printf("Error sending packet (wyslij_stick_UDP): %s\n",
				strerror(errno));
	}
	printf("%2hd△ numer osi:%2hd\twychylenie:%6hd\n", bytes_sent,
			(packet_stick_UDP[0] - 128) / 16,
			(packet_stick_UDP[0] % 16) * 64 + packet_stick_UDP[1] - 128);
}

/*2015*/
char packet_stick_TCP[3];
void wyslij_stick_TCP(unsigned int number, short value) {

	// FIXME
	packet_stick_TCP[0] = number + 40;
	packet_stick_TCP[1] = (value >> 8) & 0xFF;
	packet_stick_TCP[2] = value & 0xFF;

	if (write(SocketFD, packet_stick_TCP, 3) < 0) {
		printf("oś %d, wychylenie [%d, %d] = %d, tryb: 2, Błąd!\n",
				packet_stick_TCP[0] - 40, packet_stick_TCP[1],
				packet_stick_TCP[2],
				packet_stick_TCP[1] * 256 + packet_stick_TCP[2]);
	} else {
		printf("oś %d, wychylenie [%d, %d] = %d, tryb: 2, wysłano %d bajtów\n",
				packet_stick_TCP[0] - 40, packet_stick_TCP[1],
				packet_stick_TCP[2],
				packet_stick_TCP[1] * 256 + packet_stick_TCP[2],
				sizeof(packet_stick_TCP));
	}
}

/* wyświetla tekst na pasku stanu u dołu okna */
static void push_item(GtkWidget *widget, gpointer data, char* message) {
	static int count = 1;
	gchar *buff;
	buff = g_strdup_printf("%s", message);
	gtk_statusbar_push(GTK_STATUSBAR(widget), GPOINTER_TO_INT(data), buff);
	g_free(buff);
}

static void pop_item(GtkWidget *widget, gpointer data) {
	gtk_statusbar_pop(GTK_STATUSBAR(widget), GPOINTER_TO_INT(data));
}

unsigned char packet_dwustan_UDP[1];
/* wysyła przez UDP sygnały dotyczące przycisku joysticka lub klawisza */
void wyslij_dwustan_UDP(unsigned char number, short value) {
	/* parzyste zwolnienie, nieparzyste wciśnięcie */
	packet_dwustan_UDP[0] = 2 * (number + 1) + value;

	/*
	 FIXME:
	 trzeci parametr funkcji sendto(); to ilość bajtów do wysłania
	 funkcja snprintf() nadaje się tylko do sprawdzania długości stringów
	 zastosować inną funkcję!
	 */

	bytes_sent = sendto(sock, packet_dwustan_UDP, 1, 0, (struct sockaddr*) &sa,
			sizeof(struct sockaddr_in));
	/* wysyła na gniazdo sock, pakiet packet_dwustan_UDP, o długości strlen(packet_dwustan_UDP)... */

	if (bytes_sent < 0) {
		printf("Error sending packet (wyslij_dwustan_UDP): %s\n",
				strerror(errno));
	}

	printf("%2hd△ numer przycisku: %2hd\tstan:%2hd\n", bytes_sent,
			(packet_dwustan_UDP[0] / 2) - 1, (packet_dwustan_UDP[0] % 2));
}

unsigned char packet_dwustan_TCP[1];
/* wysyła przez TCP sygnały dotyczące przycisku joysticka lub klawisza */
void wyslij_dwustan_TCP(unsigned char number, short value) {
	/* parzyste zwolnienie, nieparzyste wciśnięcie */
	packet_dwustan_TCP[0] = 2 * (number + 1) + value;
	if (write(SocketFD, packet_dwustan_TCP, 1) < 0) {
		printf("numer przycisku: %2hd\tstan:%2hd, Błąd!\n",
				(packet_dwustan_TCP[0] / 2) - 1, (packet_dwustan_TCP[0] % 2));
	} else {
		printf("numer przycisku: %2hd\tstan:%2hd, wysłano %d bajtów\n",
				(packet_dwustan_TCP[0] / 2) - 1, (packet_dwustan_TCP[0] % 2),
				sizeof(packet_dwustan_TCP[0]));
	}
}

/* w zależności od wpisanego w pliku konfiguracyjnym trybu wysyłania
 poniższe funkcje wywołują odpowiednie funkcje przekazując im argumenty */

void wyslij_dwustan(unsigned char number, short value) {
	switch (tryb_wysylania) {
	case 0:
		printf("przycisk %d, stan %d, tryb: 2\n", number, value);
		break;
	case 1:
		wyslij_dwustan_UDP(number, value); /* wysyła protokołem UDP */
		break;
	case 2:
		wyslij_dwustan_TCP(number, value); /* wysyła protokołem TCP */
		break;
	case 3:
		printf("przycisk %d, stan %d, tryb: 3\n", number, value); /* nie użyta */
		break;
	}
}

/* przełącza stan checkboxa */
void toggle_serwokontroler_button_callback(
		GtkWidget *check_button_serwokontroler, gpointer data) {
	if (gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(check_button_serwokontroler))) {
		wyslij_dwustan(14, 1);
		/* wysyła sygnał załączenia serwokontrolera */
		push_item(statusbar, GINT_TO_POINTER(context_id),
				"serwokontroler włączony");
		/* wyświetla wiadomość na pasku stanu */
	}

	else {
		wyslij_dwustan(14, 0);
		/* wysyła sygnał wyłączenia serwokontrolera */
		push_item(statusbar, GINT_TO_POINTER(context_id),
				"serwokontroler wyłączony");
		/* wyświetla wiadomość na pasku stanu */
	}
}

/* przełącza stan checkboxa */
void toggle_regulator_button_callback(GtkWidget *check_button_regulator,
		gpointer data) {
	if (gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(check_button_regulator))) {
		wyslij_dwustan(9, 1);
		/* wysyła sygnał załączenia regulatora: przycisk 9 wciśnięty */
		push_item(statusbar, GINT_TO_POINTER(context_id), "regulator włączony");

	} else {
		wyslij_dwustan(8, 1);
		/* wysyła sygnał wyłączenia regulatora:	przycisk 8 wciśnięty */
		push_item(statusbar, GINT_TO_POINTER(context_id),
				"regulator wyłączony");
	}
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

int wyslij_stick_ponownie() {
	if (stick_y == 0) {
		return FALSE;
	}
	/* jeśli joystick wraca do pozycji neutralnej to funkcja zwraca wartość FALSE i nie jest więcej wywoływana, a informacje o wychyleniu w osi Y nie są przesyłane */

	else
		/* jeśli wychylenie joysticka w osi Y jest inne niż 0 to poniższe instrukcje są powtarzane regularnie, zależnie od wybranego trybu wysyłania */
		switch (tryb_wysylania) {
		case 0:
			printf("oś %d, wychylenie %d, tryb: 0\n", 1, stick_y);
			break;
		case 1:
			wyslij_stick_UDP(1, stick_y);
			break;
		case 2:
			wyslij_stick_TCP(1, stick_y);
			break;
		case 3:
			printf("oś %d, wychylenie %d, tryb: 3\n", 1, stick_y);
			break;
		}
}

void wyslij_stick(unsigned int number, short value) {
	/* powtarza wysyłanie co pewnien czas
	 zabezpieczenie przed wyjechaniem pojazdu z zasięgu */
	if (number == 1) {/* jeśli zdarzenie dotyczy osi Y */
		g_timeout_add(500, wyslij_stick_ponownie, NULL);
		/* powtarzaj funkcję wyslij_stick_ponownie co zadaną liczbę milisekund */
		/* zmienna failsafe w odbiorniku powinna mieć wartość nieco wyższą */
	}

	/* prędkość ruchu ramienia nie jest wysyłana bo powinna być raczej obliczna
	 w odbiorniku na podstawie różnicy czasu dotarcia pakietów */
	switch (tryb_wysylania) {
	case 0:
		printf("oś %d, wychylenie %d, tryb: 0\n", number, value);
		break;
	case 1:
		wyslij_stick_UDP(number, value);
		break;
	case 2:
		wyslij_stick_TCP(number, value);
		break;
	case 3:
		printf("oś %d, wychylenie %d, tryb: 3\n", number, value);
		break;
	}
}

/* funkcje wysyłające pakiety przy kliknięciu przycisków w oknie */
void wyslij_przycisk_13_on() {
	wyslij_dwustan(13, 1);
}
void wyslij_przycisk_13_off() {
	wyslij_dwustan(13, 0);
}
/* wyłącza odbiornik */
void wyslij_przycisk_12_on() {
	wyslij_dwustan(12, 1);
}
void wyslij_przycisk_12_off() {
	wyslij_dwustan(12, 0);
}

/*
 poniżej funkcje wywoływane przy kliknięciu poszczególnych przycisków strzałek

 przyciski: 2, 4, 8, 6
 ustawiają tylko jedną oś, a ich puszczenie zeruje tylko jedną oś

 przyciski: 1, 3, 7, 9
 ustawiają dwie osie i zerują dwie osie

 przycisk 5 zeruje obie osie przy kliknięciu i nie robi nic przy zwolnieniu
 */

/* przyciski wystarczają do sterowania pojazdami gąsienicowymi		*/
/* do sterowania pojazdami ze skręcanymi kołami lepiej nadaje się	*/
/* klawiatura bądź gamepad analogowy								*/

void num_arrow_9() {
	stick_x = 32767;
	sprintf(wychylenie_x, "%d", stick_x);
	gtk_label_set_text(label_x, wychylenie_x);
	wyslij_stick(0, stick_x);

	stick_y = -32768;
	sprintf(wychylenie_y, "%d", stick_y);
	gtk_label_set_text(label_y, wychylenie_y);
	wyslij_stick(1, stick_y);
}

void num_arrow_8() {
	stick_y = -32768;
	sprintf(wychylenie_y, "%d", stick_y);
	gtk_label_set_text(label_y, wychylenie_y);
	wyslij_stick(1, stick_y);
}

void num_arrow_7() {
	stick_x = -32768;
	sprintf(wychylenie_x, "%d", stick_x);
	gtk_label_set_text(label_x, wychylenie_x);
	wyslij_stick(0, stick_x);

	stick_y = -32768;
	sprintf(wychylenie_y, "%d", stick_y);
	gtk_label_set_text(label_y, wychylenie_y);
	wyslij_stick(1, stick_y);
}

void num_arrow_6() {
	stick_x = 32767;
	sprintf(wychylenie_x, "%d", stick_x);
	gtk_label_set_text(label_x, wychylenie_x);
	wyslij_stick(0, stick_x);
}

void num_arrow_4() {
	stick_x = -32768;
	sprintf(wychylenie_x, "%d", stick_x);
	gtk_label_set_text(label_x, wychylenie_x);
	wyslij_stick(0, stick_x);
}

void num_arrow_3() {
	stick_x = 32767;
	sprintf(wychylenie_x, "%d", stick_x);
	gtk_label_set_text(label_x, wychylenie_x);
	wyslij_stick(0, stick_x);

	stick_y = 32767;
	sprintf(wychylenie_y, "%d", stick_y);
	gtk_label_set_text(label_y, wychylenie_y);
	wyslij_stick(1, stick_y);
}

void num_arrow_2() {
	stick_y = 32767;
	sprintf(wychylenie_y, "%d", stick_y);
	gtk_label_set_text(label_y, wychylenie_y);
	wyslij_stick(1, stick_y);
}

void num_arrow_1() {
	stick_x = -32768;
	sprintf(wychylenie_x, "%d", stick_x);
	gtk_label_set_text(label_x, wychylenie_x);
	wyslij_stick(0, stick_x);

	stick_y = 32767;
	sprintf(wychylenie_y, "%d", stick_y);
	gtk_label_set_text(label_y, wychylenie_y);
	wyslij_stick(1, stick_y);
}

void no_num_arrow() {
	/* funkcja wywoływana przy zwalnianiu przycisków po przekątnych */
	stick_x = 0;
	sprintf(wychylenie_x, "%d", stick_x);
	gtk_label_set_text(label_x, wychylenie_x);
	stick_y = 0;
	sprintf(wychylenie_y, "%d", stick_y);
	gtk_label_set_text(label_y, wychylenie_y);

	wyslij_stick(0, stick_x);
	/* 0-oś X, wychylenie joysticka */
	wyslij_stick(1, stick_y);
	/* 1-oś Y, wychylenie joysticka */
}
/* funkcje wywoływane przy zwalnianiu przycisków na krzyżu */
void no_num_arrow_x() {
	stick_x = 0;
	sprintf(wychylenie_x, "%d", stick_x);
	gtk_label_set_text(label_x, wychylenie_x);
	wyslij_stick(0, stick_x);
}

void no_num_arrow_y() {
	stick_y = 0;
	sprintf(wychylenie_y, "%d", stick_y);
	gtk_label_set_text(label_y, wychylenie_y);
	wyslij_stick(1, stick_y);
}

void klawisz(GtkWidget *widget, GdkEventKey *event, int stan) { /* parametr stan jest przekazywany do funkcji i przyjmuje wartości 1 lub 0 */

	uint klawisz;
	klawisz = ((GdkEventKey*) event)->keyval;
	uint klawisz_hardware;
	klawisz_hardware = ((GdkEventKey*) event)->hardware_keycode;
	uint czas;
	czas = ((GdkEventKey*) event)->time;

	if (stan == 1) {
		printf("klawisz wcisnięty: %6X,%6X,%8hd\n", klawisz, klawisz_hardware,
				czas);
		if (klawisz == 0xff51) /* strzalka w lewo		*/
		{
			stick_x = -32767;
			sprintf(wychylenie_x, "%d", stick_x);
			gtk_label_set_text(label_x, wychylenie_x);
			wyslij_stick(0, stick_x);
		}
		if (klawisz == 0xff53) /* strzalka w prawo	*/
		{
			stick_x = 32766;
			sprintf(wychylenie_x, "%d", stick_x);
			gtk_label_set_text(label_x, wychylenie_x);
			wyslij_stick(0, stick_x);
		}
		if (klawisz == 0xff52) /* strzalka w górę		*/
		{
			stick_y = -32767;
			sprintf(wychylenie_y, "%d", stick_y);
			gtk_label_set_text(label_y, wychylenie_y);
			wyslij_stick(1, stick_y);
		}
		if (klawisz == 0xff54) /* strzalka w dół		*/
		{
			stick_y = 32766;
			sprintf(wychylenie_y, "%d", stick_y);
			gtk_label_set_text(label_y, wychylenie_y);
			wyslij_stick(1, stick_y);
		}

		/* FIXME: dodać obsługę klawiatury numerycznej  */

		if (klawisz == 'f') /* klawisz f */
		{
			printf("zmiana trybu fullscreen\n");
			switch_Fullscreen();
		}
	}
	if (stan == 0) {
		printf("klawisz zwolniony: %6X,%6X,%8hd\n", klawisz, klawisz_hardware,
				czas);
		if (klawisz == 0xff51) /* strzalka w lewo zwolniona */
		{
			stick_x = 0;
			sprintf(wychylenie_x, "%d", stick_x);
			gtk_label_set_text(label_x, wychylenie_x);
			wyslij_stick(0, stick_x);
		}
		if (klawisz == 0xff53) /* strzalka w prawo zwolniona */
		{
			stick_x = 0;
			sprintf(wychylenie_x, "%d", stick_x);
			gtk_label_set_text(label_x, wychylenie_x);
			wyslij_stick(0, stick_x);
		}
		if (klawisz == 0xff52) /* strzalka w górę zwolniona */
		{
			stick_y = 0;
			sprintf(wychylenie_y, "%d", stick_y);
			gtk_label_set_text(label_y, wychylenie_y);
			wyslij_stick(1, stick_y);
		}
		if (klawisz == 0xff54) /* strzalka w dół zwolniona */
		{
			stick_y = 0;
			sprintf(wychylenie_y, "%d", stick_y);
			gtk_label_set_text(label_y, wychylenie_y);
			wyslij_stick(1, stick_y);
		}
		/* FIXME: dodać obsługę klawiatury numerycznej  */
	}
}

int joystick_fd, rc;
/* dopisany parametr */
int open_joystick(char *joystick_device) {
	joystick_fd = open(joystick_descriptor, O_RDWR | O_NONBLOCK | O_NOCTTY);
	//joystick_fd = open(joystick_descriptor, O_RDONLY | O_NONBLOCK | O_NOCTTY);
	/* joystick_fd to port joysticka */
	if (joystick_fd < 0)
		return joystick_fd;
	/* maybe ioctls to interrogate features here? */
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
/* warning */
void zczytuj_joystick() {
	rc = read_joystick_event(&jse);
	usleep(1000);
	if (rc == 1) {

		if (jse.type == JS_EVENT_AXIS)

		/* FIXME: w trybie dyskretnym zdarzenia prawego joysticka są identyczne ze zdarzeniami przycisków 0-3
		 trzeba poprawić rozróżnianie zdarzeń, to może być zależne modelu podłączonego joysticka */

		{
			if (jse.number == 0) {
				stick_x = jse.value;
				sprintf(wychylenie_x, "%d", stick_x);
				gtk_label_set_text(label_x, wychylenie_x);
				wyslij_stick(0, stick_x);
				if (stick_x != 0) {
					play_rumble_effect(RUMBLE_WEAK_RUMBLE_EFFECT);
				}
				/* oś X (0), wychylenie joysticka, prędkość */
			} else if (jse.number == 1) {
				stick_y = jse.value;
				sprintf(wychylenie_y, "%d", stick_y);
				gtk_label_set_text(label_y, wychylenie_y);
				wyslij_stick(1, stick_y);
				if (stick_y != 0) {
					play_rumble_effect(RUMBLE_STRONG_RUMBLE_EFFECT);
				}
				/* oś Y (1), wychylenie joysticka, prędkość */
			} else if (jse.number == 3) {
				stick_3 = jse.value;
				printf("oś: %d wychylenie %d\n", jse.number, stick_3);
				/* drukowanie wychyleń jest tylko w konsoli, a nie w etykietach Labels */
				wyslij_stick(3, stick_3);
				if (stick_3 != 0) {
					play_rumble_effect(RUMBLE_WEAK_RUMBLE_EFFECT);
				}
				/* oś X drugiego drążka (3) */
			} /* FIXME: generuje dziwne wartości */
			/*else if (jse.number == 4) {
				stick_4 = jse.value;
				printf("oś: %d wychylenie %d\n", jse.number, stick_4);

				wyslij_stick(1, stick_4);
				if (stick_4 != 0) {
					play_rumble_effect(RUMBLE_WEAK_RUMBLE_EFFECT);
				}
			}
			else if (jse.number != 4) {
				stick_4 = jse.value;
				printf("oś: %d wychylenie %d\n", jse.number, stick_4);

				wyslij_stick(2, stick_4);
				if (stick_4 != 0) {
					play_rumble_effect(RUMBLE_WEAK_RUMBLE_EFFECT);
				}
			*/
			}
			/* inne osie */
		}

		if (jse.type == JS_EVENT_BUTTON) {
			/* naciskanie przycisków */
			/* FIXME: dodać możliwość przypisywanie klawiszom/ przyciskom działania za pomocą pliku config.cfg */
			if (jse.value == 1)

			{
				switch (jse.number) {
				case 0: /* wyslij_dwustan(0,1); */
					break;
				case 1:
					wyslij_dwustan(1, 1);
					break;
				case 2: /* wyslij_dwustan(2,1); */
					break;
				case 3:
					wyslij_dwustan(3, 1);
					break;
				case 4:
					wyslij_dwustan(4, 1);
					//play_rumble_effect(RUMBLE_DAMPING_EFFECT);
					break;
				case 5:
					wyslij_dwustan(5, 1);
					//play_rumble_effect(RUMBLE_SPRING__EFFECT);
					break;
				case 6:
					wyslij_dwustan(6, 1);
					//play_rumble_effect(RUMBLE_SINE_VIBE_EFFECT);
					break;
				case 7:
					wyslij_dwustan(7, 1);
					//play_rumble_effect(RUMBLE_CONSTANT_FORCE_EFFECT);
					break;
				case 8:
					wyslij_dwustan(8, 1);
					//stop_all_rumble_effects();
					break;
				case 9:
					wyslij_dwustan(9, 1);
					break;
				case 10:
					wyslij_dwustan(10, 1); /* wciśnięcie lewego joysticka */
					break;
				case 11:
					wyslij_dwustan(11, 1); /* wciśnięcie prawego joysticka */
					break;
				default:
					printf("przycisk %u nacisniety\n", jse.number);
					break;
				}
				virgin_buttons = FALSE;
				/* rozplombowuje przyciski */
			}

			/* zwalnianie przyciskow */
			if (jse.value == 0 && virgin_buttons == FALSE) {
				switch (jse.number) {
				case 0: /* wyslij_dwustan(0,0); */
					break;
				case 1:
					wyslij_dwustan(1, 0);
					break;
				case 2: /* wyslij_dwustan(2,0); */
					break;
				case 3:
					wyslij_dwustan(3, 0);
					break;
				case 4:
					wyslij_dwustan(4, 0);
					stop_all_rumble_effects();
					break;
				case 5:
					wyslij_dwustan(5, 0);
					stop_all_rumble_effects();
					break;
				case 6:
					wyslij_dwustan(6, 0);
					stop_all_rumble_effects();
					break;
				case 7:
					wyslij_dwustan(7, 0);
					stop_all_rumble_effects();
					break;
				case 8:
					wyslij_dwustan(8, 0);
					break;
				case 9:
					wyslij_dwustan(9, 0);
					break;
				case 10:
					wyslij_dwustan(10, 0); /* zwolnienie lewego joysticka */
					break;
				case 11:
					wyslij_dwustan(11, 0); /* zwolnienie prawego joysticka */
					break;
				default:
					printf("przycisk %u zwolniony\n", jse.number);
					break;
				}
				/* przycisk ma zazwyczaj tylko dwa stany */
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
	gst_init(&argc, &argv);
	gtk_init(&argc, &argv);

	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(main_window), "telerobot client");
	/* gdk_window_set_icon_list(GTK_WINDOW(main_window), NULL); */
	gtk_window_set_default_size(GTK_WINDOW(main_window), 973, 309); /* faktycznie 1024, 254 */
	gtk_window_set_position(GTK_WINDOW(main_window), GTK_WIN_POS_CENTER);
	/* gtk_widget_show(main_window); */
	left_table = gtk_table_new(3, 3, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(left_table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(left_table), 2);

	drawing_area_frame = gtk_aspect_frame_new(NULL, 0.5, 0.0, 1.33333,
			TRUE /* ignore child's aspect */);

	gtk_frame_set_shadow_type(GTK_FRAME(drawing_area_frame),
			GTK_SHADOW_ETCHED_IN);

	drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(drawing_area, 320, 240);
	/* ustawia rozmiar obszaru wyświetlania obrazu z kamery */
	gtk_container_add(GTK_CONTAINER(drawing_area_frame), drawing_area);

	/* tworzenie przycisków strzałek i umieszczanie ich w odpowiednich komórkach tabeli */
	GtkWidget *button_num_arrow_9;
	button_num_arrow_9 = gtk_button_new_with_label("▲▢");
	gtk_table_attach_defaults(GTK_TABLE(left_table), button_num_arrow_9, 2, 3,
			0, 1);
	GtkWidget *button_num_arrow_8;
	button_num_arrow_8 = gtk_button_new_with_label("▲▲");
	gtk_table_attach_defaults(GTK_TABLE(left_table), button_num_arrow_8, 1, 2,
			0, 1);
	GtkWidget *button_num_arrow_7;
	button_num_arrow_7 = gtk_button_new_with_label("▢▲");
	gtk_table_attach_defaults(GTK_TABLE(left_table), button_num_arrow_7, 0, 1,
			0, 1);
	GtkWidget *button_num_arrow_4;
	button_num_arrow_4 = gtk_button_new_with_label("▼▲");
	gtk_table_attach_defaults(GTK_TABLE(left_table), button_num_arrow_4, 0, 1,
			1, 2);
	GtkWidget *button_num_arrow_5;
	button_num_arrow_5 = gtk_button_new_with_label("▢▢");
	gtk_table_attach_defaults(GTK_TABLE(left_table), button_num_arrow_5, 1, 2,
			1, 2);
	GtkWidget *button_num_arrow_6;
	button_num_arrow_6 = gtk_button_new_with_label("▲▼");
	gtk_table_attach_defaults(GTK_TABLE(left_table), button_num_arrow_6, 2, 3,
			1, 2);
	GtkWidget *button_num_arrow_3;
	button_num_arrow_3 = gtk_button_new_with_label("▢▼");
	gtk_table_attach_defaults(GTK_TABLE(left_table), button_num_arrow_3, 2, 3,
			2, 3);
	GtkWidget *button_num_arrow_2;
	button_num_arrow_2 = gtk_button_new_with_label("▼▼");
	gtk_table_attach_defaults(GTK_TABLE(left_table), button_num_arrow_2, 1, 2,
			2, 3);
	GtkWidget *button_num_arrow_1;
	button_num_arrow_1 = gtk_button_new_with_label("▼▢");
	gtk_table_attach_defaults(GTK_TABLE(left_table), button_num_arrow_1, 0, 1,
			2, 3);

	tank_controlls = gtk_aspect_frame_new(NULL, /* label */
	0.5, /* pozycja na osi x */
	0.0, /* pozycja na osi y */
	1, /* xsize/ysize = 1 */
	FALSE /* ignore child's aspect */);

	gtk_frame_set_shadow_type(GTK_FRAME(tank_controlls), GTK_SHADOW_ETCHED_IN);
	/* tworzy ozdobną ramkę woków strzałek */
	gtk_container_add(GTK_CONTAINER(tank_controlls), left_table);
	/* umieszcza numpad kierujący czołgiem w ramce */
	main_table = gtk_table_new(1, 3, FALSE);
	/* tworzy główną tabelę z trzema kolumnami i jednym wierszem */
	gtk_table_attach_defaults(GTK_TABLE(main_table), tank_controlls, 0, 1, 0,
			1);
	/* zagnieżdża strzałki do kierowania czołgiem w lewej komórce głównej tabeli */
	gtk_table_attach_defaults(GTK_TABLE(main_table), drawing_area_frame, 1, 2,
			0, 1);
	/* zagnieżdża ramkę video drugiej komórce głównej tabeli */

	/* prawa komórka głównej tabeli */
	right_table = gtk_table_new(3, 3, TRUE);
	gtk_table_set_row_spacings(GTK_TABLE(right_table), 2);
	gtk_table_set_col_spacings(GTK_TABLE(right_table), 2);

	/* prawa ramka */
	wskazniki = gtk_aspect_frame_new(NULL, /* label */
	0.5, /* wyrównanie w poziomie */
	0.0, /* wyrównanie w pionie */
	1, /* xsize/ysize = 1 */
	FALSE /* ignore child's aspect */);

	/* zamiast "0" może być NULL - wtedy pole jest puste */
	label_x = gtk_label_new("0");
	label_y = gtk_label_new("0");

	/*
	 label_3 = gtk_label_new("0");
	 label_4 = gtk_label_new("0");
	 etykiety osi prawego joysticka wyłączyłem bo sprawiały problemy
	 */

	/* 0 - pierwsza kolumna, 1 - druga kolumna następna (numerowane od zera) */

	/* umieszcza wskaźniki w polach tabeli */
	gtk_table_attach_defaults(GTK_TABLE(right_table), label_x, 0, 1, 0, 1);
	gtk_label_set_justify(label_x, GTK_JUSTIFY_RIGHT);
	gtk_table_attach_defaults(GTK_TABLE(right_table), label_y, 1, 2, 0, 1);
	gtk_label_set_justify(label_y, GTK_JUSTIFY_RIGHT);
	/* "0,2,Y,Y" oznacza że zajmują dwie komórki tabeli */

	/*
	 gtk_table_attach_defaults(GTK_TABLE(right_table), label_3, 1, 2, 0, 1 );
	 gtk_table_attach_defaults(GTK_TABLE(right_table), label_4, 1, 2, 1, 2 );
	 etykiety osi prawego joysticka wyłączyłem bo sprawiały problemy
	 */

	/* typ obwiedni */
	gtk_frame_set_shadow_type(GTK_FRAME(wskazniki), GTK_SHADOW_ETCHED_IN);
	/* umieszcza right_table w wskazniki */
	gtk_container_add(GTK_CONTAINER(wskazniki), right_table);

	gtk_table_attach_defaults(GTK_TABLE(main_table), wskazniki, 2, 3, 0, 1);
	/* umieszcza ramkę wskazniki w prawej (2,3) komórce głównej tabeli */

	/* łączy przyciski z akcją */
	g_signal_connect(button_num_arrow_9, "pressed", G_CALLBACK(num_arrow_9),
			NULL);
	g_signal_connect(button_num_arrow_9, "released", G_CALLBACK(no_num_arrow),
			NULL);

	g_signal_connect(button_num_arrow_8, "pressed", G_CALLBACK(num_arrow_8),
			NULL);
	g_signal_connect(button_num_arrow_8, "released", G_CALLBACK(no_num_arrow_y),
			NULL);

	g_signal_connect(button_num_arrow_7, "pressed", G_CALLBACK(num_arrow_7),
			NULL);
	g_signal_connect(button_num_arrow_7, "released", G_CALLBACK(no_num_arrow),
			NULL);

	g_signal_connect(button_num_arrow_6, "pressed", G_CALLBACK(num_arrow_6),
			NULL);
	g_signal_connect(button_num_arrow_6, "released", G_CALLBACK(no_num_arrow_x),
			NULL);

	g_signal_connect(button_num_arrow_5, "pressed", G_CALLBACK(no_num_arrow),
			NULL);
	g_signal_connect(button_num_arrow_5, "released", G_CALLBACK(no_num_arrow),
			NULL);
	/* g_signal_connect(button_num_arrow_5, "clicked", G_CALLBACK(no_num_arrow), NULL);  */

	g_signal_connect(button_num_arrow_4, "pressed", G_CALLBACK(num_arrow_4),
			NULL);
	g_signal_connect(button_num_arrow_4, "released", G_CALLBACK(no_num_arrow_x),
			NULL);

	g_signal_connect(button_num_arrow_3, "pressed", G_CALLBACK(num_arrow_3),
			NULL);
	g_signal_connect(button_num_arrow_3, "released", G_CALLBACK(no_num_arrow),
			NULL);

	g_signal_connect(button_num_arrow_2, "pressed", G_CALLBACK(num_arrow_2),
			NULL);
	g_signal_connect(button_num_arrow_2, "released", G_CALLBACK(no_num_arrow_y),
			NULL);

	g_signal_connect(button_num_arrow_1, "pressed", G_CALLBACK(num_arrow_1),
			NULL);
	g_signal_connect(button_num_arrow_1, "released", G_CALLBACK(no_num_arrow),
			NULL);

	gtk_signal_connect(GTK_OBJECT(main_window), "key_press_event",
			G_CALLBACK(klawisz), 1);
	gtk_signal_connect(GTK_OBJECT(main_window), "key_release_event",
			G_CALLBACK(klawisz), 0);

	/* tworzy przycisk wyłączający serwer */
	/* TODO: dodać okienko z pytaniem o potwierdzenie */
	quit_icon = gtk_image_new_from_file("icons/system-shutdown.svg");
	quit_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(quit_button), quit_icon);
	g_signal_connect(quit_button, "pressed", G_CALLBACK(wyslij_przycisk_13_on),
			NULL);
	g_signal_connect(quit_button, "released",
			G_CALLBACK(wyslij_przycisk_13_off), NULL);
	gtk_table_attach_defaults(GTK_TABLE(right_table), quit_button, 2, 3, 0, 1);
	gtk_button_set_relief(quit_button, GTK_RELIEF_NONE);

	/* Sets whether the button will grab focus when it is clicked with the mouse.
	 Making mouse clicks not grab focus is useful in places like toolbars
	 where you don't want the keyboard focus removed from the main area of the application. */
	/* gtk_button_set_focus_on_click(quit_button, 0); */

	/* tworzy przycisk fullscreen enable */
	fullscreen_enable_icon = gtk_image_new_from_file(
			"icons/view-fullscreen.svg");
	fullscreen_enable_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(fullscreen_enable_button),
			fullscreen_enable_icon);
	g_signal_connect(fullscreen_enable_button, "clicked",
			G_CALLBACK(switch_Fullscreen), NULL);
	gtk_table_attach_defaults(GTK_TABLE(right_table), fullscreen_enable_button,
			2, 3, 1, 2);
	gtk_button_set_relief(fullscreen_enable_button, GTK_RELIEF_NONE);

	/* tworzy przycisk fullscreen disable */
	fullscreen_disable_icon = gtk_image_new_from_file(
			"icons/view-windowed.svg");
	fullscreen_disable_button = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(fullscreen_disable_button),
			fullscreen_disable_icon);
	g_signal_connect(fullscreen_disable_button, "clicked",
			G_CALLBACK(switch_Fullscreen), NULL);
	gtk_table_attach_defaults(GTK_TABLE(right_table), fullscreen_disable_button,
			2, 3, 1, 2);
	gtk_button_set_relief(fullscreen_disable_button, GTK_RELIEF_NONE);

	/* tworzy przycisk video */
	video_icon = gtk_image_new_from_file("icons/camera-web.svg");
	check_button_video = gtk_check_button_new();
	gtk_toggle_button_set_active(check_button_video, TRUE);
	g_signal_connect(check_button_video, "clicked", G_CALLBACK(toggle_video),
			NULL);
	gtk_table_attach_defaults(GTK_TABLE(right_table), video_icon, 1, 2, 2, 3);
	gtk_table_attach_defaults(GTK_TABLE(right_table), check_button_video, 1, 2,
			2, 3);

	/* tworzy przyciski record_start_button oraz record_stop_button */
	/*
	 record_start_icon= gtk_image_new_from_file ("icons/media-record.svg");
	 record_start_button= gtk_button_new();
	 gtk_container_add (GTK_CONTAINER (record_start_button), record_start_icon);
	 // g_signal_connect(record_start_button, "clicked", G_CALLBACK(recording_start), NULL);
	 gtk_table_attach_defaults(GTK_TABLE(right_table), record_start_button, 2, 3, 2, 3 );
	 gtk_button_set_relief(GTK_BUTTON(record_start_button), GTK_RELIEF_NONE);

	 record_stop_icon= gtk_image_new_from_file ("icons/media-playback-stop.svg");
	 record_stop_button= gtk_button_new();
	 gtk_container_add (GTK_CONTAINER (record_stop_button), record_stop_icon);
	 // g_signal_connect(record_stop_button, "clicked", G_CALLBACK(recording_stop), NULL);
	 gtk_table_attach_defaults(GTK_TABLE(right_table), record_stop_button, 2, 3, 2, 3 );
	 gtk_button_set_relief(GTK_BUTTON(record_stop_button), GTK_RELIEF_NONE);
	 */

	/* tworzy checkbox i ikonkę joysticka */
	joystick_icon = gtk_image_new_from_file("icons/applications-games.svg");
	check_button_joystick = gtk_check_button_new();
	/* gtk_container_add (GTK_CONTAINER (check_button_joystick), joystick_icon); */
	/* łączenie sygnału przeniesione za pierwsze wykrycie joysticka */
	gtk_table_attach_defaults(GTK_TABLE(right_table), joystick_icon, 0, 1, 2,
			3);
	gtk_table_attach_defaults(GTK_TABLE(right_table), check_button_joystick, 0,
			1, 2, 3);

	check_button_serwokontroler = gtk_check_button_new_with_label("sk");
	/* jeśli set_active znajduje się w tym miejscu to nie generuje zdarzenia */
	gtk_toggle_button_set_active(check_button_serwokontroler, TRUE);
	g_signal_connect(check_button_serwokontroler, "clicked",
			G_CALLBACK(toggle_serwokontroler_button_callback), NULL);
	gtk_table_attach_defaults(GTK_TABLE(right_table),
			check_button_serwokontroler, 0, 1, 1, 2);
	check_button_regulator = gtk_check_button_new_with_label("reg");
	gtk_table_attach_defaults(GTK_TABLE(right_table), check_button_regulator, 1,
			2, 1, 2);
	gtk_toggle_button_set_active(check_button_regulator, TRUE);
	g_signal_connect(check_button_regulator, "clicked",
			G_CALLBACK(toggle_regulator_button_callback), NULL);
	/* źródło, stan który musi wystąpić, dane do przekazania */

	/* obszar pokazujący pozycję joysticka */
	/*
	 grzybek_lewy = gtk_drawing_area_new ();
	 gtk_widget_set_size_request (grzybek_lewy, 48, 48);
	 gtk_table_attach_defaults(GTK_TABLE(right_table), grzybek_lewy, 0, 1, 3, 3 );
	 g_signal_connect (G_OBJECT (grzybek_lewy), "draw", G_CALLBACK (draw_callback), NULL);
	 */

	/* pakowanie głównych elementów okna programu: */
	vbox = gtk_vbox_new(FALSE, 2);
	statusbar = gtk_statusbar_new();
	gtk_container_add(GTK_CONTAINER(main_window), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(main_table), 5);
	/* ustawianie szerokości marginesu wokół elementu */
	gtk_box_pack_start(GTK_BOX(vbox), main_table, TRUE, TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox), statusbar, FALSE, TRUE, 1);

	/* statusbar */
	context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(statusbar),
			"Statusbar example");
	g_signal_connect_swapped(G_OBJECT(main_window), "destroy",
			G_CALLBACK(zamknij), NULL);
	/* wczytuje wartości zmiennych z pliku config.cfg */
	wczytaj_config();

	/* wyświetla wszystkie elementy okna */
	gtk_widget_show_all(main_window);

	if (tryb_wysylania == 1) {
		otworz_UDP();
	}

// TCP
	if (tryb_wysylania == 2) {
		SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (-1 == SocketFD) {
			perror("cannot create socket");
			exit(EXIT_FAILURE);
		}

		memset(&stSockAddr, 0, sizeof(stSockAddr));

		stSockAddr.sin_family = AF_INET;
		stSockAddr.sin_port = htons(6000);
		Res = inet_pton(AF_INET, "192.168.0.105", &stSockAddr.sin_addr);

		if (0 > Res) {
			perror("error: first parameter is not a valid address family");
			printf("error: first parameter is not a valid address family");
			close(SocketFD);
			exit(EXIT_FAILURE);
		} else if (0 == Res) {
			perror(
					"char string (second parameter does not contain valid ipaddress)");
			printf(
					"char string (second parameter does not contain valid ipaddress)");
			close(SocketFD);
			exit(EXIT_FAILURE);
		}

		if (-1
				== connect(SocketFD, (struct sockaddr *) &stSockAddr,
						sizeof(stSockAddr))) {
			perror("connect failed");
			printf("connect failed");
			close(SocketFD);
			exit(EXIT_FAILURE);
		} else {
			printf("TCP socket created\n");
		}
	}

	/* wysyła pakiet z numerem kamery i adresem IP na jaki ma być streamingowany obraz z kamery */
	video_start();

	/* ukrywanie przycisków do nagrywania */
	/*
	 gtk_widget_hide(record_start_button);
	 gtk_widget_hide(record_stop_button);
	 */

	/* rozpoczyna wyświetlanie obrazu */
	video_receive();

	/* uruchamia "audio_receive" z opóźnieniem 500 milisekund (właściwie co 500 milisekund) wymaga tego specyfika protokołu TCP */
	g_timeout_add(500, audio_receive, NULL);
	/* FIXME: są problemy z dźwiękiem, chyba chodzi o synchronizację dźwięku z obrazem */

	/* wykrywanie joysticka przy starcie programu */
	wykryj_joystick();

	/* w tym miejscu obydwa przyciski są wyświetlone jednocześnie! */
	/* FIXME: elementy okna powinny się pojawiać po kolei a nie wszystkie na raz za pomocą funkcji gtk_widget_show_all(main_window); */
	switch (isFullscreen) {
	case 0:
		gtk_widget_hide(fullscreen_disable_button);
		/* gtk_widget_show(fullscreen_enable_button); niepotrzebne */
		/* g_print ("widoczny pozostaje przycisk \"fullscreen_enable_button\"\n"); */
		/* gtk_window_unfullscreen(GTK_WINDOW(main_window));  niepotrzebne - domyślnie okno jest niezmaksymalizowane */
		break;

	case 1:
		gtk_widget_hide(fullscreen_enable_button);
		/* gtk_widget_show(fullscreen_disable_button); niepotrzebne */
		/* g_print ("widoczny pozostaje przycisk \"fullscreen_disable_button\"\n"); */
		gtk_window_fullscreen(GTK_WINDOW(main_window));
		break;
	}
	/* fullscreen włączony lub nie, został już tylko jeden przycisk */

	gtk_main();
	/* Out of the main loop, clean up nicely */
	g_print("Stopping playback\n");
	gst_element_set_state(pipeline, GST_STATE_NULL);

	g_print("Deleting pipeline\n");
	gst_object_unref(GST_OBJECT(pipeline));

// FIXME zdublowane
	if (tryb_wysylania == 1) {
		close(sock);
		g_print("Closing UDP socket\n");
	}

	if (tryb_wysylania == 2) {
		write(SocketFD, '\n', 2);
		(void) shutdown(SocketFD, SHUT_RDWR);
		close(SocketFD);
		g_print("Closing TCP socket\n");
	}

	/* return jest też w funkcji zamknij */
	return 0;
}
