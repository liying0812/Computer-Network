/*
 * COEN 146 Lab4
 * Name: Liying Liang
 * Lab: FTv2 UDP RDT 3.0 Client*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>
#include "tfv2.h"

int main(int argc, char *argv[]){
	int sock=0, nameBytes=0;
	int count=0;
	int chksum=0;//checksum
	int sequence=0; //sequence number
	char buff[10]; //hold data for each packet
	struct sockaddr_in serverAddr;
	socklen_t addr_size;

	srand(time(0)); //randomization

	if (argc != 5)
	{
		printf ("missing arguments\n");
		return 1;
	}

	//local variable needed
	struct timeval tv; //timer
	int rv; //select return value

	//setup for select, in the beginning of function
	fd_set readfds;
	fcntl(sock, F_SETFL, O_NONBLOCK);

	// initialize socket addresses
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons (atoi (argv[1]));
	inet_pton (AF_INET, argv[2], &serverAddr.sin_addr.s_addr);
	memset (serverAddr.sin_zero, '\0', sizeof (serverAddr.sin_zero));
	addr_size = sizeof serverAddr;

	//Create UDP socket
	if ((sock = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf ("socket error\n");
		return 1;
	}

	//open file
	FILE* fin=fopen(argv[3],"rb");
	if (fin==NULL){
		printf("Fail to open the file. \n");
		return 1;
	}

	//set output file Name
	nameBytes=strlen(argv[4])+1;
	//make PACKET for outupt file name
	PACKET namePacket;
	namePacket.header.seq_ack=sequence;
	namePacket.header.length=nameBytes;
	namePacket.header.checksum=0;
	strcpy(namePacket.data, argv[4]);

	int check=calc_checksum(&namePacket, sizeof(namePacket));
  namePacket.header.checksum=check;

	printf("calculate check: %d \n", check);

	printf("Start sending file name\n");
	while((sendto(sock, &namePacket, sizeof(namePacket), 0, (struct sockaddr *)&serverAddr, addr_size))<0){
		printf("file name fail to sent. \n");
	}
	printf("file name sent\n");

	//start before calling select
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);

	//setup for timer
	tv.tv_sec=10;
	tv.tv_usec=0;

	//call select function
	rv=select(sock+1, &readfds, NULL, NULL, &tv);

	if(rv<0){
			printf("select error\n");
			rv=select(sock+1, &readfds, NULL, NULL, &tv);
	}

	if(rv==0){ //timeout and no packet is received
		printf("Reach timeout and no ACK has received from server. \n");
		printf("Start retransmiision. \n");
		while ((sendto(sock, &namePacket, sizeof(namePacket), 0, (struct sockaddr *)&serverAddr, addr_size))<0){
			count++;
			if(count>=3){
				printf("Restransmission limit reached. \n");
				break;
			}
			printf("Retransmission failed...\n");
			tv.tv_sec=10;
			tv.tv_usec=0;
			//should it consider 3 times of Retransmission?
		}
		printf("Successful retransmission. \n");
		count=0;
	}
	else if (rv==1){ //ACK has been received before timeout

		//create packet to get ACK
		PACKET snamePacket;
		if((recvfrom (sock, &snamePacket, sizeof(snamePacket), 0, NULL, NULL))>0&&snamePacket.header.seq_ack==!sequence){
      	printf("Wrong file name packet. Starting retransmission...\n");

				//recalculate checksum
				int recal=calc_checksum(&namePacket, sizeof(namePacket));
				namePacket.header.checksum=recal;

				//send packet with updated checksum
      	while ((sendto(sock, &namePacket, sizeof(namePacket), 0, (struct sockaddr *)&serverAddr, addr_size))<0){
      	printf("Retransmission failed. Restarting...\n");
				count++;

				if(count>=3){
					printf("3 times retransmission reach. \n");
					break;
				}
			}

      printf("Successful retransmission. \n");
  	}
		printf("Correct ACK is received. Processing...\n");
	}
  sequence=!sequence;
	count=0;


	//start make packet from file
	while(!feof(fin)){
		//read from file to buff
		int total=fread(buff, 1, 10, fin);

		//make PACKET
		PACKET clientPacket;
		clientPacket.header.seq_ack=sequence;
		clientPacket.header.length=total;
		clientPacket.header.checksum=0;
		strcpy(clientPacket.data, buff);

		//calculate checksum with randomization, certain possibility of wrong checksum
		if(rand()%10<=6){
			chksum=calc_checksum(&clientPacket, sizeof(clientPacket));
			clientPacket.header.checksum=chksum;
		}
		printf("checksum:%d\n ",chksum);

		//sending packet
		printf("Start sending packet \n");
		while((sendto(sock, &clientPacket, sizeof(clientPacket), 0, (struct sockaddr *)&serverAddr, addr_size))<0){
			printf("Fail to send packet. Please resend. \n");
		}
		printf("Packet sent. \n");

		//start before calling select
		FD_ZERO(&readfds);
		FD_SET(sock, &readfds);

		//setup for timer
		tv.tv_sec=10;
		tv.tv_usec=0;

		//call select function
		rv=select(sock+1, &readfds, NULL, NULL, &tv);

		if(rv<0){
				printf("select error\n");
				rv=select(sock+1, &readfds, NULL, NULL, &tv);
		}
		if(rv==0){ //timeout and no packet is received
			printf("Reach timeout and no ACK has received from server. \n");
			printf("Start retransmiision. \n");
			while ((sendto(sock, &clientPacket, sizeof(clientPacket), 0, (struct sockaddr *)&serverAddr, addr_size))<0){
				count++;
				if(count>=3){
					printf("Restransmission limit reached. \n");
					break;
				}
				printf("Retransmission failed...\n");
				tv.tv_sec=10;
				tv.tv_usec=0;
				//should it consider 3 times of Retransmission?
			}
			printf("Successful retransmission. \n");
			count=0;
		}
		else if (rv==1){ //receive packet before timeout
			//make packet to receive ACK
			PACKET serverPacket;
			if((recvfrom (sock, &serverPacket, sizeof(serverPacket), 0, NULL, NULL))>0&&serverPacket.header.seq_ack==!sequence){
				printf("Wrong ACK. Starting retransmission...\n");

				//recaculate checksum
				int newcheck=calc_checksum(&clientPacket, sizeof(clientPacket));
        clientPacket.header.checksum=newcheck;

				//send packet with updated checksum
				while ((sendto(sock, &clientPacket, sizeof(clientPacket), 0, (struct sockaddr *)&serverAddr, addr_size))<0){
					printf("retransmission failed. Restarting...\n");
					count++;

					if(count>=3){
						printf("retranmission time limit reach.\n");
						break;
					}
				}
				printf("Successful retransmission. \n");
			}
			printf("Correct ACK is received. Processing...\n");
		}

		//update sequence number
		sequence=!sequence;
		count=0;
	}

	fclose(fin);

	//send empty packet to indicate end
	PACKET finalPacket;
	finalPacket.header.seq_ack=sequence;
	finalPacket.header.length=0;
	finalPacket.header.checksum=0;
	strcpy(finalPacket.data, " ");
	int finalcheck=calc_checksum(&finalPacket, sizeof(finalPacket));
	finalPacket.header.checksum=finalcheck;

	printf("Start sending final packet...\n");
	while((sendto(sock, &finalPacket, sizeof(finalPacket), 0, (struct sockaddr *)&serverAddr, addr_size))<0){
		printf("Fail to send final packet.");
	}
	printf("Final packet sent.\n");

	//start before calling select
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);

	//setup for timer
	tv.tv_sec=10;
	tv.tv_usec=0;

	//call select function
	rv=select(sock+1, &readfds, NULL, NULL, &tv);

	if(rv<0){
			printf("select error\n");
			rv=select(sock+1, &readfds, NULL, NULL, &tv);
	}

	if(rv==0){ //timeout and no packet is received
		printf("Reach timeout and no ACK has received from server. \n");
		printf("Start retransmiision. \n");
		while ((sendto(sock, &finalPacket, sizeof(finalPacket), 0, (struct sockaddr *)&serverAddr, addr_size))<0){
			printf("Retransmission failed...\n");
			count++;
			if(count>=3){
				printf("Restransmission limit reached. \n");
				break;
			}
			tv.tv_sec=1;
			tv.tv_usec=0;
			//should it consider 3 times of Retransmission?
		}
		printf("Successful retransmission. \n");
		count=0;
	}
	else if (rv==1){

		//packet to receive final packet ACK
		PACKET rFinalPacket;
		if((recvfrom (sock, &rFinalPacket, sizeof(rFinalPacket), 0, NULL, NULL))>0&&rFinalPacket.header.seq_ack==!sequence){
			printf("Wrong packet. Starting retransmission...\n");

			//recaculate checksum
			int newfinalCheck=calc_checksum(&finalPacket, sizeof(finalPacket));
			finalPacket.header.checksum=newfinalCheck;

			//send packet with updated checksum
			while ((sendto(sock, &finalPacket, sizeof(finalPacket), 0, (struct sockaddr *)&serverAddr, addr_size))<0){
				printf("retransmission failed. Restarting...\n");
				count++;

				if(count>=3){
					printf("retranmission time limit reach.\n");
					break;
				}
			}
			printf("Successful retransmission. \n");
		}
	}
	printf("Correct ACK has been received. Processing...\n");
	sequence=!sequence;
	count=0;

	return 0;


}
