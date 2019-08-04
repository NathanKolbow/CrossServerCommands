#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

const char *FQDN_LIST[] = { "localhost" };
const char *IP_LIST[] = { "127.0.0.1" };
const int PORT = 3491;
const char *PORT_STRING = "3491";
const int BUFFER_SIZE = 2048;

const char *SUCCESS_MESSAGE = "g/asB9R0Dr(QAE!cZ/H8+In>2 4 kHKpql";
const char *WELCOME_MESSAGE = "83e31f401c6ec80dff99b24560e29cf2";
const char *FINAL_MESSAGE = "____-_>><_";

char * getHostname(void);
char * getIP(void);

void package(char *);

int main(int argv, char *args[]) {
	// args[0] will ALWAYS be ./connect or its equal
	char *myFQDN = getHostname();
	char *myIP = getIP();

	char command[BUFFER_SIZE];
	memset(command, 0, sizeof(command));
	int i;
	for(i = 1; i < argv-1; i++) {
		strcat(command, args[i]);
		strcat(command, " ");
	}

	char inputFile[strlen(args[argv-1]) + 1];
	strcpy(inputFile, args[argv-1]);

	char inData[BUFFER_SIZE];
        if(strcmp(inputFile, "NULLIFY") == 0) {
                strcpy(inData, "NULLIFY");
        } else {
                FILE *in_ptr = fopen(inputFile, "r");
                char line[BUFFER_SIZE];
		while(fgets(line, BUFFER_SIZE-1, in_ptr) != NULL)
			strcat(inData, line);
        }

	package(command);
	package(inData);

	if(inputFile[0] != '/' && strcmp(inputFile, "NULLIFY") != 0) {
		printf("Input file path must be absolute.  Please try again.\n");
		exit(1);
	}

	for(i = 0; i < sizeof(FQDN_LIST) / sizeof(char *); i++) {
		if(strcmp(FQDN_LIST[i], myFQDN) == 0 || strcmp(IP_LIST[i], myIP) == 0)
			continue;
		// do the thing
		int status;
		struct addrinfo *servinfo;
		struct addrinfo hints;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;

		if((status = getaddrinfo(FQDN_LIST[i], PORT_STRING, &hints, &servinfo)) != 0) {
        	        printf("getaddrinfo error: %s\n", gai_strerror(status));
        	        exit(1);
	        }

		// servinfo now points to an array of 1 or more struct addrinfo's
		struct addrinfo *p;
		struct addrinfo *res;
		char ipstr[INET6_ADDRSTRLEN];

		for(p = servinfo; p != NULL; p = p->ai_next) {
			void *addr;
			if(p->ai_family == AF_INET) {
				struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
				addr = &(ipv4->sin_addr);
			} else {
				printf("IPv6 connections not supported.\n");
				exit(1);
			}

			inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
			if(strcmp(ipstr, IP_LIST[i]) == 0) {
				res = p;
				break;
			}
		}

		// res is now the desired server
		int serverSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if(serverSocket == -1) {
			printf("Error obtaining server socket.\n");
			printf("error: %s\n", strerror(errno));
			continue;
		}

		connect(serverSocket, res->ai_addr, res->ai_addrlen);

		/* PROTOCOL FOR PROPER CONNECTIONS
		 *	See listen.c
		 */

		// status 0 - try again
		// status 1 - successful
		// status -1 - connection dropped

		status = 1;
		while(status == 1) {
			status = send(serverSocket, command, strlen(command), 0);
			if(status == 0) { printf("exit1\n"); exit(1); }

			usleep(10000);

			status = send(serverSocket, inData, strlen(inData), 0);
			if(status == 0) { printf("exit2\n"); exit(0); }
		}
		printf("Data sent to %s (%s).\n", FQDN_LIST[i], ipstr);
	} // end of for loop
}


void package(char msg[]) {
	if(strcmp(msg, "NULLIFY") == 0) {
		strcpy(msg, "2$");
		return;
	}

	int i = strlen(msg) + 1; // +1 for the $
	char c[10];
	memset(c, 0, sizeof(c));
	sprintf(c, "%d", i);

	char over[BUFFER_SIZE];
	memset(over, 0, sizeof(over));
	sprintf(over, "%lu$", i+strlen(c));
	strcat(over, msg);

	memset(msg, 0, (strlen(over) + 1) * sizeof(char));
	strcpy(msg, over);
}

char * getIP() {
	FILE *commandp;
	char myIP[100];

	commandp = popen("/bin/hostname -I", "r");
	if(commandp == NULL) {
		printf("Failed to run command /bin/hostname -A.\n");
		printf("error: %s\n", strerror(errno));
		exit(1);
	}

	// read output
	while(fgets(myIP, sizeof(myIP)-1, commandp) != NULL) { }

	pclose(commandp);

	int i;
	for(i = 0; i < 100; i++) {
		if(myIP[i] == ' ') { // the first space comes immediately before a newline, we want to remove both
			myIP[i] = '\0';
			break;
		}
	}

	char *ret = malloc(sizeof(char) * (strlen(myIP) + 1));
	strcpy(ret, myIP);

	return ret;
}

char * getHostname() {
	FILE *commandp;
	char myFQDN[100];

	commandp = popen("/bin/hostname -A", "r");
	if(commandp == NULL) {
		printf("Failed to run command /bin/hostname -A.\n");
		printf("error: %s\n", strerror(errno));
		exit(1);
	}

	// read output
	while(fgets(myFQDN, sizeof(myFQDN)-1, commandp) != NULL) { }

	pclose(commandp);

	int i;
	for(i = 0; i < 100; i++) {
		if(myFQDN[i] == ' ') { // the first space comes immediately before a newline, we want to remove both
			myFQDN[i] = '\0';
			break;
		}
	}

	char *ret = malloc(sizeof(char) * (strlen(myFQDN) + 1));
	strcpy(ret, myFQDN);

	return ret;
}

/*int main() {
	int status;
	struct addrinfo hints;
	struct addrinfo *servinfo;

	char ipstr[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if((status = getaddrinfo(HOST_FQDN, PORT_STRING, &hints, &servinfo)) != 0) {
		printf("getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	// servinfo now points to an array of 1 or more struct addrinfo's
	struct addrinfo *p;
	struct addrinfo *res;
	for(p = servinfo; p != NULL; p = p->ai_next) {
		void *addr;

		if(p->ai_family == AF_INET) {
			struct sockaddr_in *ipv4 = (struct sockaddr_in *) p->ai_addr;
			addr = &(ipv4->sin_addr);
		} else {
			struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *) p->ai_addr;
			addr = &(ipv6->sin6_addr);
		}

		inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		if(strcmp(ipstr, HOST_IP) == 0) {
			res = p;
			break;
		}
	}

	// socket used to connect to HOST_FQDN with
	int serverSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	freeaddrinfo(servinfo);
}
*/
