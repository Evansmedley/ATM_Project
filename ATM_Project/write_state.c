// Evan Smedley 101148695
// Trong Nguyen 100848232

#include <stdio.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <unistd.h>
#include <assert.h>

#include "message_queue.h"
#include "structs.h"

#define STATE_FILE "system_state.txt"

int main() {
    // Get access to state message queue
    int state_msgqid = msgq_create((key_t) 8888);

    // Erase previous state information in STATE_FILE
    FILE *f1 = fopen(STATE_FILE, "w");
    assert(f1 != NULL);
    fprintf(f1, "write_state -> Online\n");
    fclose(f1);

    // Create send buffer
    state_msg_t buffer;

    // Wait for message queue message to write state message
    while(true) {
        msgq_receive(state_msgqid, (void *)&buffer, sizeof(buffer.message), 1);
        FILE *f1 = fopen(STATE_FILE, "a");
        assert(f1 != NULL);
        fprintf(f1, "\n%s\n", buffer.message);
        fclose(f1);
    }
}