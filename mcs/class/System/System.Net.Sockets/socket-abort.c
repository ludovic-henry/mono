
int
create_pipe (int *pipe)
{
	struct sockaddr_in client;
	struct sockaddr_in server;
	int server_sock;
	int size;

	server_sock = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	assert (server_sock != -1);
	pipes [1] = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
	assert (pipe [1] != -1);

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr ("127.0.0.1");
	server.sin_port = 0;
	if (bind (server_sock, &server, sizeof (server)) == -1) {
		close (server_sock);
		printf ("wakeup_pipes_init: bind () failed, error (%d)\n", errno);
		return 1;
	}

	size = sizeof (server);
	if (getsockname (server_sock, (struct sockaddr*) &server, &size) == -1) {
		close (server_sock);
		printf ("wakeup_pipes_init: getsockname () failed, error (%d)\n", errno);
		return 1;
	}
	if (listen (server_sock, 1024) == -1) {
		close (server_sock);
		printf ("wakeup_pipes_init: listen () failed, error (%d)\n", errno);
		return 1;
	}
	if (connect (pipe [1], (struct sockaddr*) &server, sizeof (server)) == -1) {
		close (server_sock);
		printf ("wakeup_pipes_init: connect () failed, error (%d)\n", errno);
		return 1;
	}

	size = sizeof (client);
	pipe [0] = accept (server_sock, (struct sockaddr *) &client, &size);
	assert (pipe [0] != -1);

	close (server_sock);

	return 0;
}

int
main (void)
{

}