#include<stdio.h>
#include<string.h>
#include<dirent.h>
#include<stdlib.h>
#include<mpi.h>


typedef int bool;
#define true 1
#define false 0

void conta(FILE* file, int *countParola, char** parola, int* size );

int main(int argc, char**argv) {


//VARIABILI E INIZIALIZZAZIONE MPI//
MPI_Init(NULL,NULL);	
int myrank,p;
MPI_Status status;
MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
MPI_Comm_size(MPI_COMM_WORLD, &p);


//STRUCT PER TENERE TRACCIA DEI FILE DA LEGGERE E LA RELATIVA DIMENSIONE IN BYTE//
typedef struct {
	char nome[500];
	int dimensioneByte;
} fileS;

DIR* dir;
struct dirent* Dirent;
FILE* file;
dir = opendir("fileProva"); 		// APRE DIRECTORY 
int sommabyte = 0; 			//VARIABILE PER LA SOMMA TOTALE IN BYTE DI TUTTI I FILE 
fileS* fileDaLeggere; 			// DICHIARAZIONE STRUCT PER FILE DA LEGGERE 
int count = 0;				//CONTATORE PER NUMERO DI FILE IN DIRECTORY 
int inizioByte, fineByte;		//INIZIO_BYTE E FINE_BYTE PER OGNI FILE 
int indiceInizioFile;			//INDICE PER CAPIRE DA QUALE FILE BISOGNA PARTIRE 

int size = 0;				//DIMENSIONE DI COUNT_PAROLA E PAROLA 
char** parola;				//MATRICE DI CHAR PER TENERE TRACCIA DELL WORD LETTE 
int* countParola;			//CONTATORE DELLE WORD LETTE 

countParola = (int*)malloc(sizeof(int));// ALLOCAZIONE DINAMICA PER COUNT_PAROLA E LA MATRICE PAROLA
parola = (char**)malloc(sizeof(char*));
parola[0] = (char*)malloc(500*sizeof(char));

if(dir!=NULL) {		//CONTROLLO PER DIRECOTORY VUOTA

	while((Dirent=readdir(dir))!=NULL) {	// CICLO PER CONTARE IL NUMERO DI FILE E QUINDI STABILE LA DIMENSIONE PER L'ALLOCAZIONE DINAMICA 
		count++;
		}
	fileDaLeggere = (fileS*)malloc(sizeof(fileS)*count);
		
	seekdir(dir,0);
	count = 0;
	while((Dirent=readdir(dir))!=NULL) { 			//CICLO PER LEGGERE OGNI FILE DELLA DIRECTORY E CALCOLARE LA RELATIVA DIMENSIONE IN BYTE
		//printf("file : %s tipo %d \n",Dirent->d_name,Dirent->d_type);
		if(Dirent->d_type==8) {
			char stringa[100] ="fileProva/";
			strcpy(fileDaLeggere[count].nome,stringa);
			strcat(fileDaLeggere[count].nome,Dirent->d_name);
			//printf("%s\n",fileDaLeggere[count].nome);
			file=fopen(fileDaLeggere[count].nome,"r");
			if(file!=NULL) {
				fseek(file,0L,SEEK_END);
				fileDaLeggere[count].dimensioneByte = ftell(file);
				count++;
				fseek(file,0L,SEEK_SET);
				fclose(file);	
			}
		}
		
	}
	for(int i=0;i<count;i++) {
		if(myrank==0){ printf("byte %s : %d\n",fileDaLeggere[i].nome,fileDaLeggere[i].dimensioneByte);fflush(stdout);}
		sommabyte += fileDaLeggere[i].dimensioneByte;
	}
	if(myrank==0){ printf("somma byte : %d\n\n",sommabyte);fflush(stdout);}
	
}

closedir(dir);

if(sommabyte%p==0) { 					//CALCOLO INIZIO_BYTE E FINE_BYTE PER OGNI SINGOLO PROCESSORE 
	inizioByte = myrank*(sommabyte/p);
	fineByte = inizioByte+(sommabyte/p)-1;
	printf("rank %d inizio : %d fine : %d\n",myrank,inizioByte,fineByte);
	fflush(stdout);
} else {
	int resto = sommabyte%p;
	if(myrank<resto){
		inizioByte = myrank*(sommabyte/p+1);
		fineByte = inizioByte+(sommabyte/p);
		printf("rank %d inizio : %d fine : %d\n",myrank,inizioByte,fineByte);
		fflush(stdout);
	}
	else {
		inizioByte = myrank*(sommabyte/p)+resto;
		fineByte = inizioByte+(sommabyte/p)-1;
		printf("rank %d inizio : %d fine : %d\n",myrank,inizioByte,fineByte);
		fflush(stdout);
	}
}

int sommaFile = fileDaLeggere[0].dimensioneByte;	//CALCOLO INDICE_INIZIO_FILE PER OGNI SINGOLO PROCESSORE
//printf("size filedimension %d\n",count);
for(int i = 0;i<count;i++) {
	if(inizioByte<sommaFile) {
		indiceInizioFile = i;
		break;
	}
	else {
		sommaFile = sommaFile+fileDaLeggere[i+1].dimensioneByte;
	}
}

printf("rank %d Inizio File : %d\n",myrank,indiceInizioFile);
fflush(stdout);


int partenza = 0;
int indice = indiceInizioFile;
int byteLetti = 0;
int byte = 0;
int byteDaLeggere = fineByte - inizioByte;
bool endFile = false;

while(byteLetti<=byteDaLeggere) {
	file=fopen(fileDaLeggere[indice].nome,"r");
	if(file!=NULL) {
		if(indice==0) { partenza = inizioByte;}
		if(indice!=indiceInizioFile) { partenza = 0;}
		if(indice==indiceInizioFile && indice>0) {
			int s = 0;
			for(int i = 0;i<indice;i++) { s = s+fileDaLeggere[i].dimensioneByte;}
			partenza = inizioByte - s;
		}
		fseek(file, partenza, SEEK_SET );
		byte = 0;
		while(byte<=byteDaLeggere) {
			if(feof(file)) {endFile = true; break;}
			/*char stringa[500];
			fscanf(file,"%s",stringa);
			printf("%s\n",stringa);*/
			conta(file,countParola ,parola ,&size);
			if(indice == indiceInizioFile) { byte = ftell(file) - partenza; }
			else { byte = ftell(file) + byteLetti - partenza;}
		}
		fclose(file);
		byteLetti = byte;
		if(endFile) {indice++; endFile = false;}
	}
}


MPI_Finalize();

}

void conta(FILE* file,int *countParola, char** parola, int* size ) {
/*	INPUT:
	File *file:	file da leggere
	char stringa[]:	word letta dal processore
	*countParola:	ARRAY PER IL CONTEGGIO DELLE WORD LETTE
	char** parola:	MATRICE DELLE WORD LETTE
	int *size:	DIMENSIONE PER COUNT_PAROLA E LA MATRICE PAROLA USATA PER L'ALLOCAZIONE DINAMICA*/
	
	int verifica = 0;
	int indice = 0;
	char stringa[500];
	fscanf(file,"%s",stringa);
	printf("%s\n",stringa);

	
		for(int i=0;i<*size;i++) {
			if(strcmp(stringa,parola[i])==0) {
			verifica = 1;
			indice = i;
			}
		}
	
		if(verifica==1) {
			countParola[indice]++;
		}
		else {
			*size = *size+1;
			printf("1\n");
			countParola = (int*)realloc(countParola,sizeof(int)* (*size));
			printf("2\n");
			countParola[*size-1] = 1;
			printf("3\n");
			parola = (char**)realloc(parola,*size*sizeof(char*));
			printf("4\n");
			parola[*size-1] = (char*)malloc(500*sizeof(char));
			printf("5\n");
			strcpy(parola[*size-1],stringa);
			printf("6\n");				
		}	
			
	
}








