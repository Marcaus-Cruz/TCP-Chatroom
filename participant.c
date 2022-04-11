/* participant.c - code for participant. Do not rename this file */

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
	char buf[1000]; /* buffer for data from the server */

	memset((char *) &sad, 0, sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */

	char msg[1002];
	int check;
	uint8_t userLen;
	uint16_t msgLen;
	uint16_t conversion;
	time_t startTime, endTime;

	fd_set readfds, wrk_readfds;
	struct timeval tv;

	if (argc != 3) {
		fprintf(stderr, "Error: Wrong number of arguments\n");
		fprintf(stderr, "usage:\n");
        	fprintf(stderr, "./participant  server_address  server_port\n");
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
		fprintf(stderr, "Receive failed\n");
		close(sd);
		exit(EXIT_FAILURE);
	}

    	if(buf[0] == 'N'){
        	//wrong port or chatroom full
        	printf("Server is full.\n");
        	close(sd);
        	exit(EXIT_SUCCESS);
    	} else if (buf[0] == 'Y'){
		printf("Choose a username: ");
		startTime = time(0);

		// username loop
		while(1){

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

			if (endTime < startTime + 60) {
				if (strlen(msg) == 1){
					printf("Choose a username: ");
					continue;
				}

				userLen = strlen(msg) - 1;

				check = 0;
				for(int i = 0; i < userLen; i++){
					//check digits
					if(msg[i] > 48 && msg[i] < 57){
						check = 1;
					}
					//check underscore
					else if(msg[i] == '_'){
						check = 1;
					}
					//check uppercase
					else if(msg[i] >64 && msg[i] < 90){
						check = 1;
					}
					//check lowercase
					else if(msg[i] > 96 && msg[i] < 123){
						check = 1;
					}
					else{
						check = 0;
						break;
					}
				}

				if (check == 1 && userLen > 0 && userLen < 11){
					send(sd, &userLen, sizeof(uint8_t), 0);
					send(sd, &msg, userLen, 0);

					n = recv(sd, buf, sizeof(char), 0);
					if (n < 1){
						printf("Time to choose a username has expired.\n");
						close(sd);
						exit(EXIT_SUCCESS);
					}

					if (buf[0] == 'Y'){
						printf("Username accepted.");
						break;
					}
					else if (buf[0] == 'I'){
						printf("Username is invalid. Choose a valid username: ");
					}

					else if (buf[0] == 'T'){
						printf("Username is already taken. Choose a different username: ");
						startTime = time(0);
					}
				}else {
					printf("Choose a username (up to 10 characters long; allowed characters are alphabets, digits, and underscores): ");
				}
			}
			else {
				printf("Time to choose a username has expired.\n");
				close(sd);
				exit(EXIT_SUCCESS);
			}
		}

		// messaging loop

		printf("\nEnter message: ");
		while(1){
			msg[999] = '\0';
			/*checkPriv = 0;*/
			while (fgets(msg, 1001, stdin) != NULL && msg[0] != '\n'){

				// select call to see if server has shutdown
				FD_ZERO(&wrk_readfds);
				FD_SET(sd, &wrk_readfds);

				n = select((sd + 1), &wrk_readfds, NULL, NULL, &tv);
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


				if (strlen(msg) > 998 && (msg[999] != '\0' || msg[999] != '\n')){
					msgLen = strlen(msg);
					//checkPriv = 1;
				}
				else
					msgLen = strlen(msg) - 1;

				if (msgLen > 0){

					check = 0;
					for (int i=0; i < msgLen; i++){
						if (msg[i] != ' ' && msg[i] != '\n')
							check = 1;
					}

					if (check == 1){

						// this code is for prepending @$username on private messages
						// not completed, so we left it commented

						/*if (checkPriv == 1){
							for (int i = msgLen; i > -1; i--){
								msg[i+1] = msg[i];
							}
							msg[0] = '@';
						}*/

						conversion = htons(msgLen);
						send(sd, &conversion, sizeof(uint16_t), 0);
						send(sd, &msg, msgLen, 0);
					}
				}
				if (msg[msgLen] == '\n')
					break;
			}
			printf("Enter message: ");

		}

	}

	close(sd);
	exit(EXIT_SUCCESS);
}
