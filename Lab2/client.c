/*Liying Liang
 *COEN 146L Lab2
 *client file
 *1/17/2019
 */

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]){
	int i;
	int sockfd=0;
	char buff[10];
	struct sockaddr_in serv_addr;

	if (argc!=5){
		printf("Not Enough arguments");
		return 1; 
	}

	memset(buff, '0', sizeof(buff));
	memset(&serv_addr, '0', sizeof(serv_addr)); 

	//open socket
	if((sockfd=socket(AF_INET, SOCK_STREAM,0))<0){
		printf("Error: fail to create socket.");
		return 1; 
	}

	//set address
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(atoi(argv[1]));

	if(inet_pton(AF_INET, argv[2], &serv_addr.sin_addr)<=0){
		printf("inet_pton error occured\n");
		return 1; 
	}

	//connect
	if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0){		printf("Error: fail to connect");
		return 1;
	}
	
	//write output file name
	if (write(sockfd, argv[4], strlen(argv[4])+1)<0){
		printf("Error: Cannot send destination file to socket.");
		return 1; 
	}

	//Open input file
	FILE*fin = fopen(argv[3], "rb");
	if (fin==NULL){
		printf("Fail to open the file.");
		return 1; 
	}

	while(!feof(fin)){
		int total=fread(buff, 1, 10, fin);
		write(sockfd, buff, total); 
	}

	fclose(fin);
	
	return 0; 

}
