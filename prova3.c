#include<stdio.h>
#include<string.h>
#include<dirent.h>
#include<stdlib.h>
#include<mpi.h>
#include<ctype.h>

typedef int bool;
#define true 1
#define false 0

//STRUCT PER TENERE TRACCIA DEI FILE DA LEGGERE E LA RELATIVA DIMENSIONE IN BYTE//
typedef struct {
	char nome[500];
	int dimensioneByte;
} fileS;

void wordCount(int *countParola, char** parola, int* size,int inizioByte,int fineByte,int indiceInizioFile,fileS* fileDaLeggere); // FUNZIONE PER CONTARE LE PAROLE 
char* checkStringa(char stringa[]); // FUNZIONE PER CONTROLLARE OGNI SINGOLA STRINGA ED ELIMINARE DA QUESTA I CARATTERI SPECIALI 

int main(int argc, char**argv) {


//VARIABILI E INIZIALIZZAZIONE MPI//
MPI_Init(NULL,NULL);	
int myrank,p;
MPI_Status status;
MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
MPI_Comm_size(MPI_COMM_WORLD, &p);



DIR* dir;
struct dirent* Dirent;
FILE* file;
dir = opendir("fileProva"); 			//APRE DIRECTORY 
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

wordCount(countParola,parola,&size,inizioByte,fineByte,indiceInizioFile,fileDaLeggere);


MPI_Finalize();

}

void wordCount(int *countParola, char** parola, int* size,int inizioByte,int fineByte,int indiceInizioFile,fileS* fileDaLeggere) {
/*	INPUT:
	*countParola:		ARRAY PER IL CONTEGGIO DELLE WORD LETTE
	char** parola:		MATRICE DELLE WORD LETTE
	int *size:		DIMENSIONE PER COUNT_PAROLA E LA MATRICE PAROLA USATA PER L'ALLOCAZIONE DINAMICA
	int inizioByte		INDICA IL BYTE DI INIZIO DI OGNI PROCESSORE
	int fineByte		INDICA IL BYTE DI FINE DI OGNI PROCESSORE
	int indiceInizioFile	INDICA L'INDICE DEL FILE DA CUI IL PROCESSORE DEVE PARTIRE PER LEGGERE
	FileS* fileDaLeggere	STRUTTURA CHE CONTIENE I NOMI DEI FILE E I RELATIVI BYTE*/
	
	int partenza = 0;
	int indice = indiceInizioFile;
	int byteLetti = 0;
	int byte = 0;
	int byteDaLeggere = fineByte - inizioByte;
	bool endFile = false;
	FILE* file;

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
				
				int verifica = 0;
				int indiceWord = 0;
				char *stringa;
				char s[500];
				fscanf(file," %s \n",s);
				stringa = checkStringa(s);
				
				printf("%s ",stringa);

	
				for(int i=0;i<*size;i++) {
					if(strcmp(stringa,parola[i])==0) {
					verifica = 1;
					indiceWord = i;
					}
				}
			
				if(verifica==1) {
					countParola[indiceWord]++;
				}
				else {
					*size = *size+1;
					countParola = (int*)realloc(countParola,sizeof(int)* (*size));
					countParola[*size-1] = 1;
					parola = (char**)realloc(parola,*size*sizeof(char*));
					parola[*size-1] = (char*)malloc(500*sizeof(char));
					strcpy(parola[*size-1],stringa);			
				}
				
				if(indice == indiceInizioFile) { byte = ftell(file) - partenza; printf("byte1 %d\n",byte); }
				else { byte = ftell(file) + byteLetti - partenza; printf("byte2 %d\n",byte);}
			}
			fclose(file);
			byteLetti = byte;
			if(endFile) {indice++; endFile = false;}
		}
	}
	
	for(int i = 0;i<*size;i++) {
	printf("word: %s, count: %d\n",parola[i],countParola[i]);
	}
	
		
}

char* checkStringa(char stringa[]) {
/*	INPUT:
	char stringa[]:		STRINGA DA CONTROLLARE
	OUTPUT:
	char*:			STRINGA MODIFICATA*/
	int size = strlen(stringa);
	char* newString = (char*)malloc(sizeof(char)*size);
	int j = 0;
	for(int i = 0;i<size;i++) {
		if(isalnum(stringa[i])) {
			newString[j] = stringa[i];
			j++;
		}
	}
	newString[j] = '\0';
	return newString;

}








