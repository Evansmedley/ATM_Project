// Evan Smedley 101148695
// Trong Nguyen 100848232

#include <stdio.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>

#include "message_queue.h"
#include "structs.h"

int main() {
    // Create message queue
    int msgqid= msgq_create((key_t) 1224);

    // Get access to state message queue
    int state_msgqid = msgq_create((key_t) 8888);

    // Create struct to send every 60 seconds
    msg_t send_buffer;
    send_buffer.msg_type = 5;
    state_msg_t state_buffer;
    state_buffer.msg_type = 1;

    // Update states of the system
    strcpy(state_buffer.message, "interest -> Online");
    msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));
    strcpy(state_buffer.message, "interest: INTEREST message sent");

    // Interest calculate every minute
    while (true) {
        sleep(60);
        msgq_send(msgqid, (void *)&send_buffer, 0);
        msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));
    }
}