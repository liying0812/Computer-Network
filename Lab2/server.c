/*Liying Liang
 *COEN146L Lab2
 *1/17/2019
 *Server file 
 */

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>

int main(int argc, char *argv[]){
	//initializer
	int n; 
	int listenfd=0;
	int connfd=0;
	struct sockaddr_in serv_addr;
	char buff[10];

	if(argc!=2){
		printf("Arguments are wrong.");
		return 1; 
	}

	//set up
	memset(&serv_addr, '0', sizeof(serv_addr));
	memset(buff, '0', sizeof(buff)); 

	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	serv_addr.sin_port=htons(atoi(argv[1]));

	//create socket, bind, and listen
	listenfd=socket(AF_INET, SOCK_STREAM, 0);
	bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	listen(listenfd, 10); 

	//server on
	while(1){
		connfd=accept(listenfd, (struct sockaddr*)NULL, NULL);

		//receive the name of the output file
		if((n=read(connfd, buff, sizeof(buff)))<0){
			printf("Fail to receive the name of file");
			return 1; 
		}

		printf("This is output file name: %s\n", buff); 
	
		FILE* fout=fopen(buff ,"wb");
		if(fout==NULL){
			printf("Fail to open file.");
			return 1; 
		}

		while((n=read(connfd, buff, sizeof(buff)))>0){
			fwrite(buff, 1, n, fout); 
		}
		
		fclose(fout);
		close(connfd); 
	}

	return 0; 
}
