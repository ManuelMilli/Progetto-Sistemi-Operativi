#include <stdio.h>
#include <sys/sem.h>
#include <stdlib.h>

#include "common.h"

void errExit(char *message){
    printf("%s", message);
    exit(1); //esci dal programma con errore
}

int sem_set(int sem, int semnum, int sem_op, int wait) {
  short flag;
  if (wait)
    flag = 0;
  else
    flag = IPC_NOWAIT;
  
  sembuf_t curr_op;
  curr_op.sem_num = semnum;
  curr_op.sem_op = sem_op;
  curr_op.sem_flg = flag;

  return semop(sem, &curr_op, 1);
}

int sem_p(int sem, int semnum, int wait) {
  short flag;
  if (wait)
    flag = 0;
  else
    flag = IPC_NOWAIT;
  
  sembuf_t curr_op;
  curr_op.sem_num = semnum;
  curr_op.sem_op = -1;
  curr_op.sem_flg = flag;

  return semop(sem, &curr_op, 1);
}

int sem_v(int sem, int semnum, int wait) {
  short flag;
  if (wait)
    flag = 0;
  else
    flag = IPC_NOWAIT;
  
  sembuf_t curr_op;
  curr_op.sem_num = semnum;
  curr_op.sem_op = +1;
  curr_op.sem_flg = flag;

  return semop(sem, &curr_op, 1);
}