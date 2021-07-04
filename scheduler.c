/*
 *	ps -aux | grep developer
 *	kill -9 <process number>
 *	tar cvf ee450_yourUSCusername_session#.tar *
 *	gzip ee450_yourUSCusername_session#.tar
 */
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

#include "scheduler.h"

// structure to save hospital info
typedef struct Hospital_S{
	int capacity;
	int occupation;
	double score;
	double distance;
	char hospital_name;
	int udp_port_server;
	int udp_port_client;
	int sock_UDP_client;
	struct sockaddr_in dest_addr;
	int wait_flag;
}Hospital;

/*
 * check flag to wait hospital UDP message
 * return 1 if all wait flag cleared
 *
 */
int canAllHospitalGo(Hospital hospitals[]){
	for(int i = 0; i < HOSPITAL_COUNT; i ++)
		if(hospitals[i].wait_flag == 1)
			return 0;
	return 1;
}

/*
 * update hospital info according to received udp message
 * need remove wait flag if necessary
 */
int updateHospitalStatus(char buff[], Hospital hospitals[]){
	int udp_ports[] = {HOSPITAL_UDP_PORT_A, HOSPITAL_UDP_PORT_B, HOSPITAL_UDP_PORT_C};
	int index = -1;
	char *p = strtok(buff, ":");
	if(strcmp(p, "Hospital") == 0){
		p = strtok(NULL, ":");
		if(*p >= 'A' && *p <= 'C'){		// if correct UDP message from Hospital
			index = *p - 'A';
			p = strtok(NULL, ":");
			if(strcmp(p, "capacity_occupancy") == 0){	// update capacity occupancy
				p = strtok(NULL, ":");
				hospitals[index].capacity = atoi(p);
				p = strtok(NULL, ":");
				hospitals[index].occupation = atoi(p);
				hospitals[index].hospital_name = 'A' + index;
				hospitals[index].udp_port_server = udp_ports[index];
				printf("The Scheduler has received information from Hospital %c: total capacity is %d​ and initial occupancy is %d\n",
						hospitals[index].hospital_name, hospitals[index].capacity, hospitals[index].occupation);
				hospitals[index].wait_flag = 0;							
			}
			else if(strcmp(p, "map_information") == 0){	// update score distance info
				char str_score[INFO_SIZE] = "None";
				char str_distance[INFO_SIZE] = "None";
				p = strtok(NULL, ":");
				hospitals[index].score = atof(p);
				p = strtok(NULL, ":");
				hospitals[index].distance = atof(p);
				if(hospitals[index].distance > 0)
					sprintf(str_distance, "%f", hospitals[index].distance);
				if(hospitals[index].score > 0)
					sprintf(str_score, "%f", hospitals[index].score);
				printf("The Scheduler has received map information from Hospital %c, the score = %s​ and the distance = %s\n",
						hospitals[index].hospital_name, str_score, str_distance);
				hospitals[index].wait_flag = 0;						
			}
		}
	}

	return 0;
}

/*
 * create UDP server sock to receive UDP messages from hospitals
 */
int createUDPServer(){
	int sock_UDP_server;
	struct sockaddr_in saddr = {0};

	sock_UDP_server = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_UDP_server == -1){
		perror("UDP socket create");
		exit(-1);
	}

	saddr.sin_family = AF_INET;
	saddr.sin_port	 = htons(SCHEDULER_UDP_PORT);
	saddr.sin_addr.s_addr = INADDR_ANY;

	// bind to specified UDP port
	if (bind(sock_UDP_server, (struct sockaddr *)&saddr, sizeof(saddr)) == -1){
		perror("UDP socket bind");
		exit(-1);
	}

	return sock_UDP_server;
}

/*
 * read UDP message to buff
 */
void receivefromUDP(int sock_UDP_server, char buff[]){
	int len;
	len = recvfrom(sock_UDP_server, buff, BUFF_SIZE - 1, 0, NULL, NULL);
	if(len == -1) {
		perror("client request read");
		exit(-1);
	}
	buff[len] = '\0';
}

/*
 * loop reading UDP message until all hospitals not need wait any more
 * return 1 if client location not found in map
 */
int readAllHospitalInfobyUDP(int sock_UDP_server, Hospital hospitals[]){
	char buff[BUFF_SIZE];
	int flag_not_found = 0;

	while(1) {
		if(canAllHospitalGo(hospitals) == 1)	// check if all wait flag cleared
			break;
		receivefromUDP(sock_UDP_server, buff);	// read UDP message
		if(flag_not_found == 0 && strstr(buff, "location:not found") != NULL)
			flag_not_found = 1;
		updateHospitalStatus(buff, hospitals);	// update hospitals' status, change wait flag if necessary
	}
	return flag_not_found;
}

/*
 * create TCP socket, bind and listen client to accept
 */
int createTCPServer(){
	int serverSocket = 0;
	struct sockaddr_in serverAddr = {0};
	serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if(serverSocket == -1) {
		perror("create socket");
		exit(-1);
	}

	serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(SCHEDULER_TCP_PORT);

	if(bind(serverSocket, (struct sockaddr *)(&serverAddr), (socklen_t)(sizeof(serverAddr)) ) == -1) {
		perror("bind TCP");
		exit(-1);
	}

	if(listen(serverSocket, 5) == -1){
		perror("TCP listen");
        exit(-1);
    }
	return serverSocket;
}

/*
 * wait client to connect by TCP
 */
int waitTCPconnection(int sock_TCP_Server){
	int clientSocket = accept(sock_TCP_Server, NULL, NULL);
	if(clientSocket == -1){
		perror("TCP accept");
		exit(-1);
	}
	return clientSocket;
}

/*
 * receive client request location
 */
int receiveClientLocation(int sock_TCP_client){
	char buff[BUFF_SIZE] = {0};
	char *p;
	int location = -1;
	if(recv(sock_TCP_client, buff, BUFF_SIZE - 1, 0) < 0) {
		perror("TCP recv");
		exit(-1);
	}
	p = strchr(buff, ':');
	if(p == NULL){
		perror("client request formats");
		exit(-1);
	}
	location = atoi(p + 1);
	printf("The Scheduler has received client at location %d​ from the client using TCP over port %d\n", location, SCHEDULER_TCP_PORT);
	return location;
}

/*
 * send UDP message to hospital
 */
void sendUDPMessage(Hospital* p_hospital, char buff[]){
//	sendto(p_hospital->sock_UDP_client, buff, strlen(buff), 0, (struct sockaddr *)&dest_addr,sizeof(dest_addr));
	send(p_hospital->sock_UDP_client, buff, strlen(buff), 0);
}

/*
 * forward client request to available hospitals
 */
int forwardRequest(int sock_UDP_server, Hospital hospitals[], int client_location){
	char buff[BUFF_SIZE] = {0};
	struct sockaddr_in my_addr;
	int addrlen;
	sprintf(buff, "client_request:%d", client_location);
	for(int i = 0; i < HOSPITAL_COUNT; i ++){
		hospitals[i].sock_UDP_client = socket(AF_INET, SOCK_DGRAM, 0);
		if(hospitals[i].sock_UDP_client == -1){
			perror("Scheduler socket build");
			exit(-1);
		}

		// send message to available hospital
		if(hospitals[i].capacity > hospitals[i].occupation){
			hospitals[i].wait_flag = 1;
			hospitals[i].dest_addr.sin_family = AF_INET;
			hospitals[i].dest_addr.sin_port = htons(hospitals[i].udp_port_server);
			hospitals[i].dest_addr.sin_addr.s_addr = inet_addr(HOST_NAME);
			connect(hospitals[i].sock_UDP_client,(struct sockaddr*)&hospitals[i].dest_addr, sizeof(hospitals[i].dest_addr));

//			sendUDPMessage(&hospitals[i], buff);
			send(hospitals[i].sock_UDP_client, buff, strlen(buff), 0);
			printf("The Scheduler has sent client location to Hospital %c using UDP over port %d\n", hospitals[i].hospital_name, hospitals[i].udp_port_client);
			if (getsockname(hospitals[i].sock_UDP_client, (struct sockaddr *)&my_addr, (socklen_t *)&addrlen) == -1) {
				perror("getsockname");
				exit(-1);
			}
			int dddd = ntohs(my_addr.sin_port);
			hospitals[i].udp_port_client = ntohs(my_addr.sin_port);
		}
		else
			hospitals[i].wait_flag = 0;	// no need to wait unavailable hospitals UDP messages
	}
	return 0;
}

/*
 * send client assignment result to each hospitals, either assigned or not
 */
void informHospitalbyUDP(Hospital hospitals[], int assigned_index){
	char str_assigned[] = "assigned";
	char str_unassigned[] = "None";
	for(int i = 0; i < HOSPITAL_COUNT; i ++){
		if( i == assigned_index) {	// to hospital assigned
			sendUDPMessage(&hospitals[i], str_assigned);
			hospitals[i].occupation += 1;
			printf("The Scheduler has sent the result to Hospital %c​ using UDP over port %d\n", 'A' + assigned_index, hospitals[i].udp_port_client);
		}
		else	// to hospital unassigned
			sendUDPMessage(&hospitals[i], str_unassigned);
	}
}

/*
 * choose the hospital according to assign rules
 * return
 * index of hospital assigned
 * -1 if no hospital assigned
 */
int assignedHospital(Hospital hospitals[]){
	int assigned_index = -1; 	// no hospital assigned by default
	double highest_score = -1;
	int highest_index[HOSPITAL_COUNT];
	int highest_count = 0;
	double distance_shortest;
	for(int i = 0; i < HOSPITAL_COUNT; i ++){
		// None for distance < 0, need skip and check next hospital if current has no bed, or client location not found, or same location as hospital
		if(hospitals[i].capacity <= hospitals[i].occupation || hospitals[i].distance < 0){
			continue;
		}

		if(hospitals[i].score >= 0) {
			if(highest_score < 0 || hospitals[i].score > highest_score){ // need record highest score
				highest_score = hospitals[i].score;
				highest_index[0] = i;
				highest_count = 1;
			}
			else if(hospitals[i].score == highest_score){	// need record same highest score's hospital index
				highest_index[highest_count] = i;
				highest_count ++;
			}
		}
	}
	if(highest_score < 0)	// no hospital assigned
		return -1;

	// for the same score, find the hospital with shortest path
	assigned_index = highest_index[0];	//first one by default
	distance_shortest = hospitals[assigned_index].distance;
	for(int i = 1; i < highest_count; i ++){
		if(hospitals[highest_index[i]].distance < distance_shortest){
			assigned_index = highest_index[i];
			distance_shortest = hospitals[assigned_index].distance;
		}
	}
	printf("The Scheduler has assigned Hospital ​%c to the client\n", 'A' + assigned_index);
	return assigned_index;
}

/*
 * send assignment result to client
 */
void sendTCPMessage(int sock_TCP_client, char buff[]){
	if(send(sock_TCP_client, buff, BUFF_SIZE - 1, 0) < 0){
		perror("send TCP message");
		exit(-1);
	}
}

int main(void) {
	int sock_UDP_server;
	int sock_TCP_Server;
	int client_location;
	Hospital hospitals[HOSPITAL_COUNT];
	char buff[BUFF_SIZE] = {0};

	// initial hospital status
	for(int i =0 ;i < HOSPITAL_COUNT; i ++){
		hospitals[i].wait_flag = 1;
		hospitals[i].score = -1;
		hospitals[i].distance = -1;
	}

	printf("The Scheduler is up and running.\n");
	sock_UDP_server = createUDPServer();
	sock_TCP_Server = createTCPServer();

	// read all hospitals' capacity and occupation
	readAllHospitalInfobyUDP(sock_UDP_server, hospitals);

	while(1){
		int assigned_index = -1;	// no hospital assigned by default
		int flag_not_found;
		int sock_TCP_client = waitTCPconnection(sock_TCP_Server);	// wait for client connection
		client_location = receiveClientLocation(sock_TCP_client);		// wait client request, and get client location

		forwardRequest(sock_UDP_server, hospitals, client_location);	// send client request to available hospitals

		flag_not_found = readAllHospitalInfobyUDP(sock_UDP_server, hospitals);	// wait all hospital response for each request
		if(flag_not_found == 1){
			sprintf(buff, "Location %d not found", client_location);
		}
		else {
			assigned_index = assignedHospital(hospitals);
			if(assigned_index == -1) {
				strcpy(buff, "Score = None, No assignment");
			}
			else
				sprintf(buff, "assigned to Hospital:%c", 'A' + assigned_index);
		}
		sendTCPMessage(sock_TCP_client, buff);
		printf("The Scheduler has sent the result to client using TCP over port %d\n", SCHEDULER_TCP_PORT);
		informHospitalbyUDP(hospitals, assigned_index);
		// close TCP sock for current clients
		close(sock_TCP_client);
		sleep(1);		// need sleep 1s, or will open old connect, and read old client request twice
	}

	// close all socks
	close(sock_UDP_server);
	close(sock_TCP_Server);
	for(int i = 0; i < HOSPITAL_COUNT; i ++){
		close(hospitals[i].sock_UDP_client);
	}
	return 0;
}

