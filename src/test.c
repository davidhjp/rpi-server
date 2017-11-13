#include "server.h"
#include <pthread.h>
#include <stdio.h>
#include "log.h"

int main() {
	pthread_t t = run_server(2000);	
	getchar();
	struct rpiout_t p;
	p.group	= 12;
	p.node = 11;
	p.type = 10;
	p.data = 1234;
	log_debug("## Sending packet");
	send_packet(&p);
	pthread_join(t, NULL);
	return 0;
}
