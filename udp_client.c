/*************************************************
udp_client.c: the source file of the client in udp
Compiled by Lyz
*************************************************/

#include "headsock.h"

float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len);
void tv_sub(struct  timeval *out, struct timeval *in);

int main(int argc, char **argv) {
	FILE *fp;
	
	char ** pptr;
	int sockfd;
	float ti, rt;
	long len;
	struct hostent *sh;
	struct in_addr **addrs;
	struct sockaddr_in ser_addr;
	
	if (argc!= 2) {
		printf("no parameters entered/parameters not match.");
		exit(0);
	}
	
	// get host's information
	sh = gethostbyname(argv[1]);

	if (sh == NULL) {
		printf("error when gethostby name");
		exit(0);
	}
	
	// create socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd<0) {
		printf("error in socket");
		exit(1);
	}

	addrs = (struct in_addr **)sh->h_addr_list;
	printf("canonical name: %s\n", sh->h_name);
	for (pptr=sh->h_aliases; *pptr != NULL; pptr++)
		printf("the aliases name is: %s\n", *pptr);
	switch(sh->h_addrtype) {
		case AF_INET:
			printf("AF_INET\n");
		break;
		default:
			printf("unknown addrtype\n");
		break;
	}
        
	addrs = (struct in_addr **)sh->h_addr_list;
	ser_addr.sin_family = AF_INET;                                                      
	ser_addr.sin_port = htons(MYUDP_PORT);
	memcpy(&(ser_addr.sin_addr.s_addr), *addrs, sizeof(struct in_addr));
	bzero(&(ser_addr.sin_zero), 8);
	
	// open local file to read the data
	if ((fp = fopen ("myfile.txt","r+t")) == NULL) {
		printf("File doesn't exit\n");
		exit(0);
	}

	// perform the transmission and receiving
	ti = str_cli(fp, sockfd, (struct sockaddr *)&ser_addr, sizeof(struct sockaddr_in), &len);
	
	if (ti != -1)	{
		// caculate the average transmission rate
		rt = (len / (float) ti);
		printf("Ave Time(ms) : %.3f, Ave Data sent(byte): %d\nAve Data rate: %f (Kbytes/s)\n", ti, (int)len, rt);
	}
	
	close(sockfd);
	exit(0);
}

//packet transmission function
float str_cli(FILE *fp, int sockfd, struct sockaddr *addr, int addrlen, long *len) {
	char *buf;
	float time_inv = 0.0;
	int nAck = 0;
	int txSuccess = 0;
	int rxError = 0;
	int n;
	long lsize, ci;
	struct ack_so ack;
	struct pack_so sends;
	struct timeval recvt, sendt, tmo;
	
	ci = 0;
	
	sends.num = 0;
	sends.len = 0;
	sends.data[DATALEN];

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(0, &readfds);
	tmo.tv_sec = 3;
	int tries = 0;
	
	fseek(fp, 0, SEEK_END);
	lsize = ftell(fp);
	rewind(fp);
	printf("The file length is %d bytes\n", (int)lsize);
	printf("the packet length is %d bytes\n", DATALEN);

	// allocate memory to contain the whole file.
	buf = (char *) malloc (lsize);
	if (buf == NULL) exit (2);

	// copy the file into the buffer.
	fread(buf, 1, lsize, fp);

	/*** the whole file is loaded in the buffer. ***/
	// append the end byte
	buf[lsize] ='\0';
	// get the current time
	gettimeofday(&sendt, NULL);
	
	while(ci < lsize) {
		if ((lsize + 1 - ci) <= DATALEN) sends.len = lsize + 1 - ci;
		else sends.len = DATALEN;
		
		memcpy(sends.data, (buf + ci), sends.len);
		
		// STOP AND WAIT ARQ
		// Sender transmits a frame
		// send the packet to server
		n = sendto(sockfd, &sends.data, sends.len, 0, addr, addrlen);
		
		if (n == -1) {
			//send the data
			printf("send error!");
			exit(1);
		}
		
		// Sender waits for ACK before transmitting the next frame
		// receive the ack
		if ((n = recvfrom(sockfd, &ack, 2, 0, addr, &addrlen)) == -1) {
			printf("error when receiving\n");
			rxError += 1;
		} else if (ack.num != 1 || ack.len != 0) {
			printf("error in transmission\n");
			printf("*Resending packet number: %d\n", sends.num);
			//printf("activate TIMEOUT mechanism\n");
			nAck += 1;
		} else {
			ci += sends.len;
			sends.num += 1;
			printf("*Sending packet number: %d\n", sends.num);
			txSuccess += 1;
		}
	}
	
	gettimeofday(&recvt, NULL);
	// get current time
	*len = ci;
	// get the whole trans time
	tv_sub(&recvt, &sendt);
	time_inv += (recvt.tv_sec) * 1000.0 + (recvt.tv_usec) / 1000.0;
	
	printf("\n");
	printf("=======================\n");
	printf("Results of Transmission\n");
	printf("=======================\n");
	printf("total success(es): %d\n", txSuccess);
	//printf("total receive error(s): %d\n", rxError);
	printf("total nack(s): %d\n", nAck);
	return(time_inv);
}

// calcuates the time interval between out and in
void tv_sub(struct  timeval *out, struct timeval *in) {
	if ((out -> tv_usec -= in -> tv_usec) < 0) {
		--out -> tv_sec;
		out -> tv_usec += 1000000;
	}
	out -> tv_sec -= in -> tv_sec;
}
