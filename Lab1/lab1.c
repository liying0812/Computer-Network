//Name: Liying Liang 
//Lab Section: Thursday 2:15-5PM 

#include<stdio.h>
#include<stdlib.h>

int main(int argc, char*argv[]){
	FILE *fin=fopen(argv[1],"rb");
	FILE *fout=fopen(argv[2],"wb");

	if (fin==NULL||fout==NULL){
		printf("Cannot open the file");
		exit(0);
	}

	char buffer[10];
	size_t total=0; 

	while (!feof(fin)){
		total=fread(buffer, 1, 10, fin);
		fwrite(buffer, 1, total, fout);
	}

	fclose(fin);
	fclose(fout); 

	return 0; 
}
