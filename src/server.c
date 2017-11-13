#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_SOCKS 300

typedef struct sock_addr {
	struct sockaddr_in addr;
	int socket;
	int idx;
} SocketInfo;

static int skt_ptr = 0;
static int socket_desc;
static SocketInfo* sockets[MAX_SOCKS];
static pthread_mutex_t lock;


void remove_from_buffer(SocketInfo *info) {
	if(skt_ptr > 0){
		sockets[info->idx] = sockets[--skt_ptr];
		sockets[info->idx]->idx = skt_ptr;
	}
	else
		sockets[0] = 0;
	printf("removed socket %d from buffer index %d\n", info->socket, info->idx);
}

void* worker(void* arg) {
	char magic[1];
	int read_size;
	SocketInfo* info = (SocketInfo*)arg;
	printf("running th %d\n", pthread_self());
	while(1){
		read_size = recv(info->socket, magic, 1, MSG_WAITALL);

		if(read_size == 0) {
			printf("socket closed %d\n");
			int r = close(info->socket);
			if(r != 0)
				puts("error while closing socket");

			remove_from_buffer(info);
			return NULL;
		}

		if(magic[0] != 0xBB) {
			magic[0] = 0;
			puts("magic is wrong");
			continue;
		}
		printf("read %02X\n", magic[0]);
		printf("%d" ,magic[0] == 0xBB);
	}

	return NULL;
}

void run_server(int port_server) {
	int client_sock, c, read_size;
	struct sockaddr_in server, client;
	SocketInfo *info;
	pthread_t skt_thread;
	
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if(socket_desc == -1) {
		perror("bind failed");
		exit(1);
	}
	puts("Socket created");

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port_server);

	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) {
		perror("bind failed. Error");
		exit(1);
	}
	puts("bind done");

	listen(socket_desc, 3);

	c = sizeof(struct sockaddr_in);

	while(1){
		puts("waiting for an incoming connection");
		client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);

		if (client_sock < 0)
			break;
		info = (SocketInfo*)malloc(sizeof(SocketInfo));
		info->addr = client;
		info->socket = client_sock;
		info->idx = skt_ptr;
		sockets[skt_ptr++] = info;
		if(pthread_create(&skt_thread, NULL, worker, (void*)info)) {
			puts("error on creaing a thread");
			exit(1);
		}
		printf("Connection accepted: socket: %d, thread id %d\n", client_sock, skt_thread);
	}
	puts("shutdown server socket");
	close(socket_desc);

}

void shutdown_server() {
	int s = close(socket_desc);
	if(s >= 0)
		puts("closed serversocket successfully");
	else
		puts("error while closing serversocket");
}

