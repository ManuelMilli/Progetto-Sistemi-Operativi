#include <stdio.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <openssl/sha.h>
#include <signal.h>

#include "inc/common.h"
#include "inc/semaphores.h"

int msqid;
int maxProcSemId;
int maxProcMemId;

//gestione del C-c (chiusura del server)
void sig_handler(int sig) {
    if (sig == SIGINT) {
        printf("Disconnecting now...\n");
        msgctl(msqid, IPC_RMID, NULL); //elimino la coda di messaggi
        shmctl(maxProcMemId, IPC_RMID, NULL); //elimino la memoria condivisa
        semctl(maxProcSemId, 0, IPC_RMID, NULL); //elimino il semaforo
        _exit(0);
    }
}

void setup_signals() {
    sigset_t blocked_signals;

    sigfillset(&blocked_signals);
    sigdelset(&blocked_signals, SIGINT);

    if (sigprocmask(SIG_BLOCK, &blocked_signals, NULL) == -1)
        errExit("Error blocking unneeded signals");
    if (signal(SIGINT, sig_handler) == SIG_ERR)
        errExit("Error registering signal handlers");
}


void sha256(void *filebuf, ssize_t filesize, uint8_t *hash) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    SHA256_Update(&ctx, (uint8_t *)filebuf, filesize);
    SHA256_Final(hash, &ctx);
}

void setupProcessControl() {
    maxProcMemId = shmget(COMMON_IPC_KEY, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    int *maxProc = (int *)shmat(maxProcMemId, NULL, 0); //puntatore all'area di memoria con il numero massimo di processi
    if(maxProc == -1)
        errExit("impossibile montare la memoria condivisa");
    *maxProc = 1;

    union semun arg;
    maxProcSemId = semget(COMMON_IPC_KEY, 2, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); //creo il semaforo per il controllo del numero di processi
    arg.val = *maxProc; //inizializzo il semaforo con il numero massimo di processi
    if (semctl(maxProcSemId, 0, SETVAL, arg) == -1)
        errExit("Impossibile inizializzare il semaforo 1");
    arg.val = 1; //inizializzo il secondo semaforo a 1, per poter fare il P e V
    if (semctl(maxProcSemId, 1, SETVAL, arg) == -1)
        errExit("Impossibile inizializzare il semaforo 2");

    shmdt(maxProc); //al server non serve più il numero di processi massimi, lo userà solo il client
}

void setupIPC() {
    msqid = msgget(COMMON_IPC_KEY, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); //crea la coda di mess a cui si collegherà anche il client
    if(msqid == -1)
        errExit("impossibile creare la coda di messaggi");

    setupProcessControl(); //creo la memoria condivisa e il semaforo per il controllo del numero di processi
    setup_signals(); //configuro la gestione dei segnali (C-c)
}

int main() {
    #ifdef DEBUG
    printf("[DEBUG] Debug enabled. Requests will be paused.\n");
    #endif

    setupIPC(); //creo la coda di messaggi e configuro la gestione di segnali e sottoprocessi

    initial_message_t imsg;
    size_t mSize = sizeof(initial_message_t) - sizeof(long); //calcolo la dim del messaggio

    while(1){
        if (msgrcv(msqid, &imsg, mSize, 1, 0) == -1) //ricevo il messaggio
            errExit("msgrcv failed");

        sem_p(maxProcSemId, 0, 1); //attendo che ci sia spazio per un nuovo processo
        if(fork() == 0)
            break;
    }

    int memid = shmget(imsg.memory_key, imsg.filesize, 0); //mi collego alla memoria condivisa creata dal client
    if(memid == -1)
        errExit("impossibile recuperare la memoria condivisa");

    void *filebuf = shmat(memid, NULL, 0); //montaggio della memoria condivisa
    if(filebuf == -1)
        errExit("impossibile montare la memoria condivisa");

    reply_message_t rmsg; //creo il messaggio di risposta
    rmsg.mtype = 2; //definisco un nuovo id per il messaggio di risposta diverso da quello di invio
    sha256(filebuf, imsg.filesize, rmsg.hash); //faccio l'hashing del contenuto del file che troverò in hash

    #ifdef DEBUG
    printf("[DEBUG] Risposta al client in attesa...");
    char send;
    scanf("%c", &send);
    #endif

    mSize = sizeof(reply_message_t) - sizeof(long); //calcolo la dim del messaggio
    if(msgsnd(msqid, &rmsg, mSize, 0) == -1) //invio del messaggio al client
        errExit("msgsnd failed\n");

    shmctl(memid, IPC_RMID, NULL); //elimino la memoria condivisa

    sem_v(maxProcSemId, 0, 1); //segnalo che il processo è terminato e che ora c'è spazio per un nuovo processo
}
