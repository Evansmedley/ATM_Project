/**
 * Term Project - Concurrency, IPC, Semaphores, Shared Memory
 * 
 * Database Server
 *
 * Carleton University
 * Department of System and Computer Engineering
 * SYSC 4001 - Operating Systems
 *
 * @authors Evan Smedley, 101148695 & Trong Nguyen, 100848232
 * @version v1.00
 * @release April 27, 2021
 *
 * The database (DB) server wait for messages from other 
 * proccess and updates the DB accordingly.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "structs.h"
#include "message_queue.h"
#include "shared_memory.h"
#include "semaphore.h"

#define DB "DB.txt"
#define NEW_DB "newDB.txt"

/**
 * This function encodes the users pin by adding 1. Rather than converting to 
 * an int and then back to a string, 1 is added using characters.
 */
void encode_pin(char pin[]) {
    for (int i = 2; i != -1; i--) {
        if (pin[i] == '9') {
            pin[i] = '0';
        } else {
            pin[i]++;
            break;
        }
    }
}

void main() {
    // Create message queue
    int msgqid = msgq_create((key_t) 1224);

    // Get access to message queue for writing program state to a file
    int state_msgqid = msgq_create((key_t) 8888);

    // Get/create and attach shared memory
    int shmid = shm_create((key_t) 5050, sizeof(account_t));
    account_t *shared_memory = (account_t *) shm_access(shmid);

    // Create and initialize semaphore (for locking shared memory)
    int semid1 = sem_create((key_t) 2021);
    sem_initialize(semid1);

    // Create and initialize semaphore (for ensuring that a type 4 DB editing request
    // is processed with the correct values stored in shared memory)
    int semid2 = sem_create((key_t) 2323);
    sem_initialize(semid2);
    sem_acquire(semid2);

    // Declare/initialize variables for loop
    msg_t receive_buffer;
    msg_t temp_msg;
    msg_t send_buffer;
    char line_of_DB[30];
    char prev_acc_no[6];
    char temp_float_str[15];
    float temp_float;
    int message_size = sizeof(send_buffer.account_no) + sizeof(send_buffer.pin) + sizeof(send_buffer.amount);
    account_t account_info;
    state_msg_t state_buffer;
    state_buffer.msg_type = 1;
    int temp_semid;
    char temp_int_str[7];

    // Print state
    strcpy(state_buffer.message, "DB_server -> Online");
    msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

    int shmid_account_sem = shm_create((key_t) 4567, sizeof(locks_t));
    locks_t *account_locks = (locks_t *) shm_access(shmid_account_sem);

    // Create the semaphores and add their keys to the array in shared memory
    FILE *f2 = fopen(DB, "r");
    for (int i = 0; i < NUM_LOCKS * 2; i++) {
        if (i % 2 == 0) {
            account_locks->locks[i] = 5000 + i;
            sem_initialize(sem_create((key_t) 5000 + i));
            
        } else {
            if (!feof(f2)) {
                fgets(line_of_DB, sizeof(line_of_DB), f2);
                strncpy(prev_acc_no, line_of_DB, 5);
                prev_acc_no[5] = '\0';
                account_locks->locks[i] = atoi(prev_acc_no);
            } else {
                account_locks->locks[i] = 0;
            }   
        }
    }
    fclose(f2);
    sem_release(semid2);

    // Loop
    while(true) {

        // Receive a message
        msgq_receive(msgqid, (void *)&receive_buffer, message_size, -5);

        if (receive_buffer.msg_type == 1) {
            // The message received was a PIN message
            strcpy(state_buffer.message, "DB_server: Received a PIN message");
            msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

            // Check if the PIN message is to block an account
            if (receive_buffer.amount < 0.0) {
                // If it is, block the account

                FILE *f1 = fopen(DB, "r");
                assert(f1 != NULL);
                // To edit the file, create and open a new database text file
                FILE *f2 = fopen(NEW_DB, "w");
                assert(f2 != NULL);
                // Loop through the entire old database text file
                while(!feof(f1)) {
                    fgets(line_of_DB, sizeof(line_of_DB), f1);
                    // If the line f1 is pointing to is the one to be modified, modify it
                    if (strncmp(line_of_DB, receive_buffer.account_no, 5) == 0) {
                        line_of_DB[0] = 'X';
                    }
                    // Append the next line of the old database to the new database
                    if (strncmp(prev_acc_no, line_of_DB, 5) != 0) {
                        fprintf(f2, "%s", line_of_DB);
                    }
                    strncpy(prev_acc_no, line_of_DB, 5);
                }
                remove(DB);                   // Delete the old database
                rename(NEW_DB, DB);      // Switch the name of the new database to 'DB'
                fclose(f1);
                fclose(f2);

                // Print state
                sprintf(state_buffer.message, "DB_server: account %s blocked", receive_buffer.account_no);
                msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

            } else {
                // Search for the account number in the DB
                send_buffer.pin[0] = 'n';
                send_buffer.msg_type = 6;
                FILE *f1 = fopen(DB, "r");
                assert(f1 != NULL);
                while(!feof(f1)) {
                    fscanf(f1, "%5s,%3s,%f\n", temp_msg.account_no, temp_msg.pin, &temp_msg.amount);
                    if (strncmp(temp_msg.account_no, receive_buffer.account_no, 5) == 0) {
                        encode_pin(receive_buffer.pin);
                        if (strncmp(temp_msg.pin, receive_buffer.pin, 3) == 0) {
                            // If the pin and account number are both correct, set up send buffer for an OK message
                            send_buffer.pin[0] = 'y';
                            break;
                        }
                    }
                }
                fclose(f1);
                // Send message
                msgq_send(msgqid, (void *)&send_buffer, message_size);

                if (send_buffer.account_no[0] == 'y') {
                    strcpy(state_buffer.message, "DB_server: sent OK message");
                } else {
                    strcpy(state_buffer.message, "DB_server: sent PIN_WRONG message");
                }
                msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));
            }
        } else if (receive_buffer.msg_type == 2) {
            // The message received was a BALANCE message
            strcpy(state_buffer.message, "DB_server: Received BALANCE message");
            msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

            // Search for the account number in the DB
            FILE *f2 = fopen(DB, "r");
            assert(f2 != NULL);
            while(!feof(f2)) {
                fscanf(f2, "%5s,%3s,%f\n", send_buffer.account_no, send_buffer.pin, &send_buffer.amount);
                if (strncmp(send_buffer.account_no, receive_buffer.account_no, 5) == 0) {
                    // Send the balance
                    send_buffer.msg_type = 7;
                    
                    msgq_send(msgqid, (void *)&send_buffer, message_size);

                    // Print state
                    strcpy(state_buffer.message, "DB_server: Sent balance");
                    msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));
                    break;
                }
            }
            fclose(f2);

        } else if (receive_buffer.msg_type == 3) {
            // The message received was a WITHDRAW message
            strcpy(state_buffer.message, "DB_server: Received WITHDRAW/DEPOSIT message");
            msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

            // Search for the account number in the DB
            msg_t account_info;
            FILE *f3 = fopen(DB, "r");
            assert(f3 != NULL);
            while(!feof(f3)) {
                fscanf(f3, "%5s,%3s,%f\n", temp_msg.account_no, temp_msg.pin, &temp_msg.amount);
                if (strncmp(temp_msg.account_no, receive_buffer.account_no, 5) == 0) {
                    if (temp_msg.amount >= receive_buffer.amount) {
                        // Decrement the available funds by editing the database (same editing method as for blocking an account)

                        send_buffer.amount = temp_msg.amount - receive_buffer.amount;
                        rewind(f3);
                        FILE *f4 = fopen(NEW_DB, "w");
                        assert(f4 != NULL);
                        prev_acc_no[0] = '\0';
                        while (!feof(f3)) {
                            fgets(line_of_DB, sizeof(line_of_DB), f3);
                            if (strncmp(prev_acc_no, line_of_DB, 5) != 0) {
                                if (strncmp(line_of_DB, receive_buffer.account_no, 5) == 0) {
                                    fprintf(f4, "%.10s%.2f\n", line_of_DB, send_buffer.amount);
                                } else {
                                    fprintf(f4, "%s", line_of_DB);
                                }
                            }
                            strncpy(prev_acc_no, line_of_DB, 5);
                        }
                        fclose(f4);
                        // Delete the old database
                        remove(DB);
                        // Rename the new database to be called 'DB'
                        rename(NEW_DB, DB);

                        // Set up send buffer with a FUNDS_OK message
                        strcpy(state_buffer.message, "DB_server: WITHDRAW/DEPOSIT successful");
                        send_buffer.account_no[0] = 'y';
                    } else {
                        strcpy(state_buffer.message, "DB_server: WITHDRAW/DEPOSIT unsuccessful");
                        send_buffer.account_no[0] = 'n';
                    }
                    msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));
                    break;
                }
            }
            fclose(f3);

            // Send the message
            send_buffer.msg_type = 7;
            msgq_send(msgqid, (void *)&send_buffer, message_size);

            if (send_buffer.account_no[0] == 'y') {
                strcpy(state_buffer.message, "DB_server: Sent FUNDS_OK message");
                
            } else if (send_buffer.account_no[0] == 'n') {
                strcpy(state_buffer.message, "DB_server: Sent NSF message");
            }
            msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));


        } else if (receive_buffer.msg_type == 4) {
            // This means an UPDATE_DB message was received
            strcpy(state_buffer.message, "DB_server: Received UPDATE_DB message");
            msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

            // Check the semaphore to get access to the critical section
            sem_acquire(semid1);

            // Copy shared memory into a buffer
            account_info.operation = shared_memory->operation;
            strcpy(account_info.account_no, shared_memory->account_no);
            strcpy(account_info.pin, shared_memory->pin);
            account_info.amount = shared_memory->amount;

            // Release semaphore
            sem_release(semid1);

            if (account_info.operation == 0) {
                // Add new account entry

                // Properly 'encode' the pin by adding 1 before saving it to the database
                encode_pin(account_info.pin);

                // Open the file in append mode and find the end of the file
                FILE *f5 = fopen(DB, "a");
                assert(f5 != NULL);
                fseek(f5, 0, SEEK_END);

                // Assign this account to a semaphore
                for (int i = 1; i < NUM_LOCKS * 2; i += 2) {
                    if (account_locks->locks[i] == 0) {
                        strncpy(prev_acc_no, account_info.account_no, 5);
                        prev_acc_no[5] = '\0';
                        account_locks->locks[i] = atoi(prev_acc_no);
                        temp_semid = sem_create((key_t) account_locks->locks[i - 1]);
                        sem_initialize(temp_semid);
                        sem_acquire(temp_semid);
                        break;
                    }
                }
            
                // Write the new account information to the file
                fprintf(f5, "%5s,%3s,%.2f\n", account_info.account_no, account_info.pin, account_info.amount);
                fclose(f5);

                // Update state
                sprintf(state_buffer.message,"DB_server: Added new account information: %5s, %3s, %.2f", account_info.account_no, account_info.pin, account_info.amount);
                msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

                printf("%d\n", sem_getval(temp_semid));
                sem_release(temp_semid);
                printf("%d\n", sem_getval(temp_semid));

            } else if (account_info.operation == 1) {
                // Delete an account

                FILE *f6 = fopen(DB, "r");
                assert(f6 != NULL);
                // To edit the file, create and open a new database text file
                FILE *f7 = fopen(NEW_DB, "w");
                assert(f7 != NULL);
                // Loop through the entire old database text file
                while(!feof(f6)) {
                    fgets(line_of_DB, sizeof(line_of_DB), f6);
                    // If the line f6 is pointing to is the one to be deleted, delete it
                    if ((strncmp(line_of_DB, account_info.account_no, 5) != 0) && strncmp(line_of_DB, prev_acc_no, 5) != 0) {
                        fprintf(f7, "%s", line_of_DB);
                    } else if (strncmp(line_of_DB, prev_acc_no, 5) != 0) {
                        
                        // Look for the account lock for this account and delete it
                        for (int i = 1; i < NUM_LOCKS * 2; i += 2) {
                            strncpy(temp_int_str, line_of_DB, 5);
                            if (account_locks->locks[i] == atoi(temp_int_str)) {
                                temp_semid = sem_create((key_t) account_locks->locks[i -1]);
                                if (sem_getval(temp_semid) == 0) {
                                    printf("\nAn ATM is currently using this account\nThis is a case of deadlock until the ATM proceeds\n");
                                    // Print state
                                    strcpy(state_buffer.message, "DB_editor tried to edit/delete an account that an ATM is currently using");
                                    msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));
                                }
                                sem_acquire(temp_semid);
                                account_locks->locks[i] = 0;
                                sem_release(temp_semid);
                                break;
                            }
                        }
                    }
                    strncpy(prev_acc_no, line_of_DB, 5);
                }
                fclose(f6);
                fclose(f7);
                remove(DB);
                rename(NEW_DB, DB);

                sprintf(state_buffer.message, "DB_server: Deleted account %s", account_info.account_no);
                msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

            } else if (account_info.operation == 2) {
                // Unblock an account
                
                prev_acc_no[0] = '\0';

                FILE *f6 = fopen(DB, "r");
                assert(f6 != NULL);
                // To edit the file, create and open a new database text file
                FILE *f7 = fopen(NEW_DB, "w");
                assert(f7 != NULL);
                // Loop through the entire old database text file
                while(!feof(f6)) {
                    fgets(line_of_DB, sizeof(line_of_DB), f6);
                    // If the line f6 is pointing to is the one to be modified, modify it

                    if (strncmp(line_of_DB, account_info.account_no, 5) == 0) {
                        line_of_DB[0] = '0';
                    }
                    if (strncmp(line_of_DB, prev_acc_no, 5) != 0) {
                        fprintf(f7, "%s", line_of_DB);
                    }
                    strncpy(prev_acc_no, line_of_DB, 5);
                }
                fclose(f6);
                fclose(f7);
                remove(DB);
                rename(NEW_DB, DB);

                sprintf(state_buffer.message, "DB_server: Unblocked account %s", account_info.account_no);
                msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

            } else {
                strcpy(state_buffer.message, "DB_server: Invalid UPDATE_DB operation type");
                msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));
                exit(EXIT_FAILURE);
            }
            // Release the semaphore, the DB_editor can now request another modification
            sem_release(semid2);

        } else if (receive_buffer.msg_type == 5) {
            // Received an INTEREST message
            strcpy(state_buffer.message, "DB_server: Received INTEREST message");
            msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

            prev_acc_no[0] = '\0';

            FILE *f6 = fopen(DB, "r");
            assert(f6 != NULL);
            // To edit the file, create and open a new database text file
            FILE *f7 = fopen(NEW_DB, "w");
            assert(f7 != NULL);
            // Loop through the entire old database text file
            while(!feof(f6)) {
                fgets(line_of_DB, sizeof(line_of_DB), f6);
                strcpy(temp_float_str, strrchr(line_of_DB, ',') + 1);

                // Get rid of the newline that was copied
                for (int i = 0; i < sizeof(temp_float_str); i++) {
                    if (temp_float_str[i] == '\n') {
                        temp_float_str[i] = '\0';
                    }
                }
                temp_float = atof(temp_float_str);
                if (strncmp(line_of_DB, prev_acc_no, 5) != 0) {
                    if (temp_float < 0.00) {
                        fprintf(f7, "%.10s%.2f\n", line_of_DB, temp_float * 1.02);
                    } else {
                        fprintf(f7, "%.10s%.2f\n", line_of_DB, temp_float * 1.01);
                    }
                }
                strncpy(prev_acc_no, line_of_DB, 5);
            }
            fclose(f6);
            fclose(f7);
            remove(DB);
            rename(NEW_DB, DB);
        }
    }
}