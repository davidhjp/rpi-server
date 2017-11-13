#include "server.h"
#include <pthread.h>

int main() {
	pthread_t t = run_server(2000);	
	pthread_join(t, NULL);
	return 0;
}
