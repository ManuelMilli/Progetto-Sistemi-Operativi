#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "inc/common.h"
#include "inc/semaphores.h"

void hashFile(char *filePath) {
    srand(time(NULL)); //inizializzo il generatore di numeri casuali con il tempo corrente

    initial_message_t imsg;
    imsg.request_id = rand(); //genero un id di richiesta casuale
    imsg.memory_key = ftok(filePath, 3); //3 valore a caso diverso da 0, ftok genera una chiave per il file

    struct stat sb; //leggo le stat del file, mi interessa la dimensione (riga 25)
    if(lstat (filePath, &sb) == -1){
        errExit("errore nella lettura del file");
    }
    imsg.mtype = sb.st_size; //il tipo del messaggio è la dimensione del file

    //recupero le code di messaggi
    int reqQueueId = msgget(COMMON_IPC_KEY, 0); //id della message queue delle richieste
    if(reqQueueId == -1)
        errExit("impossibile recuperare la coda di messaggi per le richieste");
    int replyQueueId = msgget(COMMON_IPC_KEY + 1, 0); //id della message queue delle risposte
    if(replyQueueId == -1)
        errExit("impossibile recuperare la coda di messaggi per le risposte");

    //creo la memoria condivisa
    int memid = shmget(imsg.memory_key, sb.st_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); //id memoria condivisa in cui salviamo il contenuto del file da criptare
    if(memid == -1)
        errExit("impossibile creare la memoria condivisa");

    void *filebuf = shmat(memid, NULL, 0); //puntatore all'area di memoria condivisa allocata
    if(filebuf == -1)
        errExit("impossibile montare la memoria condivisa");

    int file = open(filePath, O_RDONLY, 0); //apertura del file in sola lettura
    if (file == - 1)
        errExit("file non trovato");

    //leggo il file e lo copio nell'area di memoria condivisa
    ssize_t fileoffset = 0;
    while(fileoffset != sb.st_size){
        ssize_t read_result = read(file, filebuf + fileoffset, sb.st_size - fileoffset);
        if (read_result == -1)
            errExit("Error");
        fileoffset += read_result;
    }

    //messaggio inviato al server con la chiave dell'area di memoria in cui viene salvato il file
    size_t mSize = sizeof(initial_message_t) - sizeof(long); //calcolo da dimensione del messaggio
    if (msgsnd(reqQueueId, &imsg, mSize, 0) == -1) //invio del messaggio al server
        errExit("msgsnd failed\n");

    //messaggio di risposta con l'hash fatto dal server
    reply_message_t rmsg;
    mSize = sizeof(reply_message_t) - sizeof(long); //calcolo la dim del messaggio
    if (msgrcv(replyQueueId, &rmsg, mSize, imsg.request_id, 0) == -1) //ricevo il messaggio
        errExit("msgrcv failed");

    //printo l'hash come stringa di esadecimali
    char char_hash[65];
    prettifyHash(rmsg.hash, char_hash); //converto l'hash in stringa di esadecimali
    printf("%s\n", char_hash);
}

void changeProcs(int newMax) {
    if (newMax < 1)
        errExit("Il numero massimo di processi non può essere inferiore a 1");

    int maxProcSemId = semget(COMMON_IPC_KEY, 1, 0); //recupero il semaforo per il controllo del numero di processi
    if(maxProcSemId == -1)
        errExit("impossibile recuperare il semaforo per il controllo del numero di processi");

    sem_p(maxProcSemId, 1, 1); //attendo che la memoria condivisa sia disponibile

    int maxProcMemId = shmget(COMMON_IPC_KEY, sizeof(int), 0); //recupero l'id della memoria condivisa
    if(maxProcMemId == -1)
        errExit("impossibile recuperare la memoria condivisa per il numero massimo di processi");

    int *maxProc = (int *)shmat(maxProcMemId, NULL, 0); //collego la memoria condivisa
    if(maxProc == (void *)-1)
        errExit("impossibile montare la memoria condivisa per il numero massimo di processi");

    int semOp = newMax - *maxProc; //calcolo l'operazione da fare sul semaforo
    sem_set(maxProcSemId, 0, semOp, 1); //aggiorno il semaforo con il valore di delta
    *maxProc = newMax; //aggiorno il valore della memoria condivisa con il nuovo numero massimo di processi

    sem_v(maxProcSemId, 1, 1); //rilascio il semaforo per il controllo del numero di processi

    printf("Operazione completata. Il numero massimo di processi server è ora %d", newMax);
}

int main(int argc, char *argv[]) {
    //controllo che siano stati passati 3 paramentri a riga di comando
    //nome eseguibile + comando + parametro
    if(argc != 3)
        errExit("Utilizzo: ./client <command> <parameter>\n"
                "Comandi:\n"
                "  hashfile <file_path> - Calcola l'hash del file specificato\n"
                "  changeprocs <new_max_procs> - Cambia il numero massimo di processi server\n");

    if (strcmp(argv[1], "hashfile") == 0)
        hashFile(argv[2]);
    if (strcmp(argv[1], "changeprocs") == 0)
        changeProcs(atoi(argv[2]));
}
