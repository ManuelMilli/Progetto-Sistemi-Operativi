#ifndef _SEMAPHORES_HH
#define _SEMAPHORES_HH

#include <sys/sem.h>

typedef struct sembuf sembuf_t;

typedef struct semid_ds semds_t;

typedef union semun {
  int val;
  semds_t* buf;
  unsigned short* array;
} semun_t;

void errExit(char *message);

int sem_set(int sem, int semnum, int sem_op, int wait);
int sem_p(int sem, int semnum, int wait);
int sem_v(int sem, int semnum, int wait);

#endif
