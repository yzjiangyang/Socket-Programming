#include "hospitalA.h"
#define HOSPITAL_NAME "A"
#define HOSPITAL_UDP_PORT 30684

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

// single link to save edges
Edge* p_Edge_list_head = NULL;
Edge* p_Edge_list_tail = NULL;
// single link to save vertex
Vertex* p_Vertex_list_head = NULL;
Vertex* p_Vertex_list_tail = NULL;

/*
 * search vertex if existed in link or not
 */
int foundVertex(Vertex* p_Vertex_list_head, int vertex_id){
	Vertex* p_vertex = p_Vertex_list_head;
	while(p_vertex != NULL){
		if(p_vertex->vertex_ID == vertex_id)
			return 1;
		p_vertex = p_vertex->p_next;
	}
	return 0;
}

/*
 * try to add vertex to link, do nothing if vertex id already existed in link
 */
void tryAddVertex(int vertex_id){
	if(foundVertex(p_Vertex_list_head, vertex_id) == 0){
		Vertex* p_vertex = malloc(sizeof(Vertex));
		p_vertex->vertex_ID = vertex_id;
		p_vertex->p_next = NULL;
		if(p_Vertex_list_head == NULL)
			p_Vertex_list_head = p_Vertex_list_tail = p_vertex;
		else {
			p_Vertex_list_tail->p_next = p_vertex;
			p_Vertex_list_tail = p_vertex;
		}
	}
}

/*
 * add edge to the tail of link
 */
void addEdge2Link(Edge* p_Edge){
	if(p_Edge_list_head == NULL)
		p_Edge_list_head = p_Edge_list_tail = p_Edge;
	else{
		p_Edge_list_tail->p_next = p_Edge;
		p_Edge_list_tail = p_Edge;
	}
}

/*
 * parse vertex and edge from buff, add vertex and edge to their links
 */
int addEdgeVertex(char buff[]){
	int vertex_L, vertex_R;
	double distance;
	char *p = strtok(buff, " ");
	if(p != NULL)
		vertex_L = atoi(p);
	else
		return -1;
	p = strtok(NULL, " ");
	if( p != NULL)
		vertex_R = atoi(p);
	else
		return -1;
	p = strtok(NULL, " ");
	if( p != NULL)
		distance = atof(p);
	else
		return -1;

	tryAddVertex(vertex_L);
	tryAddVertex(vertex_R);
	Edge* p_Edge = malloc(sizeof(Edge));;
	p_Edge->vertex_L = vertex_L;
	p_Edge->vertex_R = vertex_R;
	p_Edge->distance = distance;
	addEdge2Link(p_Edge);

	return 0;
}

/*
 * load map.txt to vertex link, and edge link
 */
int loadMapFile(char filename[]){
    FILE *fp = fopen(filename, "r");
	char buff[BUFF_SIZE];
    if(fp == NULL) {
        printf("Error: Map file open");
        exit(-1);
    }
    while(!feof(fp)){
    	fgets(buff, BUFF_SIZE, fp);
    	addEdgeVertex(buff);
    }
    fclose(fp);
    return 0;
}

/*
 * map vertex id to offset in array
 */
int getVertexIndex(int* p_VertexArray, int vertex_count, int vertex_id){
	for(int i = 0; i < vertex_count; i ++)
		if(p_VertexArray[i] == vertex_id)
			return i;
	return -1;
}

/*
 * receive UDP message
 */
void receivefromUDP(int sock_udp_server, char buff[]){
	int len;
	len = recvfrom(sock_udp_server, buff, BUFF_SIZE, 0, NULL, NULL);
	if(len == -1) {
		printf("Error: wrong input from client\n");
		exit(-1);
	}
	buff[len] = '\0';
}

/*
 * free edge link and vertex link before exit the program
 */
void freelink(){
	Edge* to_delete_edge;
	Vertex* to_delete_vertex;
	while(p_Edge_list_head != NULL){
		to_delete_edge = p_Edge_list_head;
		p_Edge_list_head = p_Edge_list_head->p_next;
		free(to_delete_edge);
	}

	while(p_Vertex_list_head != NULL){
		to_delete_vertex = p_Vertex_list_head;
		p_Vertex_list_head = p_Vertex_list_head->p_next;
		free(to_delete_vertex);
	}
}

/*
 * get size of vertex link
 */
int getlinksize(Vertex* p_head){
	int count = 0;
	Vertex* p_vertex = p_head;
	while(p_vertex != NULL){
		count ++;
		p_vertex = p_vertex->p_next;
	}
	return count;
}

/*
 * create adjacency matrix from vertex array
 */
double* createMatrix(Edge* p_Edge_list_head, int* p_VertexArray, int vertex_count){
	double* p_Matrix = malloc(sizeof(double) * vertex_count * vertex_count);
	Edge* p_edge = p_Edge_list_head;
	int vertex_index_L, vertex_index_R;
	for(int i = 0; i < vertex_count * vertex_count; i ++)
		p_Matrix[i] = 0.0;

	while(p_edge != NULL){
		vertex_index_L = getVertexIndex(p_VertexArray, vertex_count, p_edge->vertex_L);
		vertex_index_R = getVertexIndex(p_VertexArray, vertex_count, p_edge->vertex_R);
		p_Matrix[vertex_index_L* vertex_count + vertex_index_R] = p_edge->distance;
		p_Matrix[vertex_index_R* vertex_count + vertex_index_L] = p_edge->distance;
		p_edge = p_edge->p_next;
	}

	return p_Matrix;
}

/*
 * build vertex array, then can map vertex id to vertex offset, re-index
 */
int* buildVerteArray(Vertex* p_head, int vertex_count){
	int index = 0;
	Vertex* p_vertex = p_head;
	int* p_VertexArray = malloc(sizeof(int) * vertex_count);

	while(p_vertex != NULL){
		p_VertexArray[index] = p_vertex->vertex_ID;
		p_vertex = p_vertex->p_next;
		index ++;
	}
	return p_VertexArray;
}

/*
 * create UDP server return UDP socket
 */
int createUDPServer(int udp_port){
	int sock_UDP_server;
	struct sockaddr_in saddr;
	int res;

	//1. create socket
	sock_UDP_server = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_UDP_server == -1) {
		printf("Error: UDP socket create error\n");
		exit(-1);
	}

	//2. prepare IP and port
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port	 = htons(udp_port);
	saddr.sin_addr.s_addr = INADDR_ANY;
	bzero(saddr.sin_zero, 8);

	//3. bind
	res = bind(sock_UDP_server, (struct sockaddr *)&saddr, sizeof(saddr));
	if (-1 == res)
	{
		printf("Error: UDP socket bind error\n");
		exit(-1);
	}

	return sock_UDP_server;
}

/*
 * create UDP client to send to scheduler
 */
int createSockUDPClient(){
	int sock_UDP_client = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock_UDP_client == -1){
		printf("Error: Scheduler socket build\n");
		exit(-1);
	}
	return sock_UDP_client;
}

/*
 * get shorter distance between dis_1 and dis_2
 */
double getShorterDistance(double dis_1, double dis_2){
	if(dis_1 == 0)
		return dis_2;
	if(dis_2 == 0)
		return dis_1;
	if(dis_1 > dis_2)
		return dis_2;
	else
		return dis_1;
}

/*
 * return distance with path from hospital to middle_index, to last i
 */
double getDistance(double adjacency_matrix[], int vertex_count, int hospital_index, int middle_index, int i){
	if(hospital_index == middle_index)
		return adjacency_matrix[hospital_index * vertex_count + i];
	if(adjacency_matrix[hospital_index * vertex_count + middle_index] == 0 || adjacency_matrix[middle_index * vertex_count + i] == 0)
		return 0.0;
	return adjacency_matrix[hospital_index * vertex_count + middle_index] + adjacency_matrix[middle_index * vertex_count + i];
}

/*
 * search closest vertex index from hospital to set U
 * return -1 if set U is empty, or U has no path to set S
 */
int searchShortestinsetU(double adjacency_matrix[], int vertex_count, int* flag_SU, int hospital_index){
	double shortest = 0;
	int index_move = -1;
	for(int i = 0; i < vertex_count; i ++){
		if(*(flag_SU + i) == 0){
			double dis_hos_i = adjacency_matrix[hospital_index * vertex_count + i];
			if(index_move == -1) {
				index_move = i;
				shortest = dis_hos_i;
			}
			else {
				double new_dis = getShorterDistance(shortest, dis_hos_i);
				if(new_dis != shortest){
					index_move = i;
					shortest = new_dis;
				}
			}
		}
	}
	if(shortest == 0)	// if set U has no path to set S
		return -1;

	if(index_move != -1) {// if shortest vertex found, set flag and update shortest distance
		flag_SU[index_move] = 1;
		adjacency_matrix[hospital_index * vertex_count + index_move] = shortest;
		adjacency_matrix[index_move * vertex_count + hospital_index] = shortest;
	}
	return index_move;
}

/*
 * as index_move joined set S, need update distance from hospital to vertex in set U
 */
void updatesetSdistance(double adjacency_matrix[], int vertex_count, int* flag_SU, int hospital_index, int index_move){
	for(int i = 0; i < vertex_count; i ++){
		if(i == hospital_index || i == index_move)
			continue;
		if(*(flag_SU + i) == 0){
			double new_dis = getDistance(adjacency_matrix, vertex_count, hospital_index, index_move, i);
			double old_dis = adjacency_matrix[hospital_index * vertex_count + i];
			if(old_dis != getShorterDistance(old_dis, new_dis)){
				adjacency_matrix[hospital_index * vertex_count + i] = new_dis;
				adjacency_matrix[i * vertex_count + hospital_index] = new_dis;
			}
		}
	}
}

/*
 * search shortest path from hospital index to client index
 * build a group from 0 to all locations
 */
double calculateDistance(double adjacency_matrix[], int vertex_count, int hospital_index, int client_index, int* p_VertexArray){
	int index_move = -1;
	int* flag_SU = malloc(sizeof(int) * vertex_count);
	if(hospital_index == client_index)	// if client the same as hospital, return 0
		return 0.0;
	memset(flag_SU, 0, sizeof(int) * vertex_count);
	*(flag_SU + hospital_index) = 1;	// set hospital_index in set S, leave others in set U

	while(1){
		index_move = searchShortestinsetU(adjacency_matrix, vertex_count, flag_SU, hospital_index);
		if(index_move == -1)
			break;
		updatesetSdistance(adjacency_matrix, vertex_count, flag_SU, hospital_index, index_move);
	}

	free(flag_SU);
	return adjacency_matrix[hospital_index * vertex_count + client_index];	// distance of hos row updated to shortest, can return shortest path now
}

int main(int argc, char* argv[]) {
	int hospital_location, total_capacity, initial_occupancy;
	double* adjacency_matrix;
	int* p_VertexArray;
	int vertex_count;
	int sock_UDP_client;
	struct sockaddr_in dest_addr = {0};
	char buff[BUFF_SIZE];
	int sock_UDP_server;
	int location_client;
	int hospital_index;
	char str_distance[INFO_SIZE];
	char str_score[INFO_SIZE];
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(SCHEDULER_UDP_PORT);
	dest_addr.sin_addr.s_addr = inet_addr(HOST_NAME);

	if(argc < 4){
		printf("Error: command line arguments\n");
		exit(-1);
	}
	hospital_location = atoi(argv[1]);
	total_capacity = atoi(argv[2]);
	initial_occupancy = atoi(argv[3]);

	loadMapFile("map.txt");	// load edge data to link of p_head
	vertex_count = getlinksize(p_Vertex_list_head);	// get count of vertex
	p_VertexArray = buildVerteArray(p_Vertex_list_head, vertex_count);
	hospital_index = getVertexIndex(p_VertexArray, vertex_count, hospital_location);	// map hospital location to index
	if(hospital_index == -1){
		printf("Error: Hospital hospital_location not in map\n");
		exit(-1);
	}

	adjacency_matrix = createMatrix(p_Edge_list_head, p_VertexArray, vertex_count);		// create adjacency matrix

	sock_UDP_server = createUDPServer(HOSPITAL_UDP_PORT);
	printf("Hospital %s is up and running using UDP on port %d.\n", HOSPITAL_NAME, HOSPITAL_UDP_PORT);
	printf("Hospital %s has total capacity %d and initial occupancy %d.\n", HOSPITAL_NAME, total_capacity, initial_occupancy);
	sock_UDP_client = createSockUDPClient();
	sprintf(buff, "Hospital:%s:capacity_occupancy:%d:%d\n", HOSPITAL_NAME, total_capacity, initial_occupancy);
	sendto(sock_UDP_client, buff, strlen(buff), 0, (struct sockaddr *)&dest_addr,sizeof(dest_addr));

	while(1) {
		// -1 means None
		double distance = -1;
		double availability = -1;
		double score = -1;
		int client_index;
		char* p;

		receivefromUDP(sock_UDP_server, buff);
		p = strstr(buff, ":");
		if( p == NULL){
			perror("client request format error");
			exit(-1);
		}
		location_client = atoi(p + 1);		// get client location
		printf("Hospital %s has received input from client at location %d\n", HOSPITAL_NAME, location_client);

		client_index = getVertexIndex(p_VertexArray, vertex_count, location_client);	// map client location to index
		if(client_index == -1){
			printf("Hospital %s does not have the location %d in map\n", HOSPITAL_NAME, location_client);
			sprintf(buff, "Hospital:%s:location:not found", HOSPITAL_NAME);
			sendto(sock_UDP_client, buff, strlen(buff), 0, (struct sockaddr *)&dest_addr,sizeof(dest_addr));
			printf("Hospital %s has sent \"location not found\" to the Scheduler\n", HOSPITAL_NAME);
		}

		availability = ((double)total_capacity - initial_occupancy)/total_capacity;		// availability should not be negative or larger than 1
		printf("Hospital %s has capacity = ​%d​, occupation= ​%d​, availability = %f\n", HOSPITAL_NAME, total_capacity, initial_occupancy, availability);
		if(client_index == -1 || location_client == hospital_location){ // location not found or same with hospital location
			distance = -1;
			score = -1;
			strcpy(str_distance, "None");
			strcpy(str_score, "None");
			printf("Hospital %s has found the shortest path to client, distance = None​\n", HOSPITAL_NAME);
			printf("Hospital %s has the score = ​None\n", HOSPITAL_NAME);
		}
		else {
			distance = calculateDistance(adjacency_matrix, vertex_count, hospital_index, client_index, p_VertexArray);
			if(distance == 0) {
				printf("Hospital %s has found the shortest path to client, distance = None​\n", HOSPITAL_NAME);
				strcpy(str_distance, "None");
				score = -1;
				printf("Hospital %s has the score = ​None\n", HOSPITAL_NAME);	
				sprintf(str_distance, "None");
				sprintf(str_score, "None");
			}
			else {
				printf("Hospital %s has found the shortest path to client, distance = %f\n​", HOSPITAL_NAME, distance);
				sprintf(str_distance, "%f", distance);
				score = 1/(distance * (1.1 - availability));
				printf("Hospital %s has the score = ​%f\n", HOSPITAL_NAME, score);				
				sprintf(str_distance, "%f", distance);
				sprintf(str_score, "%f", score);				
			}
		}
		sprintf(buff, "Hospital:%s:map_information:%f:%f\n", HOSPITAL_NAME, score, distance);
		sendto(sock_UDP_client, buff, strlen(buff), 0, (struct sockaddr *)&dest_addr,sizeof(dest_addr));
		printf("Hospital %s has sent score = %s and distance = %s to the Scheduler\n", HOSPITAL_NAME, str_score, str_distance);

		receivefromUDP(sock_UDP_server, buff);
		if(strstr(buff, "assigned") != NULL){
			initial_occupancy ++;
			availability = (total_capacity - initial_occupancy)/total_capacity;		// availability should not be minus or larger than 1
			printf("Hospital %s has been assigned to a client, occupation is updated to %d, availability is updated to ​%f\n",
					HOSPITAL_NAME, initial_occupancy, availability);
		}
	}

	freelink();
	if(adjacency_matrix != NULL)
		free(adjacency_matrix);
	if(p_VertexArray != NULL)
		free(p_VertexArray);
	close(sock_UDP_server);
	close(sock_UDP_client);
	return 0;
}
