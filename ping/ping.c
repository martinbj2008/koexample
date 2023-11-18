#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

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

int main(int argc, char *argv[]) {
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
