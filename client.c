#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include "client.h"

/*
 * create tcp socket, connect to scheduler
 */
int createSockTCPClient(int tcp_port){
	int sock_TCP_Client;
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0x00, sizeof(serverAddr));

	sock_TCP_Client = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_TCP_Client == -1){
		perror("create socket");
		return -1;
	}

	// set server address
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(HOST_NAME);
	serverAddr.sin_port = htons(tcp_port);

	if(connect(sock_TCP_Client, (struct sockaddr *)(&serverAddr), (socklen_t)(sizeof(serverAddr)) ) == -1){
		perror("TCP connect");
		return -1;
	}

	return sock_TCP_Client;
}

/*
 * send client and location to scheduler
 */
void sendRequest(int clientSocket, int location){
	char buff[BUFF_SIZE] = {0};
	sprintf(buff, "request:%d", location);
	if(send(clientSocket, buff, sizeof(buff)-1, 0) < 0){
		perror("send request");
		exit(-1);
	}
	printf("The client has sent query to Scheduler using TCP: client location ​%d\n", location);
}

/*
 * receive assignment result from scheduler
 */
void receiveResponse(int clientSocket){
	char buff[BUFF_SIZE] = {0};
	if(recv(clientSocket, buff, sizeof(buff)-1, 0) <0 ){
		perror("recv response");
		close(clientSocket);
		exit(-1);
	}

	if(strstr(buff, "assigned") != NULL){	// if hospital assigned
		printf("The client has received results from the Scheduler: %s\n", buff);
	}
	else
		printf("%s", buff);
}

int main(int argc, char* argv[]) {
	int client_id, location;
	int sock_TCP_Client;
	if(argc < 2){	// check command line arguments
		perror("wrong command line arguments");
		exit(-1);
	}
	location = atoi(argv[1]);

	printf("The client is up and running\n");
	sock_TCP_Client = createSockTCPClient(SCHEDULER_TCP_PORT);
	sendRequest(sock_TCP_Client, location);
	receiveResponse(sock_TCP_Client);

	close(sock_TCP_Client);
	return 0;
}
