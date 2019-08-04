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
#include <time.h>
#include <signal.h>

const char *VALID_FQDNS[] = { "localhost" };
const char *VALID_IPS[] = { "127.0.0.1" };
const int PORT = 3491;
const char *PORT_STRING = "3491";

const char *SUCCESS_MESSAGE = "g/asB9R0Dr(QAE!cZ/H8+In>2 4 kHKpql";
const char *WELCOME_MESSAGE = "83e31f401c6ec80dff99b24560e29cf2";
const char *FINAL_MESSAGE = "____-_>><_";

const int BUFFER_SIZE = 2048;

void unpackage(char []);
int bindAndListen(void);
int contains(const char *[], int, char*);
int verifyData(char[]);
char * timestamp(void);

// SETUP: All servers will constantly be listening on port 3491.  When one server receives a command that should be
//	  executed across servers, that server will connect to each other server and tell them to execute the command.
//
// THIS PROGRAM: Only ever LISTENS for connections and talks once THEY have connected to ME
// ANOTHER PROGRAM: Handles connecting to other servers and is automatically invoked when any relevant command is run

int main() {
//	struct timespec sleepTime;
//	sleepTime.tv_sec = 0;
//	sleepTime.tv_nsec = 100;

printf("Binding socket and listening on port %d.\n", PORT);
	int mySocket = bindAndListen();
printf("Bound socket and beginning to listen.\n");
	// accept, then send() and recv() as appropriate
	struct sockaddr_storage their_addr;
	socklen_t addr_size = sizeof(their_addr);

	while(1) {
		int connection_fd = accept(mySocket, (struct sockaddr *)&their_addr, &addr_size);
printf("conn_fd: %d\n", connection_fd);

		// once the connection is successfully accepted, fork into another process that handles the conversation with the client
		// this process will continue to listen for new connections

		char connectedIP[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(((struct sockaddr_in *)&their_addr)->sin_addr), connectedIP, INET_ADDRSTRLEN);
		printf("Host connected from: %s\n", connectedIP);
		if(contains(VALID_IPS, sizeof(VALID_IPS) / sizeof(char *), connectedIP) == 0) { // invalid IP tried connecting
			printf("Closing this invalid connection.\n");
			close(connection_fd);
			continue;
		}
		printf("Connection accepted.\n\n");

		pid_t pid = fork();
		if(pid == 0) { // child
printf("Entering child process run state.\n");
			/* NEW PROTOCOL FOR PROPER CONNECTIONS:
			 * 1. Client send all of the data in two separate messages: command to be run, then input data.
			 *    Format: 123$ followed by the relevant data
			 *            123 is replaced w/ the string length of the message sent INCLUDING THE 123
			 * 2. The client keeps sending this data until the connection is closed, the listener closes the connection when
			 *    the string it receives agrees with the length indicator.
			 */
pid_t p = getpid();
printf("Child pid: %d\n", p);
			int status = 0;
                        char buffer[BUFFER_SIZE];
			char command[BUFFER_SIZE];
			char inputData[BUFFER_SIZE];
			int i = 0;
			while(status == 0 && i < 10) {
				memset(buffer, 0, sizeof(buffer));

	                        status = recv(connection_fd, buffer, BUFFER_SIZE, 0);

				memset(command, 0, sizeof(command));
				strcpy(command, buffer);
				memset(buffer, 0, sizeof(buffer));

				status = recv(connection_fd, buffer, BUFFER_SIZE, 0);

				memset(inputData, 0, sizeof(inputData));
				strcpy(inputData, buffer);

				if(verifyData(command) == 1 && verifyData(inputData) == 1) {
					status = 1;
				}
				i++;
			}

			printf("Data received.\n");

			unpackage(command);
			unpackage(inputData);

			FILE *tmp_ptr = tmpfile();
			fprintf(tmp_ptr, "%s", inputData);

			rewind(tmp_ptr);

			char longCommand[BUFFER_SIZE * 2];
			memset(longCommand, 0, sizeof(longCommand));
			sprintf(longCommand, "file=`mktemp` ; echo \"%s\" > $file ; %s < $file ; rm $file ;", inputData, command);
printf("Executing command sequence: %s\n", longCommand);
			system(longCommand);

			_Exit(2);

			printf("Closing child handler.\n");
		} else { // parent
			sleep(5); // sleep for 5 seconds then kill the (now dead) child process
			kill(pid, SIGKILL);
			continue; // go back to listening for more tcp connections
		}
	}
}

void unpackage(char msg[]) {
	int i = 0;
	for(; msg[i] != '$'; i++) {}
	char temp[(strlen(msg) - i) + 1];
	memcpy(temp, &msg[i+1], strlen(msg) - i);
	temp[strlen(msg) - i] = '\0';
	strcpy(msg, temp);
}

int contains(const char *list[], int size, char *item) {
	int i;
	for(i = 0; i < size; i++) {
		if(strcmp(item, list[i]) == 0)
			return 1;
	}
	return 0;
}

// returns 0 if data is bad, 1 if data is good
int verifyData(char data[]) {
	if(strcmp(data, "$2") == 0) return 1;

	int i = 0;
	while(data[i] != '$' && i < 100) i++;
	if(i > 99) return 0;
	char len[i+1];
	memcpy(len, data, i);
	len[i] = '\0';
	if(atoi(len) == strlen(data)) return 1;
	return 0;
}

// returns the socket file descriptor that the machine is listening on
int bindAndListen() {
	int status;
	struct addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo *myInfo;
	getaddrinfo(NULL, PORT_STRING, &hints, &myInfo);
	int mySocket = socket(myInfo->ai_family, myInfo->ai_socktype, myInfo->ai_protocol);

	if(mySocket == -1) {
		printf("Error obtaining own socket on port %s.\n", PORT_STRING);
		printf("error: %s\n", strerror(errno));
		exit(1);
	}

	// bind the socket locally
	int yes=1;
	// get rid of the "Address already in use" error
	if(setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) == -1) {
		printf("Error trying to reuse socket on port %s.\n", PORT_STRING);
		printf("error: %s\n", strerror(errno));
		exit(1);
	}

	if(bind(mySocket, myInfo->ai_addr, myInfo->ai_addrlen) == -1) {
		printf("Error binding socket locally.\n");
		printf("error: %s\n", strerror(errno));
		exit(1);
	}

	// listen
	status = listen(mySocket, 10);
	if(status == -1) {
		printf("Error listening on port %s.\n", PORT_STRING);
		printf("error: %s\n", strerror(errno));
		exit(1);
	}

	return mySocket;
}

char *timestamp() {
	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char * time = malloc(strlen(asctime(timeinfo)) + 3);
	memset(time, 0, sizeof(char) * strlen(time));
	strcat(time, "[");
	strcat(time, asctime(timeinfo));
	time[strlen(asctime(timeinfo))] = ']';

	return time;
}
