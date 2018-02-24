#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define CHANNEL_NAME "channel_to_migrate_socket"

void test_tcp_sock(int fd)
{
	int i;
	int ret;
	struct sockaddr_in peer;
	int peer_len;
	char *buf = "Migrate to new Process";

	peer_len = sizeof(peer);
	if (getpeername(fd, (struct sockaddr *)&peer, &peer_len) < 0) {
		perror("getpeername() failed");
		return;
	}
	printf("Peer's IP address is: %s:%d\n",
		inet_ntoa(peer.sin_addr),  (int) ntohs(peer.sin_port));

	ret = 0;
	for (i=0; i<10; i++) {
		ret = write(fd, buf, strlen(buf));
		printf("%s: write %d bytes OK.\n", __func__, ret);
		sleep(1);
	}

	return;
}

int main()
{
	int tcp_sk_fd;
	int sock, msgsock, rval;
	struct sockaddr_un server;
	char buf[1024];

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
		goto fail;

	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, CHANNEL_NAME);
	if (bind(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0)
		goto bind_fail;
	printf("Socket has CHANNEL_NAME %s\n", server.sun_path);

	listen(sock, 5);
	tcp_sk_fd = -1;

	//example: Only get on fd.
	while (tcp_sk_fd < 0) {
		msgsock = accept(sock, 0, 0);
		if (msgsock == -1)
			perror("accept");
		else
			tcp_sk_fd = recv_fd(msgsock);
		close(msgsock);
	}

	test_tcp_sock(tcp_sk_fd);

	close(sock);
	unlink(CHANNEL_NAME);
	close(tcp_sk_fd);
	return 0;

bind_fail:
	unlink(CHANNEL_NAME);
fail:
	close(sock);
	return -1;
}
