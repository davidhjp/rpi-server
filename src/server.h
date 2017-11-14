#ifndef RPISERVER_H
#define RPISERVER_H
#include <pthread.h>
#include <stdint.h>

struct rpiout_t {
	uint8_t group;
	uint8_t node;
	uint8_t type;
	uint16_t data;
};


/*
 * Starting a TCP server with port_server
 * This function is asynchronous (i.e. does not block)
 */
pthread_t run_server(int port_server, void (*handler)(char *, int));

int send_packet(struct rpiout_t *p);

int test_send_packet(int port, int group, int node, int type, int data);

int destroy(pthread_t s);

#endif
