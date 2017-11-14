#include "server.h"
#include <pthread.h>
#include <stdio.h>
#include "log.h"
#include <check.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

int temp = 2;

void handler(char* packet, int size) {
	char msgs[50] = {'\0'};
	char msg[5];
	for(int i=0; i<size; i++){
		sprintf(msg, "%02X ", packet[i] & 0xff);
		strcat(msgs, msg);
	}
	log_info("received %s", msgs);
}


START_TEST(rpi_sending) {

}
END_TEST

START_TEST(rpi_receiving) {
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 200000000L;

	pthread_t t = run_server(2000, handler);
	nanosleep(&ts, NULL);

	test_send_packet(2000,10,11,12,1234);
	destroy(t);
//   pthread_join(t, NULL);
}
END_TEST

Suite *rpi_create(void) {
	Suite *s;
	TCase *tc_core;

	s = suite_create("RPi");

	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, rpi_receiving);
	tcase_add_test(tc_core, rpi_sending);

	tcase_set_timeout(tc_core, 100);
	suite_add_tcase(s, tc_core);

	return s;
}

int main() {
//   pthread_t t = run_server(2000);	
//   getchar();
//   struct rpiout_t p;
//   p.group	= 12;
//   p.node = 11;
//   p.type = 10;
//   p.data = 1234;
//   log_debug("## Sending packet");
//   send_packet(&p);
//   pthread_join(t, NULL);

	int number_failed;
	Suite *s;
	SRunner *sr;

	s = rpi_create();
	sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
