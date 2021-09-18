// Evan Smedley 101148695
// Trong Nguyen 100848232

#ifndef SEMUN_H
#define SEMUN_H

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} arg;

#endif