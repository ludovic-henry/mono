
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <sys/socket.h>

static void*
recv_thread (void *param)
{
	int socket = (int) param;
	char buffer [128];

	printf ("before recv\n");
	recv (socket, &buffer, sizeof (buffer), 0);
	printf ("after  recv\n");

	return NULL;
}

static void*
accept_thread (void *param)
{
	int socket = (int) param;
	struct sockaddr_in client;
	socklen_t size = sizeof (struct sockaddr_in);

	printf ("before accept\n");
	accept (socket, (struct sockaddr *) &client, &size);
	printf ("after  accept\n");

	return NULL;
}

int main (int argc, char **argv)
{
	int ret;
	int fd = -1;
	pthread_t t;

	assert (argc == 3);

	switch (atoi (argv [1])) {
	case 1: {
		int fd_wr;
		int fd_server;
		struct sockaddr_in client;
		struct sockaddr_in server;
		socklen_t size;

		fd_server = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
		assert (fd_server != -1);

		fd_wr = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
		assert (fd_wr != -1);

		server.sin_family = AF_INET;
		server.sin_addr.s_addr = inet_addr ("127.0.0.1");
		server.sin_port = 0;
		ret = bind (fd_server, (struct sockaddr*) &server, sizeof (server));
		assert (ret != -1);

		size = sizeof (server);
		ret = getsockname (fd_server, (struct sockaddr*) &server, &size);
		assert (ret != -1);

		ret = listen (fd_server, 1024);
		assert (ret != -1);

		ret = connect (fd_wr, (struct sockaddr*) &server, sizeof (server));
		assert (ret != -1);

		size = sizeof (client);
		fd = accept (fd_server, (struct sockaddr *) &client, &size);
		assert (fd != -1);

		close (fd_server);

		ret = pthread_create (&t, NULL, &recv_thread, (void*)(size_t) fd);
		assert (ret != -1);

		break;
	}
	case 2: {
		struct sockaddr_in server;

		fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
		assert (fd != -1);

		server.sin_family = AF_INET;
		server.sin_addr.s_addr = inet_addr ("127.0.0.1");
		server.sin_port = 0;
		ret = bind (fd, (struct sockaddr*) &server, sizeof (server));
		assert (ret != -1);

		ret = listen (fd, 1024);
		assert (ret != -1);

		ret = pthread_create (&t, NULL, &accept_thread, (void*)(size_t) fd);
		assert (ret != -1);

		break;
	}
	default:
		assert (!"invalid argv[1]");
	}

	assert (fd >= 0);

	/* wait for thread to start */
	sleep (1);

	switch (atoi (argv [2])) {
	case 1: {
		printf ("before shutdown\n");

		fcntl (fd, F_SETFL, O_NONBLOCK);
		shutdown (fd, 2);

		sleep (1);

		printf ("after  shutdown\n");

		break;
	}
	case 2: {
		printf ("before close\n");

		close (fd);

		sleep (1);

		printf ("after  close\n");

		break;
	}
	default:
		assert (!"invalid argv[2]");
	}

	pthread_join (t, NULL);

	return 0;
}

