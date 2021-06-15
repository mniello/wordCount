#include<stdio.h>
#include<string.h>
#include<dirent.h>
#include<stdlib.h>
int conta(FILE* file);
char* scanStringa(char* stringa);
int main(int argc, char**argv) {

DIR* dir;
struct dirent* Dirent;
FILE* file;
dir = opendir("file");
int* wordcount;
int count = 0;
if(dir!=NULL) {

	while((Dirent=readdir(dir))!=NULL) {
		count++;
		}
		
	seekdir(dir,0);
	wordcount = (int*)malloc(sizeof(int)*count);
	count = 0;
	while((Dirent=readdir(dir))!=NULL) {
		printf("file : %s tipo %d \n",Dirent->d_name,Dirent->d_type);
		if(Dirent->d_type==8) {
			char stringa[100] ="file/";
			strcat(stringa,Dirent->d_name);
			printf("%s",stringa);
			file=fopen(stringa,"r");
			
			if(file!=NULL) {
				printf("ENTRATO");
				wordcount[count] = conta(file);
				count++;	
			}
		}
		
	} 

}

closedir(dir);

}

int conta(FILE* file) {

int cont=0;
char stringa[500];

while(!feof(file)) {
	fscanf(file,"%s",stringa);
	printf("%s\n",stringa);
	cont++;
}
fclose(file);
return cont;
}

char* scanStringa(char* stringa) {

}






