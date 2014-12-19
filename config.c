#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* potrzebne do sprawdzania lokalnego adresu IP */
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

char joystick_descriptor[20];
char interfejs_sieciowy[16];

int tryb_wysylania;
/*
 1 UDP IP
 2 (nie użyty)
 3 (nie użyty)
 */

/* adresy IP zapisane jako 32-bitowe liczby */
int odbiornik_IP;
int nadajnik_IP;
/* int	nadajnik_INET6; (nie użyty) */

/* numer kamery */
short int v4l_device_number;

/* gboolean */
short int isFullscreen;

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

		{ // check it is IP4
		  // is a valid IP4 Address
			tmpAddrPtr = &((struct sockaddr_in *) ifa->ifa_addr)->sin_addr;
			char addressBuffer[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
			/* printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer); */
			if (strcmp(ifa->ifa_name, interfejs_sieciowy) == 0) {
				printf("znaleziono");
				printf(" nadajnik_IP= 		%s", addressBuffer);
				nadajnik_IP = atoi(strtok(addressBuffer, ".:-")) * 16777216;
				nadajnik_IP = nadajnik_IP + atoi(strtok(NULL, ".:-")) * 65536;
				nadajnik_IP = nadajnik_IP + atoi(strtok(NULL, ".:-")) * 256;
				nadajnik_IP = nadajnik_IP + atoi(strtok(NULL, ".:-"));
				/* nadajnik_IP to adres IP zapisany jako 32-bitowa liczba */
				printf(" (%X hex)\n", nadajnik_IP);
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
		printf(" (%X hex)\n", odbiornik_IP);
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
			isFullscreen = 1;
		}
		if (strcmp(wartosc, "0") == 0) {
			isFullscreen = 0;
		}
		printf("\n");
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
void wczytaj_config() {
	FILE *filepointer;
	/* względna ścieżka do pliku konfiguracyjnego */
	filepointer = fopen("./config.cfg", "r");

	if (filepointer == NULL) {
		puts(
				"Nie można otworzyć pliku konfiguracyjnego!\n  Przypisano wartości domyślne.");

		/* tutaj przypisać wartości domyślne */
		strcpy(joystick_descriptor, "/dev/input/js0");
		strcpy(interfejs_sieciowy, "lo");
		/* czyli domyślnie loopback */
		nadajnik_IP = 0x7F000001; /* jeśli plik nie jest otwierany przypisuje wartość domyślną 127.0.0.1*/
		odbiornik_IP = 0x7F000001;
		v4l_device_number = 0;
		tryb_wysylania = 1;
		isFullscreen = 0;
	}

	else {
		printf("Otwarcie pliku konfiguracyjnego\n");

		char * wiersz = NULL;
		size_t rozmiar = 0;
		ssize_t cos;

		while ((cos = getline(&wiersz, &rozmiar, filepointer)) != -1) {
			podziel(wiersz);
		}

		/*
		 Function: ssize_t getline (char **lineptr, size_t *n, FILE *stream)

		 This function reads an entire line from stream, storing the text
		 (including the newline and a terminating null character)
		 in a buffer and storing the buffer address in *lineptr.
		 Before calling getline, you should place in *lineptr the address
		 of a buffer *n bytes long, allocated with malloc.
		 If this buffer is long enough to hold the line, getline stores
		 the line in this buffer. Otherwise, getline makes the buffer bigger
		 using realloc, storing the new buffer address back in *lineptr
		 and the increased size back in *n. See Unconstrained Allocation.
		 If you set *lineptr to a null pointer, and *n to zero,
		 before the call, then getline allocates the initial buffer for you
		 by calling malloc. In either case, when getline returns,
		 *lineptr is a char * which points to the text of the line.
		 When getline is successful, it returns the number of
		 characters read (including the newline, but not including
		 the terminating null). This value enables you to distinguish null
		 characters that are part of the line from the null character
		 inserted as a terminator. This function is a GNU extension, but
		 it is the recommended way to read lines from a stream.
		 The alternative standard functions are unreliable. If an error
		 occurs or end of file is reached without any bytes read,
		 getline returns -1.
		 */
		fclose(filepointer);
		printf("Zamknięcie pliku konfiguracyjnego\n");
	}
}
