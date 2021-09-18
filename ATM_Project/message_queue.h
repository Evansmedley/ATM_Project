// Evan Smedley 101148695
// Trong Nguyen 100848232

#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H

#include <sys/msg.h>

int msgq_create(key_t key);
void msgq_send(int msgqid, void *send_buffer, int message_size);
void msgq_receive(int msgqid, void *receive_buffer, int message_size, int message_type);
void msgq_delete(int msgqid);

#endif