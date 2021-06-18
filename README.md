# Implementazione problema Word Count con MPI
![Alt text](/img/wordCount.png?raw=true "Title")
## 1.Introduzione
Il problema Word Count consiste nel determinare il numero di occorrenze di una parola in un testo dato in input. Generalmente si presenta in applicazioni in cui è necessario
mantenere le parole inserite in limiti definiti.Il conteggio delle parole è comunemente usato anche  dai traduttori per determinare il prezzo di un lavoro di traduzione, oppure può essere utilizzato anche per calcolare le misure di leggibilità e per misurare la velocità di digitazione e lettura.
Anche se sembra semplice nella sua definizione, il problema Word Count rappresenta
una sfida nel campo della programmazione distribuita a causa dell’enorme taglia
dei problemi che possono presentarsi: infatti possono esserci situazioni in cui i
documenti da esaminare hanno dimensioni anche di 100 TB, dove un’esecuzione
sequenziale può arrivare a richiedere mesi se non anni di computazione.
Quindi bisogna pensare a Word Count come un problema di programmazione
distribuita, in cui la distrbuzione dell’input tra i nodi di un cluser può significativamente diminuire il tempo necessario. Di solito quando si risolve il problema Word Count con la programmazione distribuita utilizzando un cluster si effettuano i seguenti passaggi:
1. Il nodo MASTER legge l'elenco dei file dove bisogna contare le parole in modo da assegnare il lavoro a tutti gli altri nodi. Quindi ciascuno dei nodi dovrebbe ricevere la propria parte di lavoro dal MASTER. Una volta che un processo ha ricevuto il suo elenco di file da elaborare, dovrebbe quindi leggere in ciascuno dei file ed eseguire il conteggio delle parole, tenendo traccia delle frequenze. Ogni nodo ovviamente dovrà utilizzare una stuttura dati in modo da mantenere in memoria le occorenze.
2. La seconda fase prevede che ciascuno dei processi invii la propria stuttura al nodo master. Il MASTER dovrà quindi raccogliere tutte queste informazioni.
3. L'ultima fase consiste nel combinare il lavoro svolto da ogni nodo del cluster. Ad esempio una parola potrebbe essere conteggiata da più processi bisgona quindi sommare tutte queste occorrenze e salvare poi il lavoro.
## 2.Soluzione proposta
Viene proposta una soluzione in un ambiente distribuito, verranno quindi descritte di seguito due fasi riguardanti il programma scritto in C che utilizza MPI per la comunicazione. MPI è di fatto lo standard per la comunicazione tra nodi appartenenti a un cluster che eseguono un programma parallelo. Inoltre ogni nodo del cluster ha accesso agli stessi dati. La soluzione proposta segue un paradigma simile a quello di Map Reduce, infatti come spiegato nell'introduzione ci sarà un prima fase di Map dove ogni nodo elabora la propria parte di input, dopodichè ci sarà la fase di Reduce, dove i risultati di ogni nodo vengono raggruppati. Nel capitolo riguardante i Benchmark verrà derscritto in modo accurato l'architettura del cluster utilizzata, adesso passiamo alla descrizione della soluzione.
### 2.1 Partizionamento del lavoro 
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
	dir = opendir("file"); 		 
	int sommabyte = 0; 			 
	fileS* fileDaLeggere; 			
	int count = 0;				
	int inizioByte, fineByte;		
	int indiceInizioFile;			

	int size = 0;				
	char** parola;				
	int* countParola;			 

	countParola = (int*)malloc(sizeof(int));// ALLOCAZIONE DINAMICA PER COUNT_PAROLA E LA MATRICE PAROLA
	parola = (char**)malloc(sizeof(char*));
	parola[0] = (char*)malloc(500*sizeof(char));

	if(dir!=NULL) {		//CONTROLLO PER DIRECTORY VUOTA

		while((Dirent=readdir(dir))!=NULL) {
			count++;
			}
		fileDaLeggere = (fileS*)malloc(sizeof(fileS)*count);
			
		seekdir(dir,0);
		count = 0;
		while((Dirent=readdir(dir))!=NULL) { 
			if(Dirent->d_type==8) {
				char stringa[100] ="file/";
				strcpy(fileDaLeggere[count].nome,stringa);
				strcat(fileDaLeggere[count].nome,Dirent->d_name);
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
In questa fase avviene per prima cosa il calcolo in byte della dimensione dell'input. Il calcolo viene effetuato in modo tale da conoscere la dimensione su cui andare a lavorare, in questo modo il lavoro verrà suddiviso in byte. Questo lavoro viene svolto da tutti i nodi del cluster, in pratica ogni nodo si calcola la taglia dell'input totale, dopodichè si calcola la propria porzione di lavoro sempre in byte, su quale file deve iniziare a leggere, e da quale byte deve partire.  Si è scelto di far effettuare questo calcolo ad ogni nodo in modo da eliminare la comunicazione per quanto riguarda il partizionamento, siccome in termini di overhead la comunicazione è più dispendiosa rispetto la computazione, ad esempio un'altra possibile scelta poteva essere di far computare il partizionamento al nodo MASTER e alla fine del lavoro quest'ultimo comunicava tutto agli altri nodi.
### 2.2 Word Count
Word Count è la funzione che svolge il compito principale del problema, cioè contare le occorrenze delle parole. Prima di passare al commento della funzione Word Count è meglio commentare alcune funzioni che vengono usate all'interno e che stutture dati sono state utilizzate. Come stuttura dati per salvare le occorrenze si è scelto di utilizzare un array di interi per quanto riguarda le frequenze e una matrice di char per memorizzare le parole. Le due strutture vengono allocate dinamicamente e poi deallocate alla fine del lavoro. Per quanto riguarda invece la lettura delle stringhe all'interno dei file si è utilizzata la seguente funzione.
\`\`\`C
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
