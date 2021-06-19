#include<stdio.h>
#include<string.h>
#include<dirent.h>
#include<stdlib.h>
#include<mpi.h>
#include<ctype.h>
#include<time.h>
#include<unistd.h>
#include<time.h>

typedef int bool;
#define true 1
#define false 0

//STRUCT PER TENERE TRACCIA DEI FILE DA LEGGERE E LA RELATIVA DIMENSIONE IN BYTE//
typedef struct {
	char nome[500];
	int dimensioneByte;
} fileS;

void wordCount(int *countParola, char** parola, int* size,int inizioByte,int fineByte,int indiceInizioFile,fileS* fileDaLeggere,int myrank,int p); // FUNZIONE PER CONTARE LE PAROLE 
char* takeAndCheckString(FILE *file);	// FUNZIONE PER CONTROLLARE OGNI SINGOLA STRINGA ED ELIMINARE DA QUESTA I CARATTERI SPECIALI
void backWord(FILE* file, int partenza,int myrank); // FUNZIONE CHE PERMETTE DI TORNARE INDIETRO E RECUPERARE LA PAROLA CHE IL PROCESSORE P-1 HA SALTATO
void Sort(int* a,char** b, int dim);	//FUNZIONE PER ORDINARE IN BASE ALLE FREQUENZE I DUE ARRAY PER IL CONTEGGIO DELLE PAROLE
void CSV(int* a, char** b,int dim);		//FUNZIONE PER SALVARE IL CONTEGGIO DELLE PAROLE IN UN FILE CSV

int main(int argc, char**argv) {

	//VARIABILI PER TEMPO DI ESECUZIONE
	clock_t start,end;
	double tempo;
	start = clock();


	//VARIABILI E INIZIALIZZAZIONE MPI//
	MPI_Init(NULL,NULL);	
	int myrank,p;
	MPI_Status status;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	MPI_Comm_size(MPI_COMM_WORLD, &p);



	DIR* dir;
	struct dirent* Dirent;
	FILE* file;
	dir = opendir("file"); 		//APRE DIRECTORY 
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

	if(dir!=NULL) {		//CONTROLLO PER DIRECTORY VUOTA

		while((Dirent=readdir(dir))!=NULL) {	// CICLO PER CONTARE IL NUMERO DI FILE E QUINDI STABILE LA DIMENSIONE PER L'ALLOCAZIONE DINAMICA 
			count++;
			}
		fileDaLeggere = (fileS*)malloc(sizeof(fileS)*count);
			
		seekdir(dir,0);
		count = 0;
		while((Dirent=readdir(dir))!=NULL) { 			//CICLO PER LEGGERE OGNI FILE DELLA DIRECTORY E CALCOLARE LA RELATIVA DIMENSIONE IN BYTE
			//printf("file : %s tipo %d \n",Dirent->d_name,Dirent->d_type);
			if(Dirent->d_type==8) {
				char stringa[100] ="file/";
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

	//CALCOLO INIZIO_BYTE E FINE_BYTE PER OGNI SINGOLO PROCESSORE 
	int resto = sommabyte%p;
	if(myrank<resto){
		inizioByte = myrank*(sommabyte/p+1);
		fineByte = inizioByte+(sommabyte/p+1);
		printf("rank %d inizio : %d fine : %d\n",myrank,inizioByte,fineByte);
		fflush(stdout);
	}
	else {
		inizioByte = myrank*(sommabyte/p)+resto;
		fineByte = inizioByte+(sommabyte/p);
		printf("rank %d inizio : %d fine : %d\n",myrank,inizioByte,fineByte);
		fflush(stdout);
	}


	//CALCOLO INDICE_INIZIO_FILE PER OGNI SINGOLO PROCESSORE
	int sommaFile = fileDaLeggere[0].dimensioneByte;	
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
	
	//FUNZIONE CHE CONTA LE PAROLE NEI FILE 
	wordCount(countParola,parola,&size,inizioByte,fineByte,indiceInizioFile,fileDaLeggere,myrank,p);

	MPI_Barrier(MPI_COMM_WORLD);
	end = clock();
	tempo=((double)(end-start))/CLOCKS_PER_SEC;
	if(myrank==0){	printf("tempo %f secondi\n",tempo);}
	fflush(stdout);

	MPI_Finalize();

}

void wordCount(int *countParola, char** parola, int* size,int inizioByte,int fineByte,int indiceInizioFile,fileS* fileDaLeggere, int myrank, int p) {
/*	INPUT:
	*countParola:		ARRAY PER IL CONTEGGIO DELLE WORD LETTE
	char** parola:		MATRICE DELLE WORD LETTE
	int *size:		DIMENSIONE PER COUNT_PAROLA E LA MATRICE PAROLA USATA PER L'ALLOCAZIONE DINAMICA
	int inizioByte		INDICA IL BYTE DI INIZIO DI OGNI PROCESSORE
	int fineByte		INDICA IL BYTE DI FINE DI OGNI PROCESSORE
	int indiceInizioFile	INDICA L'INDICE DEL FILE DA CUI IL PROCESSORE DEVE PARTIRE PER LEGGERE
	FileS* fileDaLeggere	STRUTTURA CHE CONTIENE I NOMI DEI FILE E I RELATIVI BYTE
	int myrank		INTERO RELATIVO AL PROCESSO
	int p			INTERO CHE INDICA IL NUMERO DI PROCESSI */
	
	int recvcounts[p];
	int displs[p];
	
	int partenza = 0;				//INTERO  CHE INDICA AL PROCESSO DA QUALE BYTE DEVE PARTIRE
	int indice = indiceInizioFile;			//INDICE UTILIZZATO PER CAPIRE QUALE FILE SI STA LEGGENDO 
	int byteLetti = 0;				//NUMERO DI BYTE LETTI GLOBALE
	int byte = 0;					//NUMERO DI BYTE LETTI TEMPORANEMAENTE NEL SECONDO WHILE DI CONTROLLO
	int byteDaLeggere = fineByte - inizioByte;	//BYTE TOTALI CHE OGNI PROCESSO DEVE LEGGERE
	bool endFile = false;				//BOOLEANO CHE SERVE A DETERMINARE LA FINE DEL FILE E CAPIRE SE BISOGNA APRIRE UN NUOVO FILE
	FILE* file;					//PUNTATORE RELATIVO AL FILE DA LEGGERE
	
	int backWordControl = 1;			//FLAG USATO PER LA FUNZIONE BACKWORD 
	//printf("myrank %d\n",myrank);
	//sleep(myrank);
	while(byteLetti<byteDaLeggere) {		//PRIMO CICLO DI CONTROLLO OGNI PROCESSORE LEGGE IN TOTALE UN NUMERO DI BYTE <= DI ByteDaLeggere
		file=fopen(fileDaLeggere[indice].nome,"r");
		if(file!=NULL) {
			//CALCOLO DELLA PARTENZA 
			if(indice==0) { partenza = inizioByte;}	
			if(indice!=indiceInizioFile) { partenza = 0;}
			if(indice==indiceInizioFile && indice>0) {
				int s = 0;
				for(int i = 0;i<indice;i++) { s = s+fileDaLeggere[i].dimensioneByte;}
				partenza = inizioByte - s;
			}
			//MI SPOSTO IN PARTENZA
			fseek(file, partenza, SEEK_SET );
			
			
			byte = 0;
			while(byte<byteDaLeggere) {	//SECONDO CICLO DI CONTROLLO UTILE SICCOME UN PROCESSO POTREBBE DOVER LEGGERE DA PIU' FILE
			
				if(feof(file)) {endFile = true; break;}
				
				int verifica = 0;
				int indiceWord = 0;
				char *stringa;
				
				//CONTROLLO SE IL PROCESSO DEVE SPOSTARSI INDIETRO SICCOME IL PROCESSO P-1 HA SALTATO LA PAROLA DATO CHE SI TROVAVA ALLA FINE DEI SUOI BYTE
				if(backWordControl&&myrank>0) {
					backWord(file,partenza,myrank);
					backWordControl = 0;
				}
				
				//FUNZIONE CHE LEGGE UNA PAROLA NEL FILE
				stringa = takeAndCheckString(file);
				
				
				//printf("myrank %d :%s ",myrank,stringa);
				
				//CALCOLO DEI BYTE LETTI
				if(indice == indiceInizioFile) { byte = ftell(file) - partenza; /*printf("byte1 %d\n",byte);*/}
				else { byte = ftell(file) + byteLetti - partenza; /*printf("byte2 %d\n",byte);*/}
				
				//CONTROLLO SE POSSO SALVARE LA PAROLA LETTA E QUINDI SE I BYTE DA LEGGERE SONO TERMINATI
				if(byte-1<=byteDaLeggere && strlen(stringa)>0) {
				
				
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
				}
				
			}
			fclose(file);
			byteLetti = byte;
			if(endFile) {indice++; endFile = false;}
		}
	}
	
	
	/*sleep(myrank); //DA TOGLIERE
	printf("myrank %d\n",myrank);
	
	for(int i = 0;i<*size;i++) {
		printf("word: %s, count: %d\n",parola[i],countParola[i]);
		fflush(stdout);
	}*/
	
	int charInt = (sizeof(char)*500)+sizeof(int);
	int bufsiz = *size*charInt;
	
	char message[bufsiz];
	int position = 0;
	
	//EFFETTUO IL PACK DELLE DUE STRUTTURE 
	for(int i = 0;i<*size;i++) {
		MPI_Pack(parola[i],500,MPI_CHAR,message,bufsiz,&position,MPI_COMM_WORLD);
		MPI_Pack(&countParola[i],1,MPI_INT,message,bufsiz,&position,MPI_COMM_WORLD);
	}
	
	//OGNI PROCESSO INVIA AL RANK 0 LA DIMENSIONE DELLA PROPRIA STUTTURA
	MPI_Gather(size, 1, MPI_INT, recvcounts, 1, MPI_INT, 0, MPI_COMM_WORLD);
	
	int bufSizeRecv = 0;
	//IL RANK 0 SI CALCOLA RECVCOUNTS E DISPLS PER LA GATHERV
	if(myrank==0) {
		for(int i = 0;i<p;i++) {
			recvcounts[i] = recvcounts[i]*charInt;
			bufSizeRecv = bufSizeRecv +recvcounts[i];
		}
		displs[0] = 0;
	    	for(int i=1; i<p;i++) {
	    		displs[i] = recvcounts[i-1]+displs[i-1];
	    		//printf("displs %d\n",displs[i]);
    		}
    	}
    	char* recvbuf = (char*)malloc(sizeof(char)*bufSizeRecv);
    	
    	//printf("bufsizeRecv%d\n",bufSizeRecv);
	
	//ONGI PROCESSO INVIA AL RANK 0 LE DUE STRUTTURE  
	MPI_Gatherv(message, bufsiz, MPI_PACKED, recvbuf, recvcounts, displs , MPI_PACKED, 0, MPI_COMM_WORLD);
	
	free(countParola);
	for(int i = 0;i<*size;i++) {
		free(parola[i]);	
	}
	free(parola);
	
	//char matrixWord[bufSizeRecv/charInt][500];	
	//int matrixCount[bufSizeRecv/charInt];
	char** matrixWord = (char**)malloc(sizeof(char*)*(bufSizeRecv/charInt));
	for(int i = 0; i<(bufSizeRecv/charInt);i++) {
		matrixWord[i] = (char*)malloc(sizeof(char)*500);
	}
	int* matrixCount = (int*)malloc(sizeof(int)*(bufSizeRecv/charInt));
	position = 0;
	
	int dimensione = 0;
	int* countFinalWord = (int*)malloc(sizeof(int));
	char** matrixFinalWord = (char**)malloc(sizeof(char*));
	matrixFinalWord[0] = (char*)malloc(500*sizeof(char));
	
	int verifica = 0;
	int indiceWord = 0;
	
	//IL RANK 0 EFFETTUA L'UNPACK E SALVA LE STRUTTURE RICEVUTE NELLA MATRICE MATRIX WORD PER QUANTO RIGUARDA LE PAROLE E L'ARRAY MATRIXCOUNT PER IL CONTEGGIO
	if(myrank==0) {
		for(int i=0;i<(bufSizeRecv/charInt);i++) {
			MPI_Unpack(recvbuf,bufSizeRecv,&position,matrixWord[i],500,MPI_CHAR,MPI_COMM_WORLD);
			MPI_Unpack(recvbuf,bufSizeRecv,&position,&matrixCount[i],1,MPI_INT,MPI_COMM_WORLD);
		}
		//IL RANK 0 EFFETTUA LA FUNZIONE DI REDUCE IN MODO DA SOMMARE I CONTEGGI DELL STESSE PAROLE CHE GLI ALTRI PROCESSI HANNO INVIATO
		for(int i=0;i<bufSizeRecv/charInt;i++) {
			for(int j = 0;j<dimensione;j++) {
				if(strcmp(matrixWord[i],matrixFinalWord[j])==0) {
					verifica = 1;
					indiceWord = j;
					}
				}
			
				
			if(verifica==1) {
				countFinalWord[indiceWord] = countFinalWord[indiceWord] + matrixCount[i];
				verifica = 0;
				indiceWord = 0;
			}
			else {
				dimensione++;
				countFinalWord = (int*)realloc(countFinalWord,sizeof(int)*dimensione);
				countFinalWord[dimensione-1] = matrixCount[i];
				matrixFinalWord = (char**)realloc(matrixFinalWord,dimensione*sizeof(char*));
				matrixFinalWord[dimensione-1] = (char*)malloc(500*sizeof(char));
				strcpy(matrixFinalWord[dimensione-1],matrixWord[i]);			
				}
			}
			//ORDINAMENTO PER FREQUENZA
			Sort(countFinalWord,matrixFinalWord,dimensione);
			CSV(countFinalWord,matrixFinalWord,dimensione);
			printf("WORD COUNT TERMINATO\n");
			fflush(stdout);
			/*for(int j = 0;j<dimensione;j++) {
				printf("COM word: %s, count: %d\n",matrixFinalWord[j],countFinalWord[j]);
			}*/
		
	}
	
	free(countFinalWord);
	for(int i = 0;i<dimensione;i++) {
		free(matrixFinalWord[i]);	
	}
	free(matrixFinalWord);
	free(recvbuf);
	free(matrixCount);
	for(int i = 0;i<(bufSizeRecv/charInt);i++) {
		free(matrixWord[i]);	
	}
	//printf("buffer%d\n",bufSizeRecv);
	
}

char* takeAndCheckString(FILE *file) {
/*	INPUT:
	FILE *file:		PUNTATORE RELATIVO AL FILE CHE BISOGNA LEGGERE
	OUTPUT:
	char*:			STRINGA MODIFICATA*/
	
	char* newString = (char*)malloc(sizeof(char)*500);
	int i = 0;
	int c = fgetc(file);
	while(isalnum(c)) {
		newString[i] = c;
		c = fgetc(file);
		i++;
		}
	newString[i] = '\0';
	return newString;

}

void backWord(FILE* file, int partenza, int myrank) {
/*	INPUT:
	FILE* file:		FILE DI RIFERIMENTO PER RECUPERARE LA PAROLA CHE IL PROCESSORE P-1 HA SALTATO
	int partenza:		INTERO CHE DETERMINA IN QUALE POSIZIONE BISGONA INIZIARE A LEGGERE
	int myrank		INTERO RELATIVO AL PROCESSO 
	*/
	int verifica = 1;
	char c;
	int indietro = partenza;
	//if(myrank==1){printf("partenza:%d\n",partenza);}
	if(file!=NULL) {
		while(verifica) {
			fseek(file,indietro, SEEK_SET );
			c = fgetc(file);
			if(indietro==0||(!isalnum(c))) {verifica = 0;}
			else {
				indietro--;
			}		
		}
		fseek(file,indietro, SEEK_SET );
	}
}


void Sort(int* a,char** b, int dim){
/*	INPUT:
	int* a:			ARRAY CHE CONTEINE I CONTEGGI RELEATIVI ALLE PAROLE 
	char** b:		MATRICE DELLE PAROLE 
	int dim:		DIMENSIONE DELLE DUE STRUTTURE 
	*/

	int i, j, max, tmp;
	char temp[500];
	for(i=0;i<dim-1;i++){
		max=i;
		for(j=i+1;j<dim; j++){
			if(a[j]>a[max])
                     max=j;
		}
	tmp=a[max];
	strcpy(temp,b[max]);
        a[max]=a[i];
        strcpy(b[max],b[i]);
        a[i]=tmp;
        strcpy(b[i],temp);
	}
}

void CSV(int* a,char** b,int dim) {
/*	INPUT:
	int* a:			ARRAY CHE CONTEINE I CONTEGGI RELEATIVI ALLE PAROLE 
	char** b:		MATRICE DELLE PAROLE
	int dim:		DIMENSIONE DELLE DUE STRUTTURE  
	*/
	
	FILE* file;
	file = fopen("wordCount.csv","w+");
	fprintf(file,"WORD , COUNT\n");
	for(int i = 0;i<dim;i++) {
		fprintf(file,"%s , %d\n",b[i],a[i]);
	}
	fclose(file);
}










