#include <ifaddrs.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>  /* "There is some amount of overlap with sys/types.h as known by inet code" */

//UDP
int sock;
struct sockaddr_in sa;
int bytes_sent, buffer_length;
char buffer[200];

//TCP
struct sockaddr_in stSockAddr;
int Res;
int SocketFD;
GIOChannel *iochan;
size_t fromlen, recsize;
unsigned char buffer_in[1024];

/* kanał wejścia joysticka */
GIOChannel *ioch;
GError *jserror;

/* for synchronisation */
// https://stackoverflow.com/questions/15092504/how-to-time-a-function-in-milliseconds-without-boosttimer
gboolean synchronization = FALSE;
double thisTime;
long lastTime;

/* Windows */
/*
 struct      _timeb tStruct;
 struct _timeb
 {
 int   dstflag;   // holds a non-zero value if daylight saving time is in effect
 long  millitm;   // time in milliseconds since the last one-second hack
 long  time;      // time in seconds since 00:00:00 1/1/1970
 long  timezone;  // difference in minutes moving west from UTC
 };
 */

/* Linux */
struct timeval tStruct;

int wyslij_stick_ponownie(short value) {
	if (value == 0) {
		return FALSE;
	}
	/* jeśli joystick wraca do pozycji neutralnej to funkcja zwraca wartość FALSE i nie jest więcej wywoływana, a informacje o wychyleniu w osi Y nie są przesyłane */

	else
		/* jeśli wychylenie joysticka w osi Y jest inne niż 0 to poniższe instrukcje są powtarzane regularnie, zależnie od wybranego trybu wysyłania */
		switch (tryb_wysylania) {
		case 2:
			wyslij_stick_UDP(1, value);
			break;
		default:
			printf("oś %d, wychylenie %d, tryb: 3\n", 1, value);
			break;
		}
}

void wyslij_stick(unsigned int number, short value) {
	/* powtarza wysyłanie co pewnien czas
	 zabezpieczenie przed wyjechaniem pojazdu z zasięgu */
	if (number == 1 && tryb_wysylania == 2) {
		/* jeśli zdarzenie dotyczy osi Y i kierujemy pojazdem */
		g_timeout_add(500, wyslij_stick_ponownie, value);
		/* powtarzaj funkcję wyslij_stick_ponownie co zadaną liczbę milisekund */
		/* zmienna failsafe w odbiorniku powinna mieć wartość nieco wyższą */
	}

	/* prędkość ruchu ramienia nie jest wysyłana bo powinna być raczej obliczna
	 w odbiorniku na podstawie różnicy czasu dotarcia pakietów */
	switch (tryb_wysylania) {
	case 0:
		wyslij_stick_UDP(number, value);
		break;
	case 1:
		wyslij_stick_TCP(number, value);
		break;
	case 2:
		wyslij_stick_UDP_old(number, value);
		break;
	default:
		printf("oś %d, wychylenie %d, tryb: 3\n", number, value);
		break;
	}
}

/* było char */
unsigned char packet_stick_UDP[2];
void wyslij_stick_UDP_old(unsigned int number, short value) {
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
	} else {
		/* pozostałe osie */
		value = (value + 65.536) / 65.536 + 500;
		packet_stick_UDP[0] = 128 + number * 16 + value / 64;
		packet_stick_UDP[1] = 128 + (value % 64);

		/*
		 przeskalowuje na zakres 1-1000
		 tracę część rozdzielczości ale mieszczę się w dwóch bajtach
		 przedział przy 10 pozycjach wynosi 1024, czyli i tak więcej niż rozdzielczość serwokontrolera wynosząca 1000 pozycji
		 */
	}
	bytes_sent = sendto(sock, packet_stick_UDP, 2, 0, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
	if (bytes_sent < 0) {
		printf("Error sending packet (wyslij_stick_UDP): %s\n", strerror(errno));
	}
	printf("%2hd△ numer osi:%2hd\twychylenie:%6hd\n", bytes_sent, (packet_stick_UDP[0] - 128) / 16,
			(packet_stick_UDP[0] % 16) * 64 + packet_stick_UDP[1] - 128);
}

void wyslij_stick_UDP(unsigned int number, short value) {
	packet_stick_UDP[0] = number + 40;
	packet_stick_UDP[1] = (char) (value % 256);
	packet_stick_UDP[2] = (char) (value / 256);

/* Windows */
	/*_ftime(&tStruct);
	 thisTime = tStruct.time + (((double)(tStruct.millitm)) / 1000.0);

	 if (thisTime >= lastTime + 0.02)
	 {
	 bytes_sent = sendto(sock, packet_stick_UDP, 3, 0, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
	 if (bytes_sent < 0) {
	 printf("oś %d, wychylenie [%d, %d] %d, tryb: UDP, Błąd!\n", packet_stick_UDP[0] - 40, packet_stick_UDP[1], packet_stick_UDP[2], value);
	 } else {
	 printf("oś %d, wychylenie [%d, %d] %d, tryb: UDP, wysłano %d bajtów\n", packet_stick_UDP[0] - 40, packet_stick_UDP[1], packet_stick_UDP[2], value, sizeof(packet_stick_UDP));
	 }
	 lastTime = thisTime;
	 }
	 */

/* Linux */
	gettimeofday(&tStruct, 0);
	thisTime = tStruct.tv_sec + (((double) (tStruct.tv_usec)) / 1000000);

	if (synchronization) {
		if (thisTime >= lastTime + 0.02) {
			//if it's 20 or more milliseconds since last signal was sent
			bytes_sent = sendto(sock, packet_stick_UDP, 3, 0, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
			if (bytes_sent < 0) {
				printf("\"oś\": %d, wychylenie [%d, %d] %d, tryb: UDP, Błąd!\n", packet_stick_UDP[0] - 40, packet_stick_UDP[1], packet_stick_UDP[2],
						value);
			} else {
				/*printf("\"oś\": %d, wychylenie [%d, %d] %d, tryb: UDP, wysłano %d bajtów\n", packet_stick_UDP[0] - 40, packet_stick_UDP[1], packet_stick_UDP[2], value, sizeof(packet_stick_UDP));*/
			}
			lastTime = thisTime;
		} else {
			printf("blocked: %2hd / %2hd\n", tStruct.tv_sec, tStruct.tv_usec);
		}
	} else {
		bytes_sent = sendto(sock, packet_stick_UDP, 3, 0, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
		if (bytes_sent < 0) {
			printf("\"oś\" %d, wychylenie [%d, %d] %d, tryb: UDP, Błąd!\n", packet_stick_UDP[0] - 40, packet_stick_UDP[1], packet_stick_UDP[2],
					value);
		} else {
			printf("\"oś\" %d, wychylenie [%d, %d] %d, tryb: UDP, wysłano %d bajtów\n", packet_stick_UDP[0] - 40, packet_stick_UDP[1],
					packet_stick_UDP[2], value, sizeof(packet_stick_UDP));
		}
	}
}

char packet_stick_TCP[3];
void wyslij_stick_TCP(unsigned int number, short value) {

	packet_stick_TCP[0] = number + 40;
/*
	 packet_stick_TCP[1] = (char) (value & 0xFF);
	 packet_stick_TCP[2] = (char) ((value >> 8) & 0xFF);
*/
	packet_stick_TCP[1] = (char) (value % 256);
	packet_stick_TCP[2] = (char) (value / 256);

	if (write(SocketFD, packet_stick_TCP, 3) < 0) {
		printf("oś %d, wychylenie [%d, %d] %d, tryb: TCP, Błąd!\n", packet_stick_TCP[0] - 40, packet_stick_TCP[1], packet_stick_TCP[2], value);
	} else {
		printf("oś %d, wychylenie [%d, %d] %d, tryb: TCP, wysłano %d bajtów\n", packet_stick_TCP[0] - 40, packet_stick_TCP[1], packet_stick_TCP[2],
				value, sizeof(packet_stick_TCP));
	}
}

unsigned char packet_dwustan_UDP[1];
/* wysyła przez UDP sygnały dotyczące przycisku joysticka lub klawisza */
void wyslij_dwustan_UDP_old(unsigned char number, short value) {
	/* parzyste zwolnienie, nieparzyste wciśnięcie */
	packet_dwustan_UDP[0] = 2 * (number + 1) + value;
	bytes_sent = sendto(sock, packet_dwustan_UDP, 1, 0, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
	/* wysyła na gniazdo sock, pakiet packet_dwustan_UDP, o długości strlen(packet_dwustan_UDP)... */

	if (bytes_sent < 0) {
		printf("Error sending packet (wyslij_dwustan_UDP): %s\n", strerror(errno));
	}
	printf("%2hd△ numer przycisku: %2hd\tstan:%2hd\n", bytes_sent, (packet_dwustan_UDP[0] / 2) - 1, (packet_dwustan_UDP[0] % 2));
}

void wyslij_dwustan_UDP(unsigned char number, short value) {
	/* parzyste zwolnienie, nieparzyste wciśnięcie */
	packet_dwustan_UDP[0] = 2 * (number + 1) + value;
	//TODO: length check
	bytes_sent = sendto(sock, packet_dwustan_UDP, 1, 0, (struct sockaddr*) &sa, sizeof(struct sockaddr_in));
	/* wysyła na gniazdo sock, pakiet packet_dwustan_UDP, o długości strlen(packet_dwustan_UDP)... */

	if (bytes_sent < 0) {
		printf("Error sending packet (wyslij_dwustan_UDP): %s\n", strerror(errno));
	}
	printf("%2hd△ numer przycisku: %2hd\tstan:%2hd\n", bytes_sent, (packet_dwustan_UDP[0] / 2) - 1, (packet_dwustan_UDP[0] % 2));
}

unsigned char packet_dwustan_TCP[1];
/* wysyła przez TCP sygnały dotyczące przycisku joysticka lub klawisza */
void wyslij_dwustan_TCP(unsigned char number, short value) {
	/* parzyste zwolnienie, nieparzyste wciśnięcie */
	packet_dwustan_TCP[0] = 2 * (number + 1) + value;
	if (write(SocketFD, packet_dwustan_TCP, 1) < 0) {
		printf("numer przycisku: %2hd\tstan:%2hd, Błąd!\n", (packet_dwustan_TCP[0] / 2) - 1, (packet_dwustan_TCP[0] % 2));
	} else {
		printf("numer przycisku: %2hd\tstan:%2hd, wysłano %d bajtów\n", (packet_dwustan_TCP[0] / 2) - 1, (packet_dwustan_TCP[0] % 2),
				sizeof(packet_dwustan_TCP[0]));
	}
}

/* w zależności od wpisanego w pliku konfiguracyjnym trybu wysyłania poniższe funkcje wywołują odpowiednie funkcje przekazując im argumenty */
void wyslij_dwustan(unsigned char number, short value) {
	switch (tryb_wysylania) {
	if (number >= 0 && number <= 15 && value >= 0) {
		case 0:
		wyslij_dwustan_UDP(number, value);
		/* wysyła protokołem UDP, samolot */
		break;
		case 1:
		wyslij_dwustan_TCP(number, value);
		/* wysyła protokołem TCP */
		break;
		case 2:
		wyslij_dwustan_UDP_old(number, value);
		/* wysyła protokołem UDP, samochód */
		break;
		default:
		printf("przycisk %d, stan %d, tryb: 3\n", number, value);
		/* domyślna */
		break;
	}
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

