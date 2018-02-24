#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#define TCP_SERVER	"127.0.0.1"
#define TCP_PORT	9999

#define UNIX_SOCK_PATH "channel_to_migrate_socket"

/* size of control buffer to send/recv one file descriptor */
#define CONTROLLEN  CMSG_LEN(sizeof(int))

int tcp_client()
{
	int sockfd, portno, n;
	struct sockaddr_in serveraddr;
	struct hostent *server;
	char *hostname;
	char buf[] = "Test message from tcp client to python server!!!";

	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		goto fail;

	/* gethostbyname: get the server's DNS entry */
	server = gethostbyname(TCP_SERVER);
	if (server == NULL) {
		fprintf(stderr,"ERROR, no such host as %s\n", hostname);
		goto fail;
	}

	/* build the server's Internet address */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr,
			(char *)&serveraddr.sin_addr.s_addr, server->h_length);
	serveraddr.sin_port = htons(TCP_PORT);

	/* connect: create a connection with the server */
	if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
		goto connect_fail;

	/* send the message line to the server */
	n = write(sockfd, buf, strlen(buf));
	if (n < 0)
		goto connect_fail;	// write fail is same as connect fail.

	return sockfd;

connect_fail:
	close(sockfd);
fail:
	return -1;
}

int create_channel()
{
	int ret;
	int sock;
	struct sockaddr_un server;
	char buf[1024];

	//create unix socket channel to send fd.
	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock < 0)
		goto fail;
	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, UNIX_SOCK_PATH);

	if (connect(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0)
		goto fail;
	return sock;

fail:
	close(sock);
	return -1;
}

int main()
{
	int ret;
	int channel_fd;
	int tcp_sock_fd;

	tcp_sock_fd = tcp_client();
	channel_fd = create_channel();
	if (tcp_sock_fd <= 0 || channel_fd <= 0)
		goto fin;

	ret = send_fd(channel_fd, tcp_sock_fd);
	printf("tcp socket (fd: %d) has benn sent to perr %s !\n",
		tcp_sock_fd, ret == 0 ? "Success" : "Failed" );

fin:
	if (tcp_sock_fd > 0)
		close(tcp_sock_fd);
	if (channel_fd > 0)
		close(channel_fd);
	return 0;
}
