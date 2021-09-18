// Evan Smedley 101148695
// Trong Nguyen 100848232

#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>

#include "message_queue.h"
#include "structs.h"

// Create message queue
int msgq_create(key_t key) {
    int msgqid = msgget(key, IPC_CREAT | 0600);
    if (msgqid == -1) {
        fprintf(stderr, "msgget failed\n");
        exit(EXIT_FAILURE);
    }
    return msgqid;
}

// Send message to queue
void msgq_send(int msgqid, void *send_buffer, int message_size) {
    if (msgsnd(msgqid, send_buffer, message_size, IPC_NOWAIT) == -1) {
        fprintf(stderr, "msgsnd failed\n");
        exit(EXIT_FAILURE);
    }
}

// Receive message from queue
void msgq_receive(int msgqid, void *receive_buffer, int message_size, int message_type) {
    if (msgrcv(msgqid, receive_buffer, message_size, message_type, 0) == -1) {
        fprintf(stderr, "msgrcv failed\n");
        exit(EXIT_FAILURE);
    }
}

// Delete message queue
void msgq_delete(int msgqid) {
    if (msgctl(msgqid, IPC_RMID, 0) == -1) {
        fprintf(stderr, "msgctl failed\n");
    }
}