#include <stdio.h>
#include "migrate_file.h"

#define CONTROLLEN  CMSG_LEN(sizeof(int))
#define MAXLINE		128

int send_fd(int fd, int fd_to_send)
{
	struct iovec    iov[1];
	struct msghdr   msg;
	char            buf[2];
	struct cmsghdr   *cmptr = NULL;

	iov[0].iov_base = buf;
	iov[0].iov_len  = 2;
	msg.msg_iov     = iov;
	msg.msg_iovlen  = 1;
	msg.msg_name    = NULL;
	msg.msg_namelen = 0;
	if (fd_to_send < 0) {
		msg.msg_control    = NULL;
		msg.msg_controllen = 0;
		buf[1] = -fd_to_send;
		if (buf[1] == 0)
			buf[1] = 1;
	} else {
		if (cmptr == NULL && (cmptr = malloc(CONTROLLEN)) == NULL)
			return(-1);
		cmptr->cmsg_level  = SOL_SOCKET;
		cmptr->cmsg_type   = SCM_RIGHTS;
		cmptr->cmsg_len    = CONTROLLEN;
		msg.msg_control    = cmptr;
		msg.msg_controllen = CONTROLLEN;
		*(int *)CMSG_DATA(cmptr) = fd_to_send;
		buf[1] = 0;
	}

	buf[0] = 0;
	if (sendmsg(fd, &msg, 0) != 2)
		return -1;
	return 0;
}

int recv_fd(int fd)
{
	int             newfd, nr, status;
	char            *ptr;
	char            buf[MAXLINE];
	struct iovec    iov[1];
	struct msghdr   msg;
	struct cmsghdr   *cmptr = NULL;      /* malloc'ed first time */

	cmptr = malloc(CONTROLLEN);
	if (cmptr == NULL)
		return(-1);

	newfd = -1;
	iov[0].iov_base = buf;
	iov[0].iov_len  = sizeof(buf);
	msg.msg_iov     = iov;
	msg.msg_iovlen  = 1;
	msg.msg_name    = NULL;
	msg.msg_namelen = 0;

	msg.msg_control    = cmptr;
	msg.msg_controllen = CONTROLLEN;
	if ((nr = recvmsg(fd, &msg, 0)) < 0) {
		printf("recvmsg error\n");
	} else if (nr == 0) {
		printf("connection closed by server\n");
		return(-1);
	}

	for (ptr = buf; ptr < &buf[nr]; ) {
		if (*ptr++ == 0) {
			if (ptr != &buf[nr-1])
				printf("message format error !!!\n");
			status = *ptr & 0xFF;  /* prevent sign extension */
			if (status == 0) {
				if (msg.msg_controllen != CONTROLLEN)
					printf("status = 0 but no fd !!!\n");
				newfd = *(int *)CMSG_DATA(cmptr);
			} else {
				newfd = -status;
			}
			nr -= 2;
		}
	}

	return newfd;
}
