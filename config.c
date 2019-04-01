#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <string.h>

/* for checking local IP address */
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

char joystick_descriptor[32];
char interfejs_sieciowy[16];

int tryb_wysylania;
/*
 0 UDP IP
 1 TCP IP
 2 UDP old
 */

/* adresy IP zapisane jako 32-bitowe liczby */
int nadajnik_IP;
int odbiornik_IP;
int odbiornik_port;
/* int	nadajnik_INET6; (nie użyty) */

/* numer kamery */
short int v4l_device_number;

gboolean isFullscreen;
gboolean r2_throttle;

/* DualShock 4 has 12 (8 since Ubuntu 18.04) axes while connected by USB, 14 axes while connected by ds4drv Bluetooth driver */
int axis[14] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
int button[13] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
int deadzone_x1;
int deadzone_y1;

/*
 int print_interface_IP()
 {
 struct ifaddrs * ifAddrStruct=NULL;
 struct ifaddrs * ifa=NULL;
 void * tmpAddrPtr=NULL;

 getifaddrs(&ifAddrStruct);

 for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next)
 {
 if (ifa ->ifa_addr->sa_family==AF_INET)
 { // check it is IP4
 // is a valid IP4 Address
 tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
 char addressBuffer[INET_ADDRSTRLEN];
 inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
 printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
 }
 else if (ifa->ifa_addr->sa_family==AF_INET6)
 { // check it is IP6
 // is a valid IP6 Address
 tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
 char addressBuffer[INET6_ADDRSTRLEN];
 inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
 printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
 }
 }
 if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
 return 0;
 }
 */

int check_interface_ip() {
	struct ifaddrs * ifAddrStruct = NULL;
	struct ifaddrs * ifa = NULL;
	void * tmpAddrPtr = NULL;

	getifaddrs(&ifAddrStruct);

	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family == AF_INET)

		{
			/* http://man7.org/linux/man-pages/man3/getifaddrs.3.html */
			tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			if (strcmp(ifa->ifa_name, interfejs_sieciowy) == 0) {
				printf("znaleziono");
				printf(" nadajnik_IP= 		%s", addressBuffer);
				nadajnik_IP = atoi(strtok(addressBuffer, ".:-")) * 16777216;
				nadajnik_IP = nadajnik_IP + atoi(strtok(NULL, ".:-")) * 65536;
				nadajnik_IP = nadajnik_IP + atoi(strtok(NULL, ".:-")) * 256;
				nadajnik_IP = nadajnik_IP + atoi(strtok(NULL, ".:-"));
				/* nadajnik_IP to adres IP zapisany jako 32-bitowa liczba */
				printf(" (hex: %X)\n", nadajnik_IP);
			}

		}
		/* for IPV6 */
		/*
		 else if (ifa->ifa_addr->sa_family==AF_INET6)
		 {
		 tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
		 char addressBuffer[INET6_ADDRSTRLEN];
		 inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
		 printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
		 }
		 */
	}
	if (ifAddrStruct != NULL)
		freeifaddrs(ifAddrStruct);
	return 0;
}

void przytnij(char* wartosc) {
	if (wartosc[0] == ' ' || wartosc[0] == '\t') {
		while (wartosc[0] == ' ' || wartosc[0] == '\t') {
			int i = 0;
			while (wartosc[i] != '\n') {
				wartosc[i] = wartosc[i + 1];
				i++;
			}
			wartosc[i - 1] = '\0';
		}
	}

	else {
		int i = 0;
		while (wartosc[i] != '\n') {
			i++;
		}
		wartosc[i] = '\0';
		/* usuwa znak końca wiersza */
	}
}

void przypisz(char* zmienna, char* wartosc) {
	if (strcmp(zmienna, "joystick_descriptor") == 0)
	/* szuka w pliku zmiennej o nazwie joystick_descriptor */
	{
		printf("przypisano");
		strcpy(joystick_descriptor, wartosc);
		/* przypisuje zmiennej wartość pobraną z pliku */
		printf(" %s= 	%s\n", zmienna, joystick_descriptor);
	}

	if (strcmp(zmienna, "interfejs_sieciowy") == 0)
	/* szuka w pliku zmiennej o nazwie interfejs_sieciowy */
	{
		printf("przypisano");
		strcpy(interfejs_sieciowy, wartosc);
		/* przypisuje zmiennej wartość pobraną z pliku */
		printf(" %s= 		%s\n", zmienna, interfejs_sieciowy);
		check_interface_ip();
		/* znajduje adres IP interfejsu o nazwie pobranej z pliku */
	}

	if (strcmp(zmienna, "odbiornik_IP") == 0) {
		printf("przypisano");
		printf(" %s= 		%s", zmienna, wartosc);
		/* wartosc to adres IP zapisany jako ciąg znaków */
		odbiornik_IP = atoi(strtok(wartosc, ".:-")) * 16777216;
		odbiornik_IP = odbiornik_IP + atoi(strtok(NULL, ".:-")) * 65536;
		odbiornik_IP = odbiornik_IP + atoi(strtok(NULL, ".:-")) * 256;
		odbiornik_IP = odbiornik_IP + atoi(strtok(NULL, ".:-"));
		/* odbiornik_IP to adres IP zapisany jako 32-bitowa liczba */
		printf(" (hex: %X)\n", odbiornik_IP);
	}

	if (strcmp(zmienna, "odbiornik_port") == 0) {
		printf("przypisano");
		odbiornik_port = atoi(wartosc);
		printf(" %s=		%hd\n", zmienna, odbiornik_port);
	}

	if (strcmp(zmienna, "kamera") == 0) {
		printf("przypisano");
		v4l_device_number = atoi(wartosc);
		printf(" %s=			%hd\n", zmienna, v4l_device_number);
	}

	if (strcmp(zmienna, "tryb_wysylania") == 0) {
		printf("przypisano");
		tryb_wysylania = atoi(wartosc);
		printf(" %s=		%hd\n", zmienna, tryb_wysylania);
	}

	/*
	 if (strcmp(zmienna,"nadajnik_IP")==0)
	 { printf ("przypisano");
	 printf (" %s= 		%s", zmienna, wartosc);
	 nadajnik_IP= atoi(strtok (wartosc,".:-")) * 16777216;
	 nadajnik_IP= nadajnik_IP + atoi(strtok (NULL,".:-")) *65536;
	 nadajnik_IP= nadajnik_IP + atoi(strtok (NULL,".:-")) *256;
	 nadajnik_IP= nadajnik_IP + atoi(strtok (NULL,".:-"));
	 printf (" (%X hex)\n", nadajnik_IP);
	 }
	 */

	if (strcmp(zmienna, "pelny_ekran") == 0) {
		printf("przypisano");
		printf(" %s= 		%s", zmienna, wartosc);
		/* zero lub jedynka wczytana z pliku tekstowego jest ciągiem znaków */
		if (strcmp(wartosc, "1") == 0) {
			isFullscreen = TRUE;
		}
		if (strcmp(wartosc, "0") == 0) {
			isFullscreen = FALSE;
		}
		printf("\n");
	}

	if (strcmp(zmienna, "r2_throttle") == 0) {
		printf("przypisano");
		printf(" %s= 		%s", zmienna, wartosc);
		if (strcmp(wartosc, "1") == 0) {
			r2_throttle = TRUE;
		}
		if (strcmp(wartosc, "0") == 0) {
			r2_throttle = FALSE;
		}
		printf("\n");
	}

	if (strcmp(zmienna, "deadzone_x1") == 0) {
		printf("przypisano");
		deadzone_x1 = atoi(wartosc);
		printf(" %s=			%hd\n", zmienna, deadzone_x1);
	}

	if (strcmp(zmienna, "deadzone_y1") == 0) {
		printf("przypisano");
		deadzone_y1 = atoi(wartosc);
		printf(" %s=			%hd\n", zmienna, deadzone_y1);
	}

	/* remapping gamepad axles */

	if (strcmp(zmienna, "axis_yaw") == 0) {
		printf("przypisano");
		axis[0] = atoi(wartosc);
		printf(" %s=			%hd\n", zmienna, axis[0]);
	}

	if (strcmp(zmienna, "axis_pitch") == 0) {
		printf("przypisano");
		axis[1] = atoi(wartosc);
		printf(" %s=			%hd\n", zmienna, axis[1]);
	}

	if (strcmp(zmienna, "axis_roll") == 0) {
		printf("przypisano");
		axis[3] = atoi(wartosc);
		printf(" %s=			%hd\n", zmienna, axis[3]);
	}

	/* 3 - L2, 4 - R2 */
	/* 2 - L2, 5 - R2 since Ubuntu 18.04 */

	if (strcmp(zmienna, "axis_throttle") == 0) {
		printf("przypisano");
		axis[4] = atoi(wartosc);
		printf(" %s=		%hd\n", zmienna, axis[4]);
	}
	/* tutaj przypisywanie kolejnych wartośći zmiennym */
}

void podziel(char* wiersz) {
	int i = 0;
	/* było boolean */
	short int dzielto = 0;
	while (wiersz[i] != '\n') {
		if (wiersz[i] == '=') {
			dzielto = 1;
		}
		i++;
		/* jeśli w wierszu występuje znak równości */
	}

	if (wiersz[0] == '#') {
		dzielto = 0;
		/* jeśli wiersz NIE zaczyna się od hasza */
	}

	if (dzielto == 1) {
		char* zmienna = strtok(wiersz, "=");
		char* wartosc = strtok(NULL, "=");
		przytnij(wartosc);
		przypisz(zmienna, wartosc);
	}

	else {
		/* printf ("%s", wiersz); */
		/* drukowanie w całości wierszy, w których nie znaleziono zmiennych do przypisania im wartości */
	}
}

/* otwiera plik konfiguracyjny i przypisuje zmiennym pobrane z niego wartości */
void open_config(char argument[]) {
	/* relative path to default configuration file */
	char filename[32];
	FILE *filepointer;
	if (argument == NULL) {
		strcpy(filename, "./config.cfg");
	} else {
		strcpy(filename, argument);
	}
	filepointer = fopen(filename, "r");

	if (filepointer == NULL) {
		printf("Nie można otworzyć pliku konfiguracyjnego %s!\n  Przypisano wartości domyślne.\n", filename);

		/* tutaj przypisać wartości domyślne */
		strcpy(joystick_descriptor, "/dev/input/js0");
		strcpy(interfejs_sieciowy, "lo");
		/* czyli domyślnie loopback */
		nadajnik_IP = 0x7F000001; /* default value is 127.0.0.1*/
		odbiornik_IP = 0x7F000001;
		odbiornik_port = 7654;
		v4l_device_number = 0;
		tryb_wysylania = 0;
		isFullscreen = FALSE;
		r2_throttle = TRUE; /* right analog button works as throttle */
	}

	else {
		printf("Otwarcie pliku konfiguracyjnego %s\n", filename);

		char * wiersz = NULL;
		size_t rozmiar = 0;
		ssize_t line;

		while ((line = getline(&wiersz, &rozmiar, filepointer)) != -1) {
			podziel(wiersz);
		}

		/*
		 for (int i = 0; i < 12; i++)
		 {
		 printf("oś %hd=					%hd\n", i, axis[i]);
		 }
		 */
		fclose(filepointer);
		printf("Zamknięcie pliku konfiguracyjnego\n");
	}
}
