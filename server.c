/* server.c - code for server. Do not rename this file */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#define QLEN 6

int main( int argc, char **argv) {
	struct protoent* ptrp; 	/* pointer to a protocol table entry */
	struct sockaddr_in sad; /* structure to hold server's address */
	struct sockaddr_in sad2;
	struct sockaddr_in cad; /* structure to hold client's address */
	int sd, sd2, sd3, p;		/* socket descriptors */

// Specification says port numbers should be a uint16_t
	int port1; 		/* protocol port number for participants*/
	int port2;
	int alen; 		/* length of address */
	int optval = 1; 	/* boolean value when we set socket option */

	char buf[1000]; 	/* buffer for string the server sends */

	int max_sd;
	fd_set readfds, writefds;
	fd_set wrk_readfds, wrk_writefds;
	struct timeval tv;
	//struct timeval timeout, noTimeout;

	char yes = 'Y';
	char no = 'N';
	char taken = 'T';
	char invalid = 'I';
	char newObs[] = "A new observer has joined";

	int n, check, whitespace, k;
	int partCount = 0;
	int obsCount = 0;
	uint8_t userLen;
	uint16_t msgLen;
	char msg[1000];
	char stringBuild[1015];
	char participants[510][10];
	char observers[510][10];
	char temp[10];

	time_t endTime;
	time_t partTimes[510];
	time_t obsTimes[510];

	//port for participants, port for observers
	if (argc != 3) {
		fprintf(stderr, "Error: Wrong number of arguments\n");
		fprintf(stderr, "usage:\n");
		fprintf(stderr, "./server  participant_port  observer_port \n");
		exit(EXIT_FAILURE);
	}

	memset((char*)&sad, 0, sizeof(sad)); /* clear sockaddr structure */
	sad.sin_family = AF_INET; /* set family to Internet */
	sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */

	memset((char*)&sad2, 0, sizeof(sad2));
	sad2.sin_family = AF_INET;
	sad2.sin_addr.s_addr = INADDR_ANY;

	//Get port # for participants
	port1 = atoi(argv[1]);
	printf("Participant Port: %d \n", port1);

	//Get port # for observers
	port2 = atoi(argv[2]);
	printf("Observer Port: %d \n", port2);

	/* tests for illegal port value */
	if (port1 > 0) {
		sad.sin_port = htons((u_short)port1);
	}
	else { /* print error message and exit */
		fprintf(stderr, "Error: Bad port number %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	/* tests for illegal port value */
	if (port2 > 0) {
		sad2.sin_port = htons((u_short)port2);
	}
	else { /* print error message and exit */
		fprintf(stderr, "Error: Bad port number %s\n", argv[2]);
		exit(EXIT_FAILURE);
	}

	/* Map TCP transport protocol name to protocol number */
	if (((long int)(ptrp = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket */
	sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}
	sd2 = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if(sd2 < 0){
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Allow reuse of port - avoid "Bind failed" issues */
	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}
	if(setsockopt(sd2, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0){
		fprintf(stderr, "Error Setting socket option failed\n");
		exit(EXIT_FAILURE);
	}

	/* Bind a local address to the socket */
	if (bind(sd, (struct sockaddr*) & sad, sizeof(sad)) < 0) {
		fprintf(stderr, "Error: Bind failed\n");
		exit(EXIT_FAILURE);
	}
	if(bind(sd2, (struct sockaddr*) &sad2, sizeof(sad2)) < 0){
		fprintf(stderr, "Error: Bind failed sd2\n");
		exit(EXIT_FAILURE);
	}

	/* Specify size of request queue */
	if (listen(sd, QLEN) < 0) {
		fprintf(stderr, "Error: Listen failed\n");
		exit(EXIT_FAILURE);
	}
	if(listen(sd2, QLEN) < 0){
		fprintf(stderr, "Error: Listen failed sd2\n");
		exit(EXIT_FAILURE);
	}

	FD_ZERO(&readfds);
	FD_SET(sd, &readfds);
	FD_SET(sd2, &readfds);

	FD_ZERO(&writefds);

	max_sd = sd2;

	/*
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	noTimeout.tv_sec = 0;
	noTimeout.tv_usec = 0;
	*/

	for(int i = 0; i < 510; i++){
		for(int j = 0; j < 10; j++){
			participants[i][j] = '\0';
			observers[i][j] = '\0';
		}
	}


	/* Main server loop - accept and handle requests */
	while (1) {

		// Initial the set of addresses with sockets that you want to monitor to know when they are ready to recv/accept.
		// select() modifies the set of addresses given to it, so we need to reset that set again.
		FD_ZERO(&wrk_readfds);
		FD_ZERO(&wrk_writefds);
		for (int i = 0; i < (max_sd + 1); ++i) {
			if (FD_ISSET(i, &readfds)) {
				FD_SET(i, &wrk_readfds);
			}
			if (FD_ISSET(i, &writefds)) {
				FD_SET(i, &wrk_writefds);
			}
		}

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		n = select((max_sd + 1), &wrk_readfds, &wrk_writefds, NULL, &tv);
		if (n < 0) {
			fprintf(stderr, "Error: select() failed. %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}


		// Some sockets are ready for, iterate through them
		for (int soc = 0; soc < (max_sd + 1); soc++){

			// process READ operations
			if (FD_ISSET(soc, &wrk_readfds)) {

				// if soc == sd, that means we have a new participant connecting
				if (soc == sd){
					alen = sizeof(cad);
					if ((sd3 = accept(sd, (struct sockaddr *) &cad, (socklen_t*) &alen)) < 0) {
						fprintf(stderr, "Error: Accept failed\n");
						exit(EXIT_FAILURE);
					}

					if(partCount > 254){
						send(sd3, &no, sizeof(char), 0);
						close(sd3);
						continue;
					} else{
						send(sd3, &yes, sizeof(char), 0);
						partCount++;
					}

					FD_SET(sd3, &readfds);
					if (sd3 > max_sd)
						max_sd = sd3;
					partTimes[sd3] = time(0);

				}

				// if soc == sd2, that means we have a new observer connecting
				else if (soc == sd2){
					alen = sizeof(cad);
					if ((sd3 = accept(sd2, (struct sockaddr *) &cad, (socklen_t*) &alen)) < 0) {
						fprintf(stderr, "Error: Accept failed\n");
						exit(EXIT_FAILURE);
					}

					if(obsCount > 254){
						send(sd3, &no, sizeof(char), 0);
						close(sd3);
						continue;
					} else{
						send(sd3, &yes, sizeof(char), 0);
						obsCount++;
					}

					FD_SET(sd3, &readfds);
					FD_SET(sd3, &writefds);
					if (sd3 > max_sd)
						max_sd = sd3;
					obsTimes[sd3] = time(0);

				}

				// otherwise, we are receiving data from a previously connected client
				else {

					// if the soc does not have a validated username in participants[soc][0-10],
					// we hand it to the username validation code
					if (!FD_ISSET(soc, &writefds) && participants[soc][0] == '\0'){

						// These timeouts didn't seem to work for us. Not sure why.
						// setsockopt(soc, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
						n = recv(soc, buf, sizeof(uint8_t), 0);
						//setsockopt(soc, SOL_SOCKET, SO_RCVTIMEO, &noTimeout, sizeof(noTimeout));
						endTime = time(0);

						if (n == 0){
							//participant shutdown
							FD_CLR(soc, &readfds);
							close(soc);
							partCount--;
							continue;
						}
						else if (endTime > partTimes[soc] + 60){

							fprintf(stderr, "Participant Timeout\n");

							// close participant
							FD_CLR(soc, &readfds);
							close(soc);
							partCount--;
							continue;
						}

						// otherwise, participant provided username within time limit
						userLen = (uint8_t) buf[0];
						n = recv(soc, buf, userLen, 0);

						for(int i = 0; i < userLen; i++){
							msg[i] = buf[i];
						}
						msg[userLen] = '\0';

						//validate new username
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
							else {
								check = 0;
								break;
							}
						}
						if (check == 1){

							//check to see if useranme is taken
							for(int i = 0; i < 510; i++){
								strcpy(temp, participants[i]);
								if (strcmp(msg, temp) == 0)
									check = 0;
							}

							if(check == 1){
								send(soc, &yes, sizeof(char), 0);

								for (int i=0; i < (userLen+1); i++){
									participants[soc][i] = msg[i];
								}

								for (int i=0; i < 1014; i++){
									stringBuild[i] = '\0';
								}

								strcpy(temp, participants[soc]);
								strcat(stringBuild, "User ");
								n = 5;
								for (int i=0; i < strlen(temp); i++){
									stringBuild[n] = temp[i];
									n++;
								}
								strcat(stringBuild, " has joined");
								msgLen = htons(strlen(stringBuild));

								printf("%s\n", stringBuild);

								for (int i=0; i < 510; i++){
									if (observers[i][0] != '\0'){
										send(i, &msgLen, sizeof(uint16_t), 0);
										send(i, &stringBuild, strlen(stringBuild), 0);
									}
                                                        	}
							}
							else {
								send(soc, &taken, sizeof(char), 0);
								partTimes[soc] = time(0);
							}
						}
						else {
							send(soc, &invalid, sizeof(char), 0);
							// don't reset timer
						}
					}

					// else, we received a message from a validated participant
					else if (!FD_ISSET(soc, &writefds)){

						n = recv(soc, (void *) &msgLen, sizeof(uint16_t), 0);

						if(n < 0){
							fprintf(stderr, "Receive fail\n");
							continue;
						}
						else if (n == 0){

							// client shutdown has been sent
							FD_CLR(soc, &readfds);
							close(soc);
							partCount--;

							// Build string
							strcpy(temp, participants[soc]);
							for (int i=0; i < 1014; i++){
								stringBuild[i] = '\0';
							}

                                                        strcpy(temp, participants[soc]);
							strcat(stringBuild, "User ");
							n = 5;
							for (int i=0; i < strlen(temp); i++){
							stringBuild[n] = temp[i];
								n++;
							}
							strcat(stringBuild, " has left");
							msgLen = htons(strlen(stringBuild));

							printf("%s\n", stringBuild);

							// Send the message
							for (int i=0; i < 510; i++){
								if (observers[i][0] != '\0'){
									send(i, &msgLen, sizeof(uint16_t), 0);
									send(i, &stringBuild, strlen(stringBuild), 0);
								}
							}

							//remove user from participant array
							for(int i = 0; i < 10; i++){
								participants[soc][i] = '\0';
							}

							// search for affiliated observer
							p = -1;
							for (int i=0; i < 510; i++){
								if (strcmp(temp, observers[i]) == 0){
									p = i;
									break;
								}
							}

							// if affiliated observer is found, close them
							if (p > -1){
								FD_CLR(p, &readfds);
								FD_CLR(p, &writefds);
								close(p);
								for (int i=0; i < 10; i++){
									observers[p][i] = '\0';
								}
								obsCount--;
							}
						}
						else {
							// if we received anything else, it's a message

							// convert length to host byte order
							msgLen = ntohs(msgLen);

							strcpy(temp, participants[soc]);
							if (msgLen > 1000){
								// close participant
								FD_CLR(soc, &readfds);
								close(soc);
								partCount--;

								// build message
								strcpy(temp, participants[soc]);
								for (int i=0; i < 1014; i++){
									stringBuild[i] = '\0';
								}

								strcpy(temp, participants[soc]);
								strcat(stringBuild, "User ");
								n = 5;
								for (int i=0; i < strlen(temp); i++){
									stringBuild[n] = temp[i];
									n++;
								}
								strcat(stringBuild, " has left");
								msgLen = htons(strlen(stringBuild));

								printf("%s\n", stringBuild);

								// send message
								for (int i=0; i < 510; i++){
									if (observers[i][0] != '\0'){
										send(i, &msgLen, sizeof(uint16_t), 0);
										send(i, &stringBuild, strlen(stringBuild), 0);
									}
								}

								// remove username from the participant array
								for (int i=0; i < 10; i++){
									participants[soc][i] = '\0';
								}

								// search for affiliated observer
								p = -1;
								for (int i=0; i < 510; i++){
									if (strcmp(temp, observers[i]) == 0){
										p = i;
										break;
									}
								}

								// if affiliated observer is found, close them
								if (p > -1){
									FD_CLR(p, &readfds);
									FD_CLR(p, &writefds);
									close(p);
									for (int i=0; i < 10; i++){
										observers[p][i] = '\0';
									}
									obsCount--;
								}
							}

							whitespace = 11 - strlen(temp);

							recv(soc, buf, msgLen, 0);

							check = 0;
							for(int i = 0; i < msgLen; i++){
								msg[i] = buf[i];
								if (msg[i] != ' ' && msg[i] != '\n')
									check = 1;
							}
							msg[msgLen] = '\0';

							//this is a private message
							n = 0;
							check = 1;
							if(msg[0] == '@'){
								for(int i = 0; i < msgLen; i++){
									if(i == 12){
										check = 0;
										break;
									}
									else if(msg[i] == ' '){
										for(int j = 1; j < i; j++){
											temp[n] = msg[j];
											n++;
										}
										break;
									}
								}
								p = -1;
								if(check == 1){
									check = 0;
									temp[n] = '\0';
									for(int i = 0; i < 510; i++){
										if(strcmp(temp, participants[i]) == 0){
											check = 1;
											for(int j = 0; j < 510; j++){
												if(strcmp(participants[i], observers[j]) == 0){
													p = j;
													break;
												}
											}
											break;
										}
									}
								}
								//we found the participant AND that observer
								if(p > -1 && check == 1 && strlen(temp) > 0){
									//subtract username out of message
									k = 0;
									for(int i = n + 2; i < msgLen; i++){
										buf[k] = msg[i];
										k++;
									}
									buf[k] = '\0';

									//create message to send
									strcpy(temp, participants[soc]);

									for(int i = 0; i < 1015; i++){
										stringBuild[i] = '\0';
									}
									stringBuild[0] = '%';

									for(int i = 1; i < (whitespace+1); i++){
										stringBuild[i] = ' ';
									}
									strcat(stringBuild, temp);
									strcat(stringBuild, ": ");
									strcat(stringBuild, buf);

									printf("%s\n", stringBuild);

									msgLen = htons(strlen(stringBuild));

									send(p, &msgLen, sizeof(uint16_t), 0);
									send(p, &stringBuild, strlen(stringBuild), 0);

								}
								//no observer, participant only
								else if(check == 1 && p == -1){
									//do nothing
								}
								//no observer no participant
								else{
									for(int i = 0; i < 1014; i++){
										stringBuild[i] = '\0';
									}
									strcat(stringBuild, "Warning: user ");
									for(int i = 0; i < msgLen; i++){
										if(msg[i] == ' '){
											n = i;
											break;
										}
									}
									k = 1;
									for(int i = 14; i < 997; i++){
										stringBuild[i] = msg[k];
										if (k != n)
											k++;
										if(k == n)
											break;
									}
									strcat(stringBuild, " doesn't exist...");

									msgLen = htons(strlen(stringBuild));

									strcpy(temp, participants[soc]);
									p = -1;
									for(int i = 0; i < 510; i++){
										if(strcmp(temp, observers[i]) == 0){
											p = i;
											break;
										}
									}
									//observer of sender exists
									if(p > -1){
										send(p, &msgLen, sizeof(uint16_t), 0);
										send(p, &stringBuild, strlen(stringBuild), 0);
									}
								}
							}
							//this is a public message
							else{

								if (check == 1 && msgLen > 0){

									for (int i=0; i < 1015; i++){
										stringBuild[i] = '\0';
									}

									stringBuild[0] = '>';
									for (int i=1; i < (whitespace+1); i++){
										stringBuild[i] = ' ';
									}
									strcat(stringBuild, temp);
									strcat(stringBuild, ": ");
									strcat(stringBuild, msg);
									printf("%s\n", stringBuild);

									msgLen = htons(strlen(stringBuild));

									for (int i=0; i < 510; i++){
										if (observers[i][0] != '\0'){
											send(i, &msgLen, sizeof(uint16_t), 0);
											send(i, &stringBuild, strlen(stringBuild), 0);
										}
									}
								}
							}
						}
					}

					// if we are an observer and have no associated username yet
					else if (FD_ISSET(soc, &writefds) && observers[soc][0] == '\0'){

						// get username
						n = recv(soc, buf, sizeof(uint8_t), 0);
						endTime = time(0);

						if (endTime > obsTimes[soc] + 60) {
							fprintf(stderr, "Observer Timeout\n");
							FD_CLR(soc, &writefds);
							FD_CLR(soc, &readfds);
							close(soc);
							obsCount--;
							continue;
						}

						userLen = (uint8_t) buf[0];
						n = recv(soc, buf, userLen, 0);

						// observer shutdown
						if (n == 0){
							FD_CLR(soc, &writefds);
							FD_CLR(soc, &readfds);
							close(soc);
							obsCount--;
						}

						//get username
						for(int i = 0; i < userLen; i++){
							msg[i] = buf[i];

						}
						msg[userLen] = '\0';

						//validate username
						check = 0;
						for(int i = 0; i < 510; i++){
							strcpy(temp, participants[i]);
							if (strcmp(msg, temp) == 0)
								check = 1;
						}
						if (check == 1){
							for (int i=0; i < 510; i++){
								strcpy(temp, observers[i]);
								if (strcmp(msg, temp) == 0)
									check = 0;
							}
						}
						else {
							// participant doesn't exist
							send(soc, &no, sizeof(char), 0);
						}

						if (check == 1){
							// valid association
							for (int i=0; i <userLen; i++){
								observers[soc][i] = msg[i];
							}
							observers[soc][userLen] = '\0';
							send(soc, &yes, sizeof(char), 0);

							printf("%s\n", newObs);
							msgLen = htons(25);
							for (int i=0; i < 510; i++){
								if (observers[i][0] != '\0'){
									send(i, &msgLen, sizeof(uint16_t), 0);
									send(i, &newObs, strlen(newObs), 0);
								}
							}
						}
						else {
							//already taken
							send(soc, &taken, sizeof(char), 0);
						}
					}
					else if (FD_ISSET(soc, &writefds) && observers[soc][0] != '\0'){
						n = recv(soc, buf, sizeof(uint8_t), MSG_DONTWAIT);
						if (n < 1){
							for (int i=0; i < 11; i++){
								observers[soc][i] = '\0';
							}
							FD_CLR(soc, &readfds);
							FD_CLR(soc, &writefds);
							close(soc);
							obsCount--;
						}
					}

				}
			}

			// process WRITE operations
			else if (FD_ISSET(soc, &wrk_writefds)){

			}

		}

	}
}

