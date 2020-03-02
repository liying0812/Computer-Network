/*
 * COEN 146 Lab4
 * Name: Liying Liang
 * Lab: FTV2 UDP RDT 3.0 server*/

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "tfv2.h"

int main(int argc, char *argv[]){
	//int state=0;
	int chksum;
	int n;
	int sock, nbytes;
	char buff[10]; //hold receiving data
	int sequence=0;
	struct sockaddr_in serv_Addr, clientAddr;
	struct sockaddr_storage serverStorage;
	socklen_t addr_size, client_addr_size;

	srand(time(0)); //randomization

	if (argc!=2){
		printf("need the port number\n");
		return 1;
	}

	//initialize socket
	serv_Addr.sin_family=AF_INET;
	serv_Addr.sin_port=htons((short)atoi (argv[1]));
	serv_Addr.sin_addr.s_addr=htonl(INADDR_ANY);
	memset((char *)serv_Addr.sin_zero, '\0', sizeof(serv_Addr.sin_zero));
	addr_size=sizeof(serverStorage);

	//create socket
	if ((sock=socket(AF_INET, SOCK_DGRAM, 0))<0){
		printf("socket error\n");
		return 1;
	}

	//bind socket
	if (bind(sock,(struct sockaddr *)&serv_Addr, sizeof(serv_Addr))!=0){
		printf("bind error\n");
		return 1;
	}

	printf("waiting for file name. \n");

	//receive file name
	FILE* fout;

	PACKET namePacket;
	while ((recvfrom (sock, &namePacket, sizeof(namePacket), 0, (struct sockaddr *)&serverStorage, &addr_size))<0){
		printf("Fail to received file name packet. \n");
	}
	printf("File name packet received. \n");

	//printf("packet checksum %d\n", namePacket.header.checksum);
	//calculate checksum
	int tempCK=namePacket.header.checksum;
	namePacket.header.checksum=0;
	int realCK=calc_checksum(&namePacket, sizeof(namePacket));
	//printf("calculate checksum:%d\n", realCK);

	if(tempCK==realCK){//checksum matched
		printf("filename checksum received: %d \n", tempCK);
		printf("filename cal ck: %d \n", realCK);
		printf("Checksum matached. Correct file name packet received, extracting data. ");
		//extract data from the packet
		strcpy(buff, namePacket.data);
		//write data to file
		//fwrite(buff, 1, namePacket.header.length, fout);

		fout=fopen(buff, "wb");
		if(fout==NULL){
			printf("Fail to open file\n");
			return 1;
		}

		//create new packet with updated ACK
		PACKET snamePacket;
		snamePacket.header.seq_ack=namePacket.header.seq_ack;
		snamePacket.header.length=0;
		snamePacket.header.checksum=0;

		//send packet back
		while((sendto(sock, &snamePacket, sizeof(snamePacket), 0, (struct sockaddr *)&serverStorage, addr_size))<0){
			printf("Failed to send file name packet ACK. Retransmission. \n");
		}
		printf("File name packet ACK. Processing. \n");
		sequence=!sequence;
	 }

	 else{
		printf("filename checksum received: %d \n", tempCK);
    printf("filename cal ck: %d \n", realCK);
		printf("Checksum mismatached. Wrong file name packet received.\n");

		PACKET snamePacket;
		  snamePacket.header.seq_ack=!namePacket.header.seq_ack;
		  snamePacket.header.length=0;
		  snamePacket.header.checksum=0;
			strcpy(snamePacket.data, " ");
			while((sendto(sock, &snamePacket, sizeof(snamePacket), 0, (struct sockaddr *)&serverStorage, addr_size))<0){
				printf("Failed to send packet. Retransmission.");
			}
			printf("successful retransmission. \n");
	}

	while (1){

		PACKET clientPacket;

		//receive packet
		while ((recvfrom(sock, &clientPacket, sizeof(clientPacket), 0, (struct sockaddr *)&serverStorage, &addr_size))<0){
			printf("Fail to receive packet. \n");
		}
		printf("Packet received.\n");

		if(clientPacket.header.length==0){
			printf("Final packet has been received. \n");

			PACKET fPacket;
			fPacket.header.seq_ack=clientPacket.header.seq_ack;
			fPacket.header.length=0;
			fPacket.header.checksum=0;

			if (rand()%10<=6){ //certain probability to send ACK

				//create new packet with updated ACK
				/*PACKET fPacket;
				fPacket.header.seq_ack=clientPacket.header.seq_ack;
		  	fPacket.header.length=0;
		  	fPacket.header.checksum=0;*/

				//send packet back
				while((sendto(sock, &fPacket, sizeof(fPacket), 0, (struct sockaddr *)&serverStorage, addr_size))<0){
					printf("Failed to send packet. Retransmission. \n");
				}

				printf("ACK sent. \n");
				sequence=!sequence;
			}
			else {
				printf("ACK lost. Waiting\n");
			}

			break;
		}

		//calculate checksum
		int temp=clientPacket.header.checksum;
		clientPacket.header.checksum=0;
		chksum=calc_checksum(&clientPacket, sizeof(clientPacket));

		if(temp==chksum){//checksum matched
			printf("Packet checksum received: %d \n", temp);
			printf("filename cal ck: %d \n", chksum);
			printf("Correct packet ACK received, extracting data. \n");
			//extract data from the packet
			strcpy(buff, clientPacket.data);
			//write data to file
			fwrite(buff, 1, clientPacket.header.length, fout);
			printf("Finish writing data from packet. \n");

			//create new packet with updated ACK
			PACKET serverPacket;
			serverPacket.header.seq_ack=clientPacket.header.seq_ack;
			serverPacket.header.length=0;
			serverPacket.header.checksum=0;

			if (rand()%10<=6){ //certain probability to send ACK

				//create new packet with updated ACK
				/*PACKET serverPacket;
				serverPacket.header.seq_ack=clientPacket.header.seq_ack;
		  	serverPacket.header.length=0;
		  	serverPacket.header.checksum=0;*/

				//send packet back
				while((sendto(sock, &serverPacket, sizeof(serverPacket), 0, (struct sockaddr *)&serverStorage, addr_size))<0){
					printf("Failed to send packet. Retransmission. \n");
				}
				printf("ACK sent. \n");
				sequence=!sequence;
			}
			else {
				printf("ACK lost. Waiting... \n");
			}
		}
		else{
				printf("filename checksum received: %d \n", temp);
      	printf("filename cal ck: %d \n", chksum);

				printf("Checksum matching failed. Wrong packet received. \n");

				//with updated ACK
				PACKET serverPacket;
				serverPacket.header.seq_ack=!clientPacket.header.seq_ack;
				serverPacket.header.length=0;
				serverPacket.header.checksum=0;
				strcpy(serverPacket.data, " ");

				if (rand()%2!=0){ //certain probability of losing ACK
					/*PACKET serverPacket;
					serverPacket.header.seq_ack=!clientPacket.header.seq_ack;
		  		serverPacket.header.length=0;
		  		serverPacket.header.checksum=0;
					strcpy(serverPacket.data, " ");*/

					while((sendto(sock, &serverPacket, sizeof(serverPacket), 0, (struct sockaddr *)&serverStorage, addr_size))<0){
						printf("Failed to send packet. Retransmission.\n");
					}

					printf("ACK sent. \n");
				}
				else {
					printf("ACK lost. waiting...\n");
				}
		}

	}



	fclose(fout);
	return 0;

}
