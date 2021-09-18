// Evan Smedley 101148695
// Trong Nguyen 100848232

#ifndef STRUCTS_H
#define STRUCTS_H
#define NUM_LOCKS 10

typedef struct msg {
    long int msg_type;
    char account_no[7];
    char pin[5];
    float amount;
} msg_t;

typedef struct account_modification {
    int operation;
    int num_accounts;
    char account_no[7];
    char pin[5];
    float amount;
} account_t;

typedef struct state_message {
    long int msg_type;
    char message[100];
} state_msg_t;

typedef struct account_locks {
    int locks[NUM_LOCKS * 2];
} locks_t;

#endif