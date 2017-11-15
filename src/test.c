#include "server.h"
#include <pthread.h>
#include <stdio.h>
#include "log.h"
#include <check.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

struct rpi_t data;

long conv_ms_ns(int ms) {
	return ms * 1000000;
}

void handler_rec(char* packet, int size) {
	char msgs[200] = {'\0'};
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

void handler_send(char* packet, int size) {
	char msgs[200] = {'\0'};
	char msg[5];
	for(int i=0; i<size; i++){
		sprintf(msg, "%02X ", packet[i] & 0xff);
		strcat(msgs, msg);
	}
	log_info("received %s", msgs);
	data.group = packet[9] & 0xff;
	data.node = packet[10] & 0xff;
	data.type = packet[11] & 0xff;
	data.data = ((packet[12] << 8) | (packet[13] & 0xff)) & 0xffff;
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
	ts.tv_nsec = conv_ms_ns(200);

	pthread_t t = ems_run_server(2000, handler_rec);
	nanosleep(&ts, NULL);

	// TODO: Create a client and receive data
	pthread_t tc = ems_test_run_client("127.0.0.1", 2000, handler_send);
	ck_assert(tc != NULL);
	nanosleep(&ts, NULL);

	// Packet is sent from the RPi side
	ck_assert(ems_send(&d) > 0);

	int *r;
	pthread_join(tc, (void**)&r);
	ck_assert(r == 0);

	// Check received data
	ck_assert_int_eq(data.group, GROUP);
	ck_assert_int_eq(data.node, NODE);
	ck_assert_int_eq(data.type, TYPE);
	ck_assert_int_eq(data.data, DATA);

	ck_assert(ems_destroy(t) == 0);
	pthread_join(t, NULL);
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
	ts.tv_nsec = conv_ms_ns(200);

	pthread_t t = ems_run_server(2000, handler_rec);
	nanosleep(&ts, NULL);
	int sock = ems_send2("127.0.0.1",2000,10,11,12,1234);
	ck_assert(sock > 0);
	ts.tv_nsec = conv_ms_ns(300); // Just to make sure that the server gets the value before closing the socket
	nanosleep(&ts, NULL);
	close(sock);
	ems_destroy(t);
	pthread_join(t, NULL);

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
