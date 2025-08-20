#ifndef _COMMON_HH
#define _COMMON_HH
#include <stdint.h>

typedef struct {
    long mtype;
    key_t memory_key;
    ssize_t filesize;
} initial_message_t;

typedef struct {
    long mtype;
    uint8_t hash[32];
} reply_message_t;

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