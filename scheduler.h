#ifndef COMMON_PROJECT_H_
#define COMMON_PROJECT_H_

#define HOST_NAME "127.0.0.1"
#define SCHEDULER_TCP_PORT 34684
#define SCHEDULER_UDP_PORT 33684
#define HOSPITAL_UDP_PORT_A 30684
#define HOSPITAL_UDP_PORT_B 31684
#define HOSPITAL_UDP_PORT_C 32684
#define BUFF_SIZE 1024
#define INFO_SIZE 256
#define HOSPITAL_COUNT 3

typedef struct Edge_S{
	int vertex_L;
	int vertex_R;
	double distance;
	struct Edge_S* p_next;
}Edge;

typedef struct Vertex_S{
	int vertex_ID;
	struct Vertex_S* p_next;
}Vertex;

#endif /* COMMON_PROJECT_H_ */
