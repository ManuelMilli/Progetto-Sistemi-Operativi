#include <stdio.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "inc/common.h"

int main(int argc, char *argv[]){
    //controllo che siano stati passati 2 paramentri a riga di comando
    //nome eseguibile + file da criptare
    if(argc != 2)
        errExit("file non specificato");

    initial_message_t imsg;
    imsg.mtype = 1; //tipo del messaggio filtrato (messaggio iniziale)
    imsg.memory_key = ftok(argv[1], 3); //3 valore a caso diverso da 0, ftok genera una chiave per il file

    struct stat sb; //leggo le stat del file, mi interessa la dimensione (riga 25)
    if(lstat (argv[1], &sb) == -1){
        errExit("errore nella lettura del file");
    }
    imsg.filesize = sb.st_size; //passo anche la dimensione del file nel messaggio al server

    //recupero la coda di messaggi
    int msqid = msgget(COMMON_IPC_KEY, 0); //id della message queue
    if(msqid == -1)
        errExit("impossibile recuperare la coda di messaggi");

    //creo la memoria condivisa
    int memid = shmget(imsg.memory_key, sb.st_size, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR); //id memoria condivisa in cui salviamo il contenuto del file da criptare
    if(memid == -1)
        errExit("impossibile creare la memoria condivisa");

    void *filebuf = shmat(memid, NULL, 0); //puntatore all'area di memoria condivisa allocata
    if(filebuf == -1)
        errExit("impossibile montare la memoria condivisa");

    int file = open(argv[1], O_RDONLY, 0); //argv[1] è il file da criptare
    if (file == - 1) {
        errExit("file non trovato");
    }

    ssize_t fileoffset = 0;
    while(fileoffset != sb.st_size){
        ssize_t read_result = read(file, filebuf + fileoffset, sb.st_size - fileoffset);
        if (read_result == -1)
            errExit("Error");
        fileoffset += read_result;
    }

    //messaggio inviato al server con la chiave dell'area di memoria in cui viene salvato il file
    size_t mSize = sizeof(initial_message_t) - sizeof(long); //calcolo da dimensione del messaggio
    if (msgsnd(msqid, &imsg, mSize, 0) == -1) //invio del messaggio al server
        errExit("msgsnd failed\n");

    //messaggio di risposta con l'hash fatto dal server
    reply_message_t rmsg;
    mSize = sizeof(reply_message_t) - sizeof(long); //calcolo la dim del messaggio
    if (msgrcv(msqid, &rmsg, mSize, 2, 0) == -1) //ricevo il messaggio
        errExit("msgrcv failed");

    //printo l'hash come stringa di esadecimali
    char char_hash[65];
    prettifyHash(rmsg.hash, char_hash); //converto l'hash in stringa di esadecimali
    printf("%s\n", char_hash);
}
