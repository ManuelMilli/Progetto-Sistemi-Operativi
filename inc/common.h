#ifndef _COMMON_HH
#define _COMMON_HH

#include <stdint.h>
#include <sys/ipc.h>
#include <unistd.h>

#define COMMON_IPC_KEY 69420

typedef struct {
    long mtype;
    key_t memory_key;
    ssize_t filesize;
} initial_message_t;

typedef struct {
    long mtype;
    uint8_t hash[32];
} reply_message_t;

void errExit(char *message);

char* prettifyHash(uint8_t *hash, char *buffer);

#endif
