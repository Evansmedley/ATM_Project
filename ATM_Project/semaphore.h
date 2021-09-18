// Evan Smedley 101148695
// Trong Nguyen 100848232

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <sys/sem.h>

int sem_create(key_t key);
void sem_initialize(int semid);
void sem_acquire(int semid);
int sem_getval(int semid);
void sem_release(int semid);
void sem_delete(int semid);

#endif