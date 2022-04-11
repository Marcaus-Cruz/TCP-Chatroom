/* observer.c - code for observer. Do not rename this file */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>

int main( int argc, char **argv) {

	struct hostent *ptrh; /* pointer to a host table entry */
	struct protoent *ptrp; /* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold an IP address */
	int sd; /* socket descriptor */
	int port; /* protocol port number */
	char *host; /* pointer to host name */
	int n; /* number of characters read */
	char buf[1014]; /* buffer for data from the server */

	memset((char *) &sad, 0, sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	char msg[1015];
	int check = 0;
	time_t startTime, endTime;
	uint8_t userLen;
	uint16_t msgLen;

	fd_set readfds, wrk_readfds;
	struct timeval tv;

	if (argc != 3) {
		fprintf(stderr, "Error: Wrong number of arguments\n");
		fprintf(stderr, "usage:\n");
		fprintf(stderr, "./observer  server_address  server_port\n");
		exit(EXIT_FAILURE);
	}

	port = atoi(argv[2]); /* convert to binary */
	if (port > 0) /* test for legal value */
		sad.sin_port = htons((u_short) port);
	else {
		fprintf(stderr, "Error: bad port number %s\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; /* if host argument specified */

	/* Convert host name to equivalent IP address and copy to sad. */
	ptrh = gethostbyname(host);
	if (ptrh == NULL) {
		fprintf(stderr, "Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if (((long int) (ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *) &sad, sizeof(sad)) < 0) {
		fprintf(stderr, "connect failed\n");
		exit(EXIT_FAILURE);
	}

	FD_ZERO(&readfds);
	FD_SET(sd, &readfds);
	tv.tv_sec = 0;
	tv.tv_usec = 50;

	//get Y or N
	n = recv(sd, buf, sizeof(char), 0);
	if (n == 0){
		printf("Receive failed\n");
		close(sd);
		exit(EXIT_FAILURE);
	}

	if(buf[0] == 'N'){
		//chatroom full
		printf("Server is full.\n");
		close(sd);
		exit(EXIT_FAILURE);
	} else{
		//do observer stuff
		printf("Enter a participant's username: ");
		while(check == 0){

			startTime = time(0);
			fgets(msg, 1000, stdin);
			endTime = time(0);

			// check server hasn't sent us shutdown
			FD_ZERO(&wrk_readfds);
			FD_SET(sd, &wrk_readfds);

			n = select((sd+1), &wrk_readfds, NULL, NULL, &tv);
			if (n < 0){
				fprintf(stderr, "Error: select() failed. %s\n", strerror(errno));
				close(sd);
				exit(EXIT_FAILURE);
			}

			if (FD_ISSET(sd, &wrk_readfds)){
				printf("Server shutdown\n");
				close(sd);
				exit(EXIT_SUCCESS);
			}

			// 60 second internal timeout
			if (endTime < startTime + 60) {
				if (strlen(msg) == 1){
					printf("Enter a participant's username: ");
					continue;
				}

				userLen = strlen(msg) - 1;

				if(userLen > 0 && userLen < 11){
					send(sd, &userLen, sizeof(uint8_t), 0);
					send(sd, &msg, userLen, 0);

					n = recv(sd, buf, sizeof(char), 0);
					if (n < 1){
						printf("Time to enter a username has expired.\n");
						close(sd);
						exit(EXIT_SUCCESS);
					}

					if(buf[0] == 'N'){
						printf("There is no active participant with that username.\n");
						close(sd);
						exit(EXIT_SUCCESS);
					} else if(buf[0] == 'T'){
						printf("That participant already has an observer. Enter a different participant's username: ");
					} else if(buf[0] == 'Y'){
						check = 1;
					}
				}
			}
			else{
				printf("Time to enter a username has expired.\n");
				close(sd);
				exit(EXIT_SUCCESS);
			}
		}

		while(1){
			//recieve and print msgs
			// buf buffer should be 1014, cause we don't send the null terminator
			// msg buffer should be 1015, 14 prepended chars, 1000 message length, 1 null char at the end

			n = recv(sd, (void *) &msgLen, sizeof(uint16_t),0);
			if (n == 0){
				break;
			}
			else if (n == -1){
				fprintf(stderr, "Receive failed\n");
				close(sd);
				exit(EXIT_FAILURE);
			}

			msgLen = ntohs(msgLen);

			recv(sd, buf, msgLen, 0);

			for (int i=0; i < msgLen; i++){
				msg[i] = buf[i];
			}
			msg[msgLen] = '\0';

			printf("%s\n", msg);
		}
	}

	close(sd);
	exit(EXIT_SUCCESS);
}
