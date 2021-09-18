// Evan Smedley 101148695
// Trong Nguyen 100848232

#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <sys/shm.h>

int shm_create(key_t key, int size);
void *shm_access(int shmid);
void shm_detach(void *shared_memory);
void shm_delete(int shmid);

#endif