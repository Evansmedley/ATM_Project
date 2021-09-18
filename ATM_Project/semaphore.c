// Evan Smedley 101148695
// Trong Nguyen 100848232

#include <sys/sem.h>
#include <stdlib.h>
#include <stdio.h>

#include "semaphore.h"
#include "semun.h"

// Create semaphore
int sem_create(key_t key) {
    int semid = semget(key, 1, 0666 | IPC_CREAT);
    if (semid == -1) {
        fprintf(stderr, "semget failed\n");
        exit(EXIT_FAILURE);
    }
    return semid;
}

// Initialize semaphore
void sem_initialize(int semid) {
    union semun union_semun;
    union_semun.val = 1;
    if (semctl(semid, 0, SETVAL, union_semun) == -1) {
        fprintf(stderr, "Failed to initialize semaphore\n");
    }
}

// Acquire wait semaphore
void sem_acquire(int semid) {
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = -1;
    sem_b.sem_flg = SEM_UNDO;

    if (semop(semid, &sem_b, 1) == -1) {
        fprintf(stderr, "semaphore wait operation failed\n");
        exit(EXIT_FAILURE);
    }
}

// Check semaphore value
int sem_getval(int semid) {
    int semval = semctl(semid, 0, GETVAL, 0);
    if (semval == -1) {
        fprintf(stderr, "semaphore check operation failed\n");
        exit(EXIT_FAILURE);
    }
    return semval;
}

// Release semaphore
void sem_release(int semid) {
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = 1;
    sem_b.sem_flg = SEM_UNDO;

    if (semop(semid, &sem_b, 1) == -1) {
        fprintf(stderr, "semaphore release operation failed\n");
        exit(EXIT_FAILURE);
    }
}

// Delete semaphore
void sem_delete(int semid) {
    union semun del_sem_union;
    if (semctl(semid, 0, IPC_RMID, del_sem_union) == -1) {
        fprintf(stderr, "semaphore deletion failed\n");
        exit(EXIT_FAILURE);
    }
}