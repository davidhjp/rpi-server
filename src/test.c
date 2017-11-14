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
struct rpi_t data;

void handler(char* packet, int size) {
	char msgs[50] = {'\0'};
	char msg[5];
	for(int i=0; i<size; i++){
		sprintf(msg, "%02X ", packet[i] & 0xff);
		strcat(msgs, msg);
	}
	log_info("received %s", msgs);
	data.group = packet[2] & 0xff;
	data.node = packet[1] & 0xff;
	data.type = packet[4] & 0xff;
	data.data = ((packet[5] << 8) | (packet[6] & 0xff)) & 0xffff;
}


START_TEST(rpi_sending) {
	log_trace("=======  rpi_sending test =======");
	const int GROUP = 20;
	const int NODE  = 21;
	const int TYPE  = 22;
	const int DATA  = 2345;
	struct rpi_t d;
	d.group = GROUP;
	d.node = NODE;
	d.type = TYPE;
	d.data = DATA;
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 200000000L;


	pthread_t t = ems_run_server(2000, handler);
	nanosleep(&ts, NULL);

	// TODO: Create server and receive data

	// Packet is send from the RPi side
	ems_send(&d);

	// Check received data
}
END_TEST

START_TEST(rpi_receiving) {
	log_trace("=======  rpi_receiving test =======");
	const int GROUP = 10;
	const int NODE  = 11;
	const int TYPE  = 12;
	const int DATA  = 1234;
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = 200000000L;

	pthread_t t = ems_run_server(2000, handler);
	nanosleep(&ts, NULL);
	ems_send2("127.0.0.1",2000,10,11,12,1234);
	nanosleep(&ts, NULL);
	ems_destroy(t);
//   pthread_join(t, NULL)

	ck_assert_int_eq(data.group, GROUP);
	ck_assert_int_eq(data.node, NODE);
	ck_assert_int_eq(data.type, TYPE);
	ck_assert_int_eq(data.data, DATA);
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
