#include<stdio.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<time.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>

typedef struct{
	char name[50];
	char ip[50];
	int port; 
}Machine; 

//global mutex for thread
pthread_mutex_t my_mutex; 

//global variables 
#define n 4
int matrix[n][n];
Machine linux_machines[n];
int my_port;
int my_routerID; 
int i,j; //vairbale to traverse the array/matrix

//global variable for udp 
int sock; 
struct sockaddr_in serverAddr; 
socklen_t addr_size; 

//function declaraion
void* linkState(void*);
void* receiveInfo(void*);
void printTable(void);
void userUpdate(void); //get table update from user/keyboard

int main(int argc, char *argv[]){
	if(argc!=5){
		printf("Number of inputs is incorrect.\n");
		return 1; 
	}


	my_routerID=atoi(argv[1]);

	char* costs_file=argv[3];
	char* host_file=argv[4];  

	//initiate file pointer and open file 	
	FILE* costIN;
	FILE* machineIN;
	
	costIN=fopen(costs_file,"r");
	machineIN=fopen(host_file, "r");

	if(costIN==NULL||machineIN==NULL){
		printf("file opens error. \n");
		exit(1); 
	}

	//scan through cost file and store items in cost matrix
	for(i=0; i<n; ++i){
		for (j=0; j<n; ++j){
			fscanf(costIN, "%d", &matrix[i][j] );
		}
	}
	
	//scan through machines file and store items in machines array 
	for(i=0; i<n; ++i){
		fscanf(machineIN, "%s %s %d", linux_machines[i].name, linux_machines[i].ip, &linux_machines[i].port);
		if(i==my_routerID){
			my_port=linux_machines[i].port; 
		}
	}

	//close files
	fclose(costIN);
	fclose(machineIN);


	//initialize socket 
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_port=htons(my_port); //configure port number for our own id  
	serverAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	memset((char*)serverAddr.sin_zero, '\0',sizeof(serverAddr.sin_zero));
	addr_size=sizeof(serverAddr); 

	//create socket
	if((sock=socket(AF_INET, SOCK_DGRAM, 0))<0){
		printf("socket error. \n");
		return 1; 
	}

	//bind socket
	if(bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr))!=0){
		printf("bind error.\n");
		exit(1);  
	}

	//create thread
	pthread_t thread2, thread3;
	pthread_create(&thread2, NULL, receiveInfo, NULL);
	pthread_create(&thread3, NULL, linkState, NULL); 
	//mutex initialize
	pthread_mutex_init(&my_mutex,NULL); 
	
	int count=2;
	while(count>0){
		userUpdate(); 
		count--; 
		sleep(30); //sleep for 30 seconds after 2 changes
	} 
  
	return 1; 
}

void userUpdate(){
	int new_neighbor_id, new_cost;//to hold the receiving new info from user
	//for(i=0; i<2; ++i){//only implement 2 times
		printf("Please enter the neighbor ID that you want to update and the new cost: ");
		scanf("%d %d", &new_neighbor_id, &new_cost); 
	//}

	//update table with new cost
	pthread_mutex_lock(&my_mutex);
	matrix[my_routerID][new_neighbor_id]=new_cost;
	matrix[new_neighbor_id][my_routerID]=new_cost;
	printTable();
	pthread_mutex_unlock(&my_mutex);

	//another sockaddr_in struct for intended recipient
	struct sockaddr_in dest_addr;
	dest_addr.sin_family=AF_INET;   
	
	//send update to all neighbors 
	for (i=0; i<n; ++i){
		if(i==my_routerID){
			continue; //skip if sending to our own id 
		}

		//configure port and address 
		dest_addr.sin_port=htons(linux_machines[i].port);
		inet_pton(AF_INET, linux_machines[i].ip, &dest_addr.sin_addr.s_addr);
		
		//create array for sending update information 
		int info[3]={my_routerID, new_neighbor_id, new_cost};
		
		printf("Sending update information to %s \t  %d \n", linux_machines[i].ip, linux_machines[i].port);
		
		//send
		sendto(sock, info, sizeof(info), 0, (struct sockaddr*)&dest_addr, addr_size);
	}
	sleep(10); 
}

void* linkState(void* b){
	
		int leastDistArray[4]; //array to keep track of least cost path
		int visited[4];	//array to keep track of visited nodes 
		//int srcNode=my_routerID; ; 
		int count; 
		int min;
		int minIndex; 
		int addTemp; 		


		while(1){
			printf("least cost array: \n");
			//for each node->srcNode; 
			
			


			for(i=0; i<n; ++i){
				/*if(i==srcNode){
					continue; 
				}*/
				
				//initialize visited to 0 and leadDistArray
				for(j=0; j<n; ++j){
					visited[j]=0;
					leastDistArray[j]=matrix[i][j];
				}
				visited[i]=1; 
				
				count=n-1; 				

				//for n-1 times 
				while(count>0){
					min=1000000000;
					minIndex=-1;

					for(j=0; j<n; ++j){
						if(visited[j]==0&&leastDistArray[j]<min){
							min=leastDistArray[j];
							minIndex=j; 
						}
					}
					visited[minIndex]=1;//mark the node as visited 

					for(j=0; j<n; ++j){
						if(visited[j]==0){
							addTemp=leastDistArray[minIndex]+matrix[minIndex][j];
							if(addTemp<leastDistArray[j]){
								leastDistArray[j]=addTemp; 
							}
						}			
					}


					--count;//decrement counter 
				}

				for(j=0; j<n; ++j){
					printf("%d ", leastDistArray[j]);
				}
				printf("\n"); 	
			}
			
			
		//	printf("The least dist array:");
			/*for(i=0; i<n; ++i){
				printf("%d, ", leastDistArray[i]);
			}
			printf("\n");*/ 
		
			//sleep for a random number between 20-30 secs
			sleep(rand()%11+10);
		}
	}


void* receiveInfo(void* a){
	int info_recv[3];//array to store the receiving updates 
	int routerID_recv, neighbor_id_recv, new_cost_recv; //variabels to hold new information
	while(1){
		recvfrom(sock, info_recv, sizeof(info_recv), 0, NULL, NULL); 
		
		routerID_recv=info_recv[0];
		neighbor_id_recv=info_recv[1];
		new_cost_recv=info_recv[2]; 

		printf("Receive updates.\n");
		printf("Updating the table...\n"); 

		//lock thread
		pthread_mutex_lock(&my_mutex);
	
		//updating the table
		matrix[routerID_recv][neighbor_id_recv]=new_cost_recv;
		matrix[neighbor_id_recv][routerID_recv]=new_cost_recv; 
				
		//unlock thread
		pthread_mutex_unlock(&my_mutex); 

		printTable(); 
	}
}

//function to print cost table 
void printTable(){
	printf("Cost table has been updated: \n"); 
	for(i=0; i<n; ++i){
		for(j=0; j<n; ++j){
			printf("%d\t", matrix[i][j]);
			printf(" "); 		
		}
		printf("\n"); 
	}
}
