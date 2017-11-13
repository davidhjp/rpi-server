#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_SOCKS 300

typedef struct rpi_server {
	int sock;
	pthread_t clients[MAX_SOCKS];
	int hp;
	pthread_mutex_t lock;
} server_t;

typedef struct rpi_worker {
	int sock;
	server_t *server;
} worker_t;


void remove_client(worker_t *w) {
	pthread_t thnum = pthread_self();
	server_t *s = w->server;
	int indx = 0;
	while(s->clients[indx] != thnum) {
		indx++;
	}
	if(s->hp > 0){
		s->clients[indx] = s->clients[--(s->hp)];
		printf("removed thread clients[%d]\n", indx);
	}
}

void add_client(server_t *s, pthread_t th) {
	s->clients[s->hp++] = th;
}

void* worker(void* arg) {
	char magic[1];
	int read_size;
	worker_t *w = (worker_t*)arg;
	printf("running th %d\n", pthread_self());

	while(1){
		read_size = recv(w->sock, magic, 1, MSG_WAITALL);

		if(read_size == 0) {
			printf("socket closed %d\n", w->sock);
			int r = close(w->sock);
			if(r != 0)
				puts("error while closing socket");
			remove_client(w);
			free(w);
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

	close(w->sock);
	free(w);
	return NULL;
}


void* internal_run_server(void* arg) {
	int client_sock, c;
	struct sockaddr_in client;
	pthread_t th_client;
	server_t *serv;
	worker_t *worker_data;

	serv = (server_t *)arg;

	c = sizeof(struct sockaddr_in);
	while(1){
		puts("waiting for an incoming connection");
		client_sock = accept(serv->sock, (struct sockaddr *)&client, (socklen_t*)&c);

		if (client_sock < 0)
			break;
		if(serv->hp < MAX_SOCKS){
			worker_data = (worker_t*)malloc(sizeof(worker_t));
			worker_data->sock = client_sock;
			worker_data->server = serv;
			if(pthread_create(&th_client, NULL, worker, (void*)worker_data)) {
				puts("error on creaing a thread");
				free(worker_data);
				exit(1);
			}
			add_client(serv, th_client);
			printf("Connection accepted: socket: %d, thread id %d\n", client_sock, th_client);
		} else {
			printf("Maximum number of sockets created: %d \n", MAX_SOCKS);
			close(client_sock);
		}
	}
	puts("shutdown server socket");
	close(serv->sock);
}

pthread_t run_server(int port_server) {
	int sock_server;
	struct sockaddr_in server_addr;
	pthread_t pth_server;
	server_t *serv;

	sock_server = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_server == -1) {
		perror("bind failed");
		exit(1);
	}
	puts("Server Socket created");

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port_server);

	if( bind(sock_server,(struct sockaddr *)&server_addr , sizeof(server_addr)) < 0) {
		perror("bind failed. Error");
		exit(1);
	}
	puts("bind done");
	listen(sock_server, 3);
	serv = (server_t*)malloc(sizeof(server_t));
	serv->sock = sock_server;
	pthread_create(&pth_server, NULL, internal_run_server, (void*)serv);
	return pth_server;
}


