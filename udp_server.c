/**************************************************************
udp_server.c: the source file of the server in udp transmission
Compiled by Lyz
***************************************************************/

#include "headsock.h"

void str_ser(int sockfd, int errorRate);

void main(int argc, char *argv[]) {
	int sockfd, con_fd;
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	
	int errorRate;
	srand(time(NULL));
	
	if (argc!= 2) {
		printf("no error rate parameter entered - defaulting to 0\n");
		errorRate = 0;
	} else errorRate = atoi(argv[1]);
	
	printf("error rate parameter is %d\n\n", errorRate);
	
	// Step 1: Create a UDP socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("error in socket");
		exit(1);
	}
	
	// Initialize the socket address structure
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(MYUDP_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;	//inet_addr("172.0.0.1");
	bzero(&(my_addr.sin_zero), 8);
	
	// Step 2: Bind the socket
	if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) == -1) {
		printf("error in binding");
		exit(1);
	}
	
	// Step 3: Start receiving data
	printf("receiving start\n");
	while(1) str_ser(sockfd, errorRate);
	
	// Step 4: Close the socket
	close(sockfd);
	exit(0);
}

void str_ser(int sockfd, int errorRate) {
	FILE *fp;
	
	char buf[BUFSIZE];
	long lseek = 0;
	struct ack_so ack;
	struct pack_so recvs;
	struct sockaddr_in addr;
	int end = 0;
	int n = 0;
	int value = 0;
	int len = sizeof (struct sockaddr_in);
	
	ack.num = 0;
	ack.len = 0;
	recvs.num = 0;
	
	printf("waiting for data!\n");
    	while(!end) {
		// receive the packet
		if ((n = recvfrom(sockfd, &recvs.data, MAXSIZE, 0, (struct sockaddr *) &addr, &len)) == -1) {
			printf("receiving error!\n");
			return;
		} else {
			printf("%d data received\n", n);
			recvs.num += 1;
			printf("*Receiving/Expecting packet number: %d\n", recvs.num);
		}	
		
		// STOP AND WAIT ARQ
		// When a frame arrives, receiver sends ACK
		//ack.num = 1;
		
		if (rand() % 100 < errorRate) {
			ack.num = 0;
		} else ack.num = 1;
		
		//ack.num = rand() % 2;
		printf("*Sending ACK number: %d\n", ack.num);
		//ack.len = 0;
		
		if (ack.num == 1) {
		//if (ack.num == 1 || ack.len == 0) {
			memcpy((buf+lseek), recvs.data, n);
			lseek += n;
			
			// if it is the end of the file
			if (recvs.data[n-1] == '\0') {
				end = 1;
				n--;
			}		
		}
		
		// If received frame damaged/lost, receiver discards it
		// Transmitter uses TIMEOUT mechanism
		// If no ACK within timeout, retransmit
		
		// If ACK damaged/lost, transmitter will not recognize it
		// Transmitter will retransmit
		// Receiver gets two copies of the same frame
		// Alternate between ACK0 and ACK1 to solve the above problem
		if (ack.num != 1 || ack.len != 0) {
			printf("error in transmission\n");
			recvs.num -= 1;
		}
		sendto(sockfd, &ack, 2, 0, (struct sockaddr *) &addr, len);	
	}
	
	if((fp = fopen ("myUDPrecieve.txt","wt")) == NULL) {
		printf("File doesn't exist\n");
		exit(0);
	}
		
	printf("the file size received: %d\n", lseek);
	
	//write the data into file
	fwrite(buf, 1, lseek, fp);
	
	fclose(fp);
	printf("a file has been successfully received!\n");
	printf("\n");
}
