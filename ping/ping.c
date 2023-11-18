#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h> // read(), write(), close()


unsigned int seq = 0;
char data[2000];
struct sockaddr_in addr;

unsigned short checksum(unsigned short *buf, int bufsz) {
	unsigned long sum = 0xffff;

	while(bufsz > 1) {
		sum += *buf;
		buf++;
		bufsz -= 2;
	}

	if(bufsz == 1)
		sum += *(unsigned char*)buf;

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);

	return ~sum;
}


int send_icmp(int sd, char *data, int len) {
	int ret;
	struct icmphdr *hdr;

	hdr = (struct icmphdr *)data;

	hdr->type = ICMP_ECHO;
	hdr->code = 0;
	hdr->checksum = 0;
	hdr->un.echo.id = htons(0xffff);
	hdr->un.echo.sequence = htons(seq++);

	hdr->checksum = checksum((unsigned short*)hdr, len);

	ret = sendto(sd, data, len, 0, (struct sockaddr*)&addr, sizeof(addr));
	if(ret < 1) {
		perror("sendto");
		exit(-1);
	}
	printf("We have sended an ICMP packet to\n");

	return 0;
}

int icmp_main() {
	int i;
	char j;
	int sd;
	int ret;
	char buf[1024];
	struct icmphdr *icmphdrptr;
	struct iphdr *iphdrptr;

	int len;

	memset(data, 1, 2000);

	addr.sin_family = PF_INET; // IPv4
	ret = inet_pton(PF_INET, "54.251.152.230", &addr.sin_addr);

	// 開一個 IPv4 的 RAW Socket , 並且準備收取 ICMP 封包
	sd = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(sd < 0) {
		perror("socket");
		exit(-1);
	}


	len = sizeof(struct icmphdr);
	j=0;
	for (i=len; i<2000; i++) {
		data[i] = j++;
	}
	len += 16;

	send_icmp(sd, data, len);	
	send_icmp(sd, data, len);	
	send_icmp(sd, data, len);	

	// 清空 buf
	memset(buf, 0, sizeof(buf));

	printf("Waiting for ICMP echo...\n");

	// 接收來自目標主機的 Echo Reply
	ret = recv(sd, buf, sizeof(buf), 0);
	if (ret < 1) {
		perror("recv");
		exit(-1);
	}

	// 取出 IP Header
	iphdrptr = (struct iphdr*)buf;

	// 取出 ICMP Header
	icmphdrptr = (struct icmphdr*)(buf+(iphdrptr->ihl)*4);

	// 判斷 ICMP 種類
	switch(icmphdrptr->type) {
		case 3:
			printf("The host is a unreachable purpose!\n");
			printf("The ICMP type is %d\n", icmphdrptr->type);
			printf("The ICMP code is %d\n", icmphdrptr->code);
			break;
		case 8:
		case 0:
			printf("The host is alive!\n");
			printf("The ICMP type is %d\n", icmphdrptr->type);
			printf("The ICMP code is %d\n", icmphdrptr->code);
			printf("The ICMP code is %d\n", htons(icmphdrptr->un.echo.id));
			printf("The ICMP code is %d\n", htons(icmphdrptr->un.echo.sequence));
			break;
		default:
			printf("Another situations!\n");
			printf("The ICMP type is %d\n", icmphdrptr->type);
			printf("The ICMP code is %d\n", icmphdrptr->code);
			break;
	}

	close(sd); // 關閉 socket
	return EXIT_SUCCESS;
}

#define MAX 80
#define PORT 8080
#define SA struct sockaddr

// Function designed for chat between client and server.
void func(int connfd)
{
    char buff[MAX];
    int n;
    // infinite loop for chat
    for (;;) {
        bzero(buff, MAX);

        // read the message from client and copy it in buffer
        read(connfd, buff, sizeof(buff));
        // print buffer which contains the client contents
        printf("From client: %s\t To client : ", buff);
        bzero(buff, MAX);
        n = 0;
        // copy server message in the buffer
        while ((buff[n++] = getchar()) != '\n')
            ;

        // and send that buffer to client
        write(connfd, buff, sizeof(buff));

        // if msg contains "Exit" then server exit and chat ended.
        if (strncmp("exit", buff, 4) == 0) {
            printf("Server Exit...\n");
            break;
        }
    }
}

// Driver function
int main()
{
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli;

	// socket create and verification
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		printf("socket creation failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully created..\n");
	bzero(&servaddr, sizeof(servaddr));

	// assign IP, PORT
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	// Binding newly created socket to given IP and verification
	if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
		printf("socket bind failed...\n");
		exit(0);
	}
	else
		printf("Socket successfully binded..\n");

	// Now server is ready to listen and verification
	if ((listen(sockfd, 5)) != 0) {
		printf("Listen failed...\n");
		exit(0);
	}
	else
		printf("Server listening..\n");
	len = sizeof(cli);

	// Accept the data packet from client and verification
	connfd = accept(sockfd, (SA*)&cli, &len);
	if (connfd < 0) {
		printf("server accept failed...\n");
		exit(0);
	}
	else
		printf("server accept the client...\n");

	// Function for chatting between client and server
	func(connfd);

	// After chatting close the socket
	close(sockfd);
}
