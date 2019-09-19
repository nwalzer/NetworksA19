#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <libgen.h>
#include <sys/time.h>
#include <errno.h>

#define RCVBUFSIZE 1024	

void DieWithError(char *errorMessage){
	fprintf(stderr, "%s\n", errorMessage);
	exit(1);
}

int get_index(char* string, char c) {
    char *e = strchr(string, c);
    if (e == NULL) {
        return -1;
    }
    return (int)(e - string);
}

int main(int argc, char *argv[]){
	int sock;
	char *echoServPort = (char *) malloc(16);
	char *echoString;
	char echoBuffer[RCVBUFSIZE];
	unsigned int echoStringLen;
	int bytesRcvd;
	int recordTime = 0;

	if((argc < 3) || (argc > 4)){
		fprintf(stderr, "Usage: %s [-p] <URL or IP> <Echo Port>\n", argv[0]);
		exit(1);
	}
	if(argc == 3){
		echoString = argv[1];
		echoServPort = argv[2];
	} else {
		if(strcmp(argv[1], "-p") != 0){
			fprintf(stderr, "Invalid Option. Valid Options are: '-p'\n");
			exit(1);
		}
		
		echoString = argv[2];
		echoServPort = argv[3];
		recordTime = 1;
	}

	struct addrinfo address, *res;
	memset(&address, 0, sizeof(address));
	address.ai_family = AF_INET;
	address.ai_socktype = SOCK_STREAM;
	int error;
	
	char *path = strstr(echoString, "/");	
	char *host = (char *) malloc(RCVBUFSIZE);

	if(!host){
		DieWithError("Failed to allocate memory");
	} else if(!path){
		path = "/";
		memcpy(host, echoString, RCVBUFSIZE);
	} else {
		path++;
		memcpy(host, echoString, path - echoString - 1);
	}

	fflush(stdout);
	if((error = getaddrinfo(host, echoServPort, &address, &res)) != 0){
		DieWithError("Error resolving DNS");
	}

	if((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		DieWithError("socket() failed");
	}
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;
	if(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) != 0){
		DieWithError("Failed to set socket timeout");
	}

	struct timeval start;
	unsigned long RTT = 0;
	if(recordTime){
		if(gettimeofday(&start, NULL) < 0){
			DieWithError("Failed to get time of day - exiting");
		}
		RTT = start.tv_sec * 1000 + start.tv_usec / 1000;
	}	
	if(connect(sock, res->ai_addr, res->ai_addrlen) < 0){
		DieWithError("connect() failed");
	}
	if(recordTime){
		if(gettimeofday(&start, NULL) < 0){
			DieWithError("Failed to get time of day - exiting");
		}
		RTT = (start.tv_sec * 1000 + start.tv_usec / 1000) - RTT;
		printf("One RTT is %lu milliseconds\n", RTT);
	}
	
	echoStringLen = strlen(echoString);

	char getReq[1024];
	strcpy(getReq, "GET ");
	strcat(getReq, path);
	strcat(getReq, " HTTP/1.1\r\nConnection: Close\r\n\r\n");
	echoStringLen = strlen(getReq);
	printf("REQUEST: %s\n", getReq);

	if(send(sock, getReq, echoStringLen, 0) != echoStringLen){
		DieWithError("send() sent a different number of bytes than expected");
	}

	FILE *fp;
	
	fp = fopen("received.html","w");
	int proceed = 1;
	int foundBody = 0;
	char *temp = (char *) malloc(512);
	char *toPrint = (char *) malloc(512);
	
	while(proceed){
		if((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) < 0){
			//close(sock);
			DieWithError(strerror(errno));	
		}
		if(bytesRcvd == 0){
			break;
		}
		if(foundBody == 0){
			char *bodyPtr = (char *) malloc(512);
			if((bodyPtr = strstr(echoBuffer, "\r\n\r\n")) != NULL){
				foundBody = 1;
			} else {
				printf("%s", echoBuffer);
				fflush(stdout);
				continue;
			}
			
			if(strlen(bodyPtr) == strlen(echoBuffer)){
				printf("%s", echoBuffer);
				fflush(stdout);
				strcpy(toPrint, "");
			} else {
				memcpy(temp, echoBuffer, strlen(echoBuffer) - strlen(bodyPtr));
				printf("%s", temp); //this will be the header lines
				memcpy(toPrint, bodyPtr, strlen(bodyPtr));
			}
			//printf("FINISHED MEMCPY\n");
			fflush(stdout);
						
		} else if(strstr(echoBuffer, "</html>") != NULL){
			//toPrint = strtok(echoBuffer, "</html>");
			strcat(toPrint, "</html>");
		} else {
			strcpy(toPrint, echoBuffer);
		}
		//printf("%s", toPrint);
		fprintf(fp, "%s", toPrint);
		fflush(fp);
	}
	//fclose(fp);
	printf("\n\n\nResponse Processed... Wrote to recieved.html\n");
	fflush(stdout);
	//close(sock);
	exit(0);
}
