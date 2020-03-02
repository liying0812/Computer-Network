/*
 * COEN 146 Lab3
 * Name: Liying Liang
 * Lab: FTv2 UDP RDT 2.2 Client*/

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
#include "tfv2.h"

int main(int argc, char *argv[]){
	int sock=0, nameBytes=0;
	int count=0;
	int chksum=0;//checksum
	int sequence=0; //sequence number
	char buff[10]; //hold data for each packet
	struct sockaddr_in serverAddr;
	socklen_t addr_size;

	srand(time(0)); 

	if (argc != 5)
	{
		printf ("missing arguments\n");
		return 1;
	}


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

	//send output file Name
	nameBytes=strlen(argv[4])+1;
	//make PACKET
	PACKET namePacket;
	namePacket.header.seq_ack=sequence;
	namePacket.header.length=nameBytes;
	namePacket.header.checksum=0;
	strcpy(namePacket.data, argv[4]); 

	int check=calc_checksum(&namePacket, sizeof(namePacket));
        namePacket.header.checksum=check; 

	printf("calculate check: %d \n", check); 

	printf("Start sending file name\n");
	while((sendto(sock, &namePacket, sizeof(namePacket), 0, (struct sockaddr *)&serverAddr, addr_size))==-1){
		printf("file name fail to sent. \n");
	}
	printf("file name sent\n");
	
	PACKET snamePacket;
	while((recvfrom (sock, &snamePacket, sizeof(snamePacket), 0, NULL, NULL))>0&&snamePacket.header.seq_ack==!sequence){
                        printf("Wrong file name packet. Starting retransmission...\n");
			int recal=calc_checksum(&namePacket, sizeof(namePacket));
			namePacket.header.checksum=recal; 
                        while ((sendto(sock, &namePacket, sizeof(namePacket), 0, (struct sockaddr *)&serverAddr, addr_size))==-1){
                                printf("retransmission failed. Restarting...\n")
;
				count++;
				if(count>=3){
					printf("3 times retransmission reach");
					break; 
				}
                        }
                        printf("Successful retransmission. \n");
                }

        printf("Packet name successully sent. Processing...\n");
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

		//calculate checksum
		if(rand()%10<=6){
		chksum=calc_checksum(&clientPacket, sizeof(clientPacket));
		clientPacket.header.checksum=chksum;
		}
		printf("checksum:%d\n ",chksum);	

		//sending packet
		printf("Start sending packet \n");
		while((sendto(sock, &clientPacket, sizeof(clientPacket), 0, (struct sockaddr *)&serverAddr, addr_size))==-1){
			printf("Fail to send packet. Please resend. \n");
		}
		printf("Packet sent. \n");

		//receive ACK
		PACKET serverPacket;
		while((recvfrom (sock, &serverPacket, sizeof(serverPacket), 0, NULL, NULL))>0&&serverPacket.header.seq_ack==!sequence){
			printf("Wrong packet. Starting retransmission...\n");
			int newcal=calc_checksum(&clientPacket, sizeof(clientPacket));
                        clientPacket.header.checksum=newcal;
			while ((sendto(sock, &clientPacket, sizeof(clientPacket), 0, (struct sockaddr *)&serverAddr, addr_size))==-1){
				printf("retransmission failed. Restarting...\n")
;
				count++;
				if(count>=3){
					printf("retranmission time limit reach.\n");
					break; 
				}
			}
			printf("Successful retransmission. \n");
		}

		printf("Packet successully sent. Processing...\n");
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
	if((sendto(sock, &finalPacket, sizeof(finalPacket), 0, (struct sockaddr *)&serverAddr, addr_size))==-1){
		printf("Fail to send final packet.");
	}
	
	printf("final packet sent.\n"); 
	return 0;


}
