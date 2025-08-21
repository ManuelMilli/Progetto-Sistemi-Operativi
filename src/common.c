#include <stdio.h>
#include <stdlib.h>

#include "inc/common.h"

void errExit(char *message){
    printf("%s\n", message);
    exit(1); //esci dal programma con errore
}

char* prettifyHash(uint8_t *hash, char *buffer) {
    for (int i = 0; i < 32; i++) {
        sprintf(buffer + (i * 2), "%02x", hash[i]);
    }
    buffer[64] = '\0'; //termina la stringa
    return buffer;
}
