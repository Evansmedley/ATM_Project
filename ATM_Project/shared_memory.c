// Evan Smedley 101148695
// Trong Nguyen 100848232

#include <sys/shm.h>
#include <stdlib.h>
#include <stdio.h>

// Create shared memory
int shm_create(key_t key, int size) {
    int shmid = shmget(key, size, IPC_CREAT | 0666);
    if (shmid == -1) {
        fprintf(stderr, "shmget failed\n");
        exit(EXIT_FAILURE);
    }
    return shmid;
}

// Access shared memory
void *shm_access(int shmid) {
    void *shared_memory = shmat(shmid, (void *)0, 0);
    if (shared_memory == (void *)-1) {
        fprintf(stderr, "shmat failed\n");
        exit(EXIT_FAILURE);
    }
    return shared_memory;
}

// Detach shared memory
void shm_detach(void *shared_memory) {
    if (shmdt(shared_memory) == -1) {
        fprintf(stderr, "shmdt failed \n");
        exit(EXIT_FAILURE);
    }
}

// Delete shared memory
void shm_delete(int shmid) {
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
        fprintf(stderr, "shmctl(IPC_RMID) failed \n");
        exit(EXIT_FAILURE);
    }
}