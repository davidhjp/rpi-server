#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
// #include <stdint.h>
#include "log.h"
#include "server.h"

#define MAX_SOCKS 300
#define MAX_SERVERS 50

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_WARN
#endif

#define DATA_SIZE  2
#define FT_LEN     3
#define BASE_LEN   10
#define RP_LEN     BASE_LEN + DATA_SIZE + FT_LEN

static pthread_mutex_t log_lock;

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

struct server_array {
	server_t *l[MAX_SERVERS];
	int p;
};

struct server_array serv_list = { .p = 0 };


void remove_client(worker_t *w) {
	pthread_t thnum = pthread_self();
	server_t *s = w->server;
	int indx = 0;
	while(s->clients[indx] != thnum) {
		indx++;
	}

	pthread_mutex_lock(&s->lock);
	if(s->hp > 0){
		s->clients[indx] = s->clients[--(s->hp)];
		log_info("removed thread clients[%d]\n", indx);
	}
	pthread_mutex_unlock(&s->lock);
}

void add_client(server_t *s, pthread_t th) {
	pthread_mutex_lock(&s->lock);

	s->clients[s->hp++] = th;

	pthread_mutex_unlock(&s->lock);
}

void add_server(server_t *serv) {
	pthread_mutex_lock(&log_lock);

	serv_list.l[serv_list.p++] = serv;

	pthread_mutex_unlock(&log_lock);
}

/*
 * This is where the packets can be read
 */
void parse_incoming_packet(char* b1, int sizeofb1, char* b2, int sizeofb2) {
	char str[1000];
	char msg[10];
	log_debug("received magic size: %d", sizeofb1);
	sprintf(str, "Magic bytes: ");
	for(int i=0;i<sizeofb1 ; i++){
		sprintf(msg, "%02X \0", b1[i] & 0xff);
		strcat(str, msg);
	}
	log_debug("%s", str);
	log_debug("received data size: %d", sizeofb2);
	sprintf(str, "Data bytes: ");
	for(int i=0;i<sizeofb2 ; i++){
		sprintf(msg, "%02X \0", b2[i] & 0xff);
		strcat(str, msg);
	}
	log_debug("%s", str);
}

int check_read(int read_size, worker_t *w) {
	if(read_size == 0) {
		log_info("socket closed %d", w->sock);
		int r = close(w->sock);
		if(r != 0)
			log_warn("error while closing socket");
		remove_client(w);
		free(w);
		return 0;
	}
	return 1;
}

void* worker(void* arg) {
	char *data;
	char magic[4];
	int read_size;
	worker_t *w = (worker_t*)arg;
	log_debug("running th %d", pthread_self());

	while(1){
		read_size = recv(w->sock, magic, 4, MSG_WAITALL);

		if(!check_read(read_size, w))
			return NULL;

		if((magic[0] & 0xff) != 0xBB) {
			magic[0] = 0;
			log_debug("magic is wrong");
			continue;
		}

		int packet_size = magic[3];
		int data_size = sizeof(char) * packet_size;
		data = (char*)malloc(sizeof(char) * packet_size);
		read_size = recv(w->sock, data, packet_size, MSG_WAITALL);

		if(!check_read(read_size, w))
			return NULL;
		
		parse_incoming_packet(magic, 4, data, data_size);
		free(data);
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
		log_debug("waiting for an incoming connection");
		client_sock = accept(serv->sock, (struct sockaddr *)&client, (socklen_t*)&c);

		if (client_sock < 0)
			break;

		if(serv->hp < MAX_SOCKS){
			worker_data = (worker_t*)malloc(sizeof(worker_t));
			worker_data->sock = client_sock;
			worker_data->server = serv;
			if(pthread_create(&th_client, NULL, worker, (void*)worker_data)) {
				log_warn("error on creaing a thread");
				free(worker_data);
				exit(1);
			}
			add_client(serv, th_client);
			log_info("Connection accepted: socket: %d, thread id %d", client_sock, th_client);
		} else {
			printf("Maximum number of sockets created: %d \n", MAX_SOCKS);
			close(client_sock);
		}
	}
	log_info("shutdown server socket");
	close(serv->sock);
}

void set_lock(void *udata, int lock) {
	pthread_mutex_t *mutex = (pthread_mutex_t*)udata;
	if(lock == 1)
		pthread_mutex_lock(mutex);
	else
		pthread_mutex_unlock(mutex);
		
}

pthread_t run_server(int port_server) {
	log_set_level(LOG_LEVEL);
	int sock_server;
	struct sockaddr_in server_addr;
	pthread_t pth_server;
	server_t *serv = (server_t*)malloc(sizeof(server_t));

	add_server(serv);

	if(pthread_mutex_init(&log_lock, NULL) != 0) {
		log_error("Mutex log_lock init failed");
		exit(1);
	}

	log_set_udata(&log_lock);
	log_set_lock(set_lock);

	sock_server = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_server == -1) {
		log_error("bind failed");
		exit(1);
	}
	log_debug("Server Socket created");

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port_server);

	if( bind(sock_server,(struct sockaddr *)&server_addr , sizeof(server_addr)) < 0) {
		log_error("bind failed. Error");
		exit(1);
	}
	log_debug("bind done");
	listen(sock_server, 3);
	serv->hp = 0;
	serv->sock = sock_server;
	pthread_create(&pth_server, NULL, internal_run_server, (void*)serv);
	return pth_server;
}

void send_packet(struct rpiout_t *p) {
	char packet[RP_LEN];
	packet[0] = 0xAA;
	packet[1] = RP_LEN;
	packet[9] = p->group & 0xff;
	packet[10] = p->node & 0xff;
	packet[11] = p->type & 0xff;
	packet[12] = (p->data & 0xffff) >> 8;
	packet[13] = (p->data & 0xff);

	int s_size = serv_list.p;
	log_debug("send_packet: server #: %d", s_size);
	for(int i=0; i<s_size; i++) {
		server_t *serv = serv_list.l[i];
		log_debug("For server running socket %d", serv->sock);
		for(int j=0; j<serv->hp; j++){
			log_debug("Client %d with fd %d", j, serv->clients[j]);
			// TODO: send data over TCP socket using serv->clients[]
		}
	}


}

