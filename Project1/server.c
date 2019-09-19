#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#define MAXPENDING 5
#define RCVBUFSIZE 512

void DieWithError(char *errorMessage){
	fprintf(stderr, "%s\n", errorMessage);
	exit(1);
}
void HandleTCPClient(int clntSocket){
	char echoBuffer[RCVBUFSIZE];
	char *holder = (char *) malloc(RCVBUFSIZE);
	char *path = (char *) malloc(RCVBUFSIZE);
	if(!holder || !path){
		DieWithError("Failed to allocate memory");
	}
	
	memset(holder, '\0', RCVBUFSIZE);
	memset(path, '\0', RCVBUFSIZE);
	int recvMsgSize;

	if((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE - 1, 0)) < 0){
		DieWithError("recv() failed");
	}

	printf("\n\nREQUEST: %s\n\n", echoBuffer);

	char *temporary = malloc(RCVBUFSIZE);
	if(!temporary){
		close(clntSocket);
		DieWithError("Failed to allocate memory");
	}
	memset(temporary, '\0', RCVBUFSIZE); 
	temporary = strstr(echoBuffer, "HTTP/");
	if(!temporary){
		close(clntSocket);
		printf("Failed to locate HTTP version");
		return;
	}
	if((holder = strstr(echoBuffer, "GET ")) != NULL){
		memcpy(path, holder + 4, strlen(holder+4) - strlen(temporary) - 1);
		//printf("PATH SHOULD BE: %s\n", path);
		if(strcmp(path, "/") == 0 || strcmp(path, "") == 0 || strcmp(path, "index.html") == 0){
			//printf("Setting default path\n");
			strcpy(path, "TMDG.html");
		} else if(path[0] == '/'){
			int i = 0;
			for(i = 0; i < strlen(path); i++){
				path[i] = path[i+1];
			}
		}
		//printf("FINAL PATH: %s\n", path);
		FILE *fp = fopen(path, "r");
		//printf("FP: %p\n", fp);
		if(!fp){
			//printf("FP WAS NULL\n");
			char *response = "HTTP/1.1 404 Page Not Found\r\n\r\n<html><body><p> Could not locate page </p></body></html>";
			//printf("sending:\n%s\n", response);
			//fflush(stdout);
			send(clntSocket, response, strlen(response), 0);
			//printf("sent - ");
			//fflush(stdout);
			close(clntSocket);
			printf("Terminated socket: 404\n");
			fflush(stdout);
			return;
		}
		//printf("FP NOT NULL\n");
		strcpy(temporary,"HTTP/1.1");
		strcat(temporary, " 200 OK\r\n\r\n");
		//printf("\n%s\n", temporary);
		if(send(clntSocket, temporary, strlen(temporary), 0) != strlen(temporary)){
			close(clntSocket);
			printf("send() failed");
			return;
		}
		char sendMsg[500];
		int proceed = 1;
		while(proceed){
			int i = 0;
			for(i = 0; i < 499; i++){
				char c = getc(fp);
				if(c == EOF){
					sendMsg[i] = '\0';
					proceed = 0;
					break;
				}
				sendMsg[i] = c;
			}
			sendMsg[499] = '\0';
			if(send(clntSocket, sendMsg, strlen(sendMsg), 0) != strlen(sendMsg)){
				close(clntSocket);
				printf("send() failed");
				return;
			}
		}
		close(clntSocket);
	} else {
		memcpy(holder, temporary + 5, 3);
		strcpy(temporary, "HTTP/");
		strcat(temporary, holder);
		strcat(temporary, " 400 Bad Request\r\n\r\n");
		if(send(clntSocket, temporary, strlen(temporary), 0) != strlen(temporary)){
			printf("send() failed");
		}
		close(clntSocket);
	}
	printf("Finished with this client\n");
}

int main(int argc, char *argv[]){
	int servSock;
	int clntSock;
	struct sockaddr_in echoServAddr;
	struct sockaddr_in echoClntAddr;
	unsigned short echoServPort;
	unsigned int clntLen;

	if(argc != 2){
		fprintf(stderr, "Usage: %s <Server Port>\n", argv[0]);
		exit(1);
	}
	
	echoServPort = atoi(argv[1]);

	if((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP))<0){
		DieWithError("socket() failed");
	}

	memset(&echoServAddr, 0, sizeof(echoServAddr));
	echoServAddr.sin_family = AF_INET;
	echoServAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	echoServAddr.sin_port = htons(echoServPort);

	printf("Socket established\n");
	if(bind(servSock, (struct sockaddr*) &echoServAddr, sizeof(echoServAddr)) < 0){
		DieWithError("bind() failed");
	}
	printf("Server bound to port\n");
	if(listen(servSock, MAXPENDING) < 0){
		DieWithError("listen() failed");
	}

	printf("Waiting for connection...\n");
	fflush(stdout);
	for(;;){
		clntLen = sizeof(echoClntAddr);
		if((clntSock = accept(servSock, (struct sockaddr *) &echoClntAddr, &clntLen)) < 0){
			DieWithError("accept() failed");
		}

		//printf("Handling client %s\n", inet_ntoa(echoClntAddr.sin_addr));
		//fflush(stdout);
		HandleTCPClient(clntSock);
	}
	printf("We shouldn't get here");
	//fprintf(stderror, "We shouldn't get here");
}
