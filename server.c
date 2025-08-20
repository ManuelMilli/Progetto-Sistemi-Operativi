#include <stdio.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <openssl/sha.h>
#include <signal.h>
#include <sys/sem.h>

#include "common.h"

int msqid;
int maxProcSemId;
int maxProcMemId;

//gestione del ctrl c (chiusura del server)
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


void sha256(void *filebuf, ssize_t filesize, uint8_t *hash){
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    
    SHA256_Update(&ctx, (uint8_t *)filebuf, filesize);
    SHA256_Final(hash, &ctx);
}

int main(){
    maxProcSemId = semget(54, 1, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

    maxProcMemId = shmget(235, sizeof(int), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

    int *maxProc = (int *)shmat(maxProcMemId, NULL, 0); //puntatore all'area di memoria condivisa allocata
    if(maxProc == -1)
        errExit("impossibile montare la memoria condivisa");
    *maxProc = 3;

    union semun arg;
    arg.val = *maxProc;
    if (semctl(maxProcSemId, 0, SETVAL, arg) == -1)
        errExit("semctl SETVAL");

    initial_message_t imsg;
    msqid = msgget(69, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); //crea la coda di mess a cui si collegherà anche il client
    if(msqid == -1)
        errExit("impossibile creare la coda di messaggi");

    setup_signals();

    size_t mSize = sizeof(initial_message_t) - sizeof(long); //calcolo la dim del messaggio

    while(1){
        if (msgrcv(msqid, &imsg, mSize, 1, 0) == -1) //ricevo il messaggio
            errExit("msgrcv failed");

        sem_p(maxProcSemId, 0, 1);
        if(fork() == 0)
            break;
    }
    
    int memid = shmget(imsg.memory_key, imsg.filesize, 0); //mi collego alla memoria condivisa creata dal client
    if(memid == -1)
        errExit("impossibile recuperare la memoria condivisa");

    void *filebuf = shmat(memid, NULL, 0);
    if(filebuf == -1)
        errExit("impossibile montare la memoria condivisa");

    reply_message_t rmsg; //creo il messaggio di risposta
    rmsg.mtype = 2; //definisco un nuovo id per il messaggio di risposta diverso da quello di invio
    sha256(filebuf, imsg.filesize, rmsg.hash); //faccio l'hashing del contenuto del file che troverò in hash
    
    printf("attesa...");
    char maracas;
    scanf("%c", &maracas);
    
    mSize = sizeof(reply_message_t) - sizeof(long); //calcolo la dim del messaggio
    if(msgsnd(msqid, &rmsg, mSize, 0) == -1) //invio del messaggio al client
        errExit("msgsnd failed\n");

    shmctl(memid, IPC_RMID, NULL); //elimino la memoria condivisa
    sem_v(maxProcSemId, 0, 1);
}