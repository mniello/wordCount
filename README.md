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
```
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
```
In pratica viene letto un carattere alla volta e scartati i caratteri non alfanumerici di una stringa.
Per l'ordinamento, che viene eseguito solo dal Master una volta che ha compattato tutte le occorrenze calcolate dagli n nodi, è stata utilizzata la seguente funzione. L'ordinamento avviene in base al numero di occorrenze in modo decrescente.
```
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
```
Infine viene utilizzata una funzione che viene eseguita solo dal Master per salavare tutto il lavoro e quindi le occorrenze delle parole in un file csv.
```
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
```
Esiste ancora un'altra funzione che viene utilizzata, questa però verrà commentata in seguito con la funzione Word Count che è la seguente.
```
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
	
	int partenza = 0;				
	int indice = indiceInizioFile;			 
	int byteLetti = 0;				
	int byte = 0;					 
	int byteDaLeggere = fineByte - inizioByte;	
	bool endFile = false;				
	FILE* file;					
	
	int backWordControl = 1;			
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
				
				
				//CALCOLO DEI BYTE LETTI
				if(indice == indiceInizioFile) { byte = ftell(file) - partenza; }
				else { byte = ftell(file) + byteLetti - partenza; }
				
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
    		}
    	}
    	char* recvbuf = (char*)malloc(sizeof(char)*bufSizeRecv);
    	
	
	//ONGI PROCESSO INVIA AL RANK 0 LE DUE STRUTTURE  
	MPI_Gatherv(message, bufsiz, MPI_PACKED, recvbuf, recvcounts, displs , MPI_PACKED, 0, MPI_COMM_WORLD);
	
	free(countParola);
	for(int i = 0;i<*size;i++) {
		free(parola[i]);	
	}
	free(parola);
	

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
```
La funzione WordCount come detto prima è la funzione principale del programma, in cui viene svolto lavoro in parallelo, lavoro effettuato solo dal Master e la comunicazione con MPI tra i nodi del cluster. Alla fine del lavoro di partizionamento ogni nodo del cluster conosce quanti byte deve leggere, da quale byte deve iniziare e da quale file deve partire. Infatti vengono utilizzate queste due informazioni memorizzate nelle variabili "byteDaLeggere" e "indiceInizioFile" nel primo ciclo while. In questo ciclo ogni nodo del cluster inizia a leggere dal proprio file assegnato fino a quando non esaurisce i byte oppure arriva alla fine del file e quindi ne apre uno nuovo. Ogni volta ovviamente per leggere una stringa ogni nodo chiama la funzione "takeAndCheckString". Partizionando il lavoro in byte ovviamente si crea un problema, cioè può capitare che un nodo inizia a leggere all'interno di una stringa e quindi il conteggio delle parole non sara più preciso. Per risolvere questo problema viene utilizzata la funzione "backWord" ma non solo. In pratica quando un nodo arriva alla fine dei sui byte si controlla se riesce a leggere interamente la sua ultima parola oppure si ferma prima. Se si ferma prima in pratica il nodo non memorizza quella parola quindi non la conta, ma passa il lavoro al nodo che viene dopo. La funzione "backWord" viene eseguita una sola volta da ogni nodo e controlla se il nodo si trova all'interno di una parola, se questo succede significa che il nodo precedente non ha memorizzato la parola, quindi bisogna tornare indietro all'inizio della stringa e partire da quel punto.
```
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
```
Una volta che un nodo finisce il suo lavoro quindi arriva alla fine dei suoi byte ed ha sicuramente memorizzato il tutto nelle due strutture commentate prima, è pronto ad inviare il tutto al nodo MASTER. Quindi dopo la lettura e il conteggio delle parole avviene la fase di comunicazione con MPI.
Per comunicare è stata utilizzata la comunicazione collettiva e quindi le funzioni MPI-Gather ed MPI-Gatherv. Ogni nodo prima di inviare le due strutture al MASTER, con la MPI-Gather invia prima  la dimensione di queste siccome la dimensione è variabile per ognuno. Dopo aver inviato la dimensione delle strutture, ogni nodo effettua il Pack di queste, in pratica invece di inviare le due strutture si invia un buffer contiguo e viene creato quindi un nuovo tipo di dato.
```
for(int i = 0;i<*size;i++) {
	MPI_Pack(parola[i],500,MPI_CHAR,message,bufsiz,&position,MPI_COMM_WORLD);
	MPI_Pack(&countParola[i],1,MPI_INT,message,bufsiz,&position,MPI_COMM_WORLD);
}
```
Una volta effettuato il Pack  ogni nodo invia il nuovo tipo di dato con la MPI-Gatherv, utilizzata siccome la dimensione del nuovo tipo di dato può variare per ogni nodo.
Il nodo Master in ricezione salverà il tutto in un buffer contiguo che dovrà scompattare con la funzione MPI-Unpack. Scompattando il buffer il MASTER ricrea le due strutture, una per il conteggio delle parole e una per le parole. A questo punto il Master avrà a disposizione il lavoro svolto da tutti i nodi incluso il suo. Infine dovrà riorganizzare il conteggio siccome i nodi potrebbero aver contato le stesse parole in file differenti, quindi crea altre due strutture uguali alle precedenti ed effettua la funzione di "Reduce". Finito il seguente lavoro, con le funzioni commentate precedentemente effettua sia l'ordinamento che il salvataggio delle occorrenze in un file CSV.
## 3.Benchmark
La soluzione proposta è stata testata su Amazon Aws dove è stato istanziato un Cluster. Prima di commentare i Test effettuati, sarà descritta l'architettura del Cluster e il tipo di Input utilizzato per il test.
### 3.1 Architettura
IL Cluster è stato istanziato su Aws e comprende 8 istanze tutte dello stesso tipo. Le istanze erano del tipo m4.large ed hanno la seguente architettura hardware:
- 2vCPU
- 8GB RAM;
- 450 Mbs bandwidth
- 25 GB storage EBS
La configurazione software è invece composta da Ubuntu® 18.04.
### 3.2 Scalabilità forte
In questo tipo di test l'input rimane fisso ma vengono incrementati il numero di nodi del cluster su cui eseguire il programma. Il test aiuta a capire come il programma reagisce all'aumento del numero di nodi, quindi sono state utilizzate due metriche: Speedup ed efficienza. Inoltre è importante sottolineare che sono stati effettuati 5 test per numero di istanza.
Lo speedup nella scalabilà forte si calcola nel seguente modo:

*Sia n il numero di nodi, t1 il tempo di esecuzione per 1 nodo
e tn il tempo per n nodi, allora lo Strong Scalability Speedup è (t1/tn)*

Per quanto riguarda l'efficienza il calcolo è il seguente:

*Sia n il numero di nodi, t1 il tempo di esecuzione per 1 nodo
e tn il tempo per n nodi, allora la Strong Scalability Efficiency è (t1/(n tn))*

Per quanto rigurda l'input, come detto prima rimane lo stesso al variare del numero di nodi. Sono stati utilizzati 13 file che hanno la rispettiva taglia:
- Alice's adventures in wonderland (147 KB)
- PETER PAN (256 KB)
- The Wonderful Wizard of Oz  (203 KB)
- file1.txt (255 KB)
- file2.txt (255 KB)
- file3.txt (255 KB)
- file4.txt (255 KB)
- file5.txt (255 KB)
- file6.txt (255 KB)
- file7.txt (255 KB)
- file8.txt (255 KB)
- file9.txt (255 KB)
- file10.txt (255 KB)

I risultati ottenuti vengono mostrati nei seguenti grafici:
![Alt text](/img/StrongSpeedUp.png?raw=true "Title")
Dalla figura si può vedere che con 16 vCPU si è ottenuto uno speedUp di 4,44, un risultato per niente ottimale. Probabilmente il risultato è dovuto non dal tempo speso per la comunicazione ma per il tempo speso a computare, sopratutto nella parte iniziale e finale del programma, dove vengono effettuate le fasi di partizionamento del lavoro e raggruppamento delle occorrenze.
![Alt text](/img/EfficienzaStrong.png?raw=true "Title")
Nella seguente figura è possibile vedere che l'efficienza migliore con l'aumentare delle vCPU si raggiune con 4vCPU. Con l'aumentare dei nodi l'efficienza cala drasticamente, questo fa capire che aumentando il numero di nodi l'overhead della comunicazione in questo caso può comportare ad un calo delle prestazioni.
![Alt text](/img/MediaStrong.png?raw=true "Title")
La figura mostra i tempi di esecuzione al variare delle vCPU. Ovviamente il tempo di esecuzione dovrebbe essere minore aggiungendo unità di elaborazione. In questo caso si ha che con 16 vCPU il programma diventa 5 volte più veloce.
### 3.3 Scalabilità debole
In questo tipo di test l'input viene incrementato in maniera costante al variare dei nodi che vengono utilizzati. Questo tipo di test ci aiuta a capire se il programma richiede molta memoria e un elevato numero di risorse. Si ottiene scalabilità debole lineare se il tempo di esecuzione rimane costante mentre il carico di lavoro viene incrementato con il numero di nodi. In questo test è stata utilizzata solo la metrica di efficienza e sono stati effettuati anche questa volta 5 test per numero di istanza.
L'efficienza nella scalabilità debole è calcolata nel seguente modo:

*Sia n il numero di nodi, t1 il tempo di esecuzione per 1 nodo
e tn il tempo per n nodi, allora la Weak Scalability Efficiency è (t1/tn)*

Per quanto riguarda l'input è stato utilizzato un solo file, cioè "PETER PAN" (256 KB) che veniva copiato in maniera costante al variare delle vCPU utilizzate. Ad esempio utilizzando 5 vCPU l'input è composto da 5 file "PETER PAN". I risultati ottenuti sono i seguenti:
![Alt text](/img/efficienzaWeak.png?raw=true "Title")
Dalla figura si può notare come il programma si comporta male in termini di efficienza per quanto riguarda la scalabilità debole. Il calo è drastico sopratutto dopo 4vCPU, probabilmente anche in questo caso la computazione iniziale e finale generano molto overhead sopratutto in temrini di utilizzo di risorse e memoria.
![Alt text](/img/mediaWeak.png?raw=true "Title")
Nella figura si può vedere come il tempo di esecuzione aumenta con l'aumentare del carico di lavoro e numero di vCPU. Un risultato aspettato, siccome da come si è visto in precedenza, in termini di efficienza il programma non si comportava bene. Infatti con 16 vCPU il tempo di esecuzione è 7 volte più lento, da considerare però che il carico passa da 250 KB a 4MB.
## 4.Conclusioni
