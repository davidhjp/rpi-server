#ifndef RPISERVER_H
#define RPISERVER_H
#include <pthread.h>
#include <stdint.h>

struct rpi_t {
	uint8_t group;
	uint8_t node;
	uint8_t type;
	uint16_t data;
};


/*
 * Starting a TCP server with port_server
 * This function is asynchronous (i.e. does not block)
 */
pthread_t ems_run_server(int port_server, void (*handler)(char *, int));

int ems_send(struct rpi_t *p);

int ems_send2(char* ip, int port, int group, int node, int type, int data);

int ems_destroy(pthread_t s);

#endif
