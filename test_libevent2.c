
#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "unistd.h"
#include "inttypes.h"

#include "zmq.h"

#include "event2/event.h"

struct ctx_s {
	struct event *read_event;
	void *zmq_ctx;
	void *zmq_s;
};

static uint64_t entered_callback = 0;
static uint64_t received_messages = 0;

static void read_callback(int fd, short event, void *arg)
{
	struct ctx_s *ctx = arg;

	entered_callback++;

	char buf[5];

	for (;;) {
		int rv = zmq_recv(ctx->zmq_s, buf, 5, ZMQ_DONTWAIT);

		if (-1 == rv) {
			if (errno == EAGAIN) {
				break;
			}

			perror("zmq_recv");
			exit(1);
		}

		received_messages++;
	}
}

static void *thread_one_fn(void *arg)
{
	void *zmq_ctx = arg;

	void *zmq_s = zmq_socket(zmq_ctx, ZMQ_PAIR);
	if (!zmq_s) {
		perror("zmq_socket()");
		exit(1);
	}

	if (-1 == zmq_bind(zmq_s, "inproc://foobar")) {
		perror("zmq_bind");
		exit(1);
	}

	int zmq_fd;
	size_t zmq_fd_len = sizeof(zmq_fd);

	if (-1 == zmq_getsockopt(zmq_s, ZMQ_FD, &zmq_fd, &zmq_fd_len)) {
		perror("zmq_getsockopt");
		exit(1);
	}

	struct ctx_s ctx;
	struct event_base *eb = event_base_new();
	struct event *read_event = event_new(eb, zmq_fd, EV_READ | EV_PERSIST, read_callback, &ctx);
	event_add(read_event, 0);

	ctx.read_event = read_event;
	ctx.zmq_ctx = zmq_ctx;
	ctx.zmq_s = zmq_s;


	event_base_loop(eb, 0);

	return 0;
}

static void *thread_two_fn(void *arg)
{
	void *zmq_ctx = arg;

	void *zmq_s = zmq_socket(zmq_ctx, ZMQ_PAIR);
	if (!zmq_s) {
		perror("zmq_socket()");
		exit(1);
	}

	if (-1 == zmq_connect(zmq_s, "inproc://foobar")) {
		perror("zmq_bind");
		exit(1);
	}

	for (;;) {
		if (-1 == zmq_send(zmq_s, "marko", 5, 0)) {
			perror("zmq_bind");
			exit(1);
		}
	}
}

int main()
{

	void *zmq_ctx = zmq_ctx_new();

	pthread_t thread_one;
	pthread_t thread_two;

	if (0 != pthread_create(&thread_one, 0, thread_one_fn, zmq_ctx)) {
		fprintf(stderr, "error while creating first thread\n");
		exit(1);
	}

	if (0 != pthread_create(&thread_two, 0, thread_two_fn, zmq_ctx)) {
		fprintf(stderr, "error while creating second thread\n");
		exit(1);
	}

	uint64_t saved_messages = 0;
	uint64_t saved_callbacks = 0;

	for (;;) {

		printf("received messages: %"PRIu64" (%f msg/sec)\n",
				received_messages,
				(double)(received_messages-saved_messages));
		printf("entered read callback: %"PRIu64" (%f ent/sec)\n",
				entered_callback,
		      		(double)(entered_callback-saved_callbacks));

		saved_messages = received_messages;
		saved_callbacks = entered_callback;

		sleep(1);
	}

	zmq_ctx_destroy(zmq_ctx);

	return 0;
}

