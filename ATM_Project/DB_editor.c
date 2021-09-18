/** 
 * Term Project - Concurrency, IPC, Semaphores, Shared Memory
 * 
 * Database Editor
 *
 * Carleton University
 * Department of System and Computer Engineering
 * SYSC 4001 - Operating Systems
 *
 * @authors Evan Smedley, 101148695 & Trong Nguyen, 100848232
 * @version v1.00
 * @release April 27, 2021
 *
 * The database (DB) editor allows adminstrators of the 
 * DB to modify, add, delete, and unblock accounts. 
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "structs.h"
#include "semaphore.h"
#include "shared_memory.h"
#include "message_queue.h"


/*
 * Check if user input is a valid int.
 */
bool all_int_char(char *user_input) {
    for (int i = 0; user_input[i] != '\n'; i++) {
        if ((user_input[i] < '0') || (user_input[i] > '9')) {
            return false;
        }
    }
    return true;
}

/*
 * Check if user input is a valid float.
 */
bool all_float_char(char *user_input) {
    for (int i = 0; user_input[i] != '\n'; i++) {
        if ((user_input[i] < '0') || (user_input[i] > '9')) {
            if (user_input[i] != '.'){
                return false;
            }
        }
    }
    return true;
}

/*
 * Find the index of the newline character.
 */
int locate_newline(char *user_input) {
    int i = 0;
    while (user_input[i] != '\n') {
        i++;
    }
    return i;
}

/*
 * Flush the stidn buffer.
 */
void empty_stdin_buffer() {
    int c = getchar();
    while(c != '\n' && c != EOF) {
        c = getchar();
    }
}

/*
 * This function enables the user to edit the database.
 */
void db_editor(pid_t state_pid, pid_t server_pid, pid_t interest_pid) {

    // Initialize semaphores that count ATM's
    int semid3 = sem_create((key_t) 7564);
    int semid4 = sem_create((key_t) 7565);
    sem_initialize(semid3);
    sem_initialize(semid4);
    sem_acquire(semid3);

    // Initialize semaphore that allows exiting all atm's
    int exit_atms_semid = sem_create((key_t) 9090);
    sem_initialize(exit_atms_semid);
    sem_acquire(exit_atms_semid);

    // Create and initialize semaphore for locking shared memory 
    int semid1 = sem_create((key_t) 2021);
    sem_initialize(semid1);

    // Create and initialize semaphore that ensure that the correct values are in shared memory for the 
    // DB_server to process. Before this semaphore, if the user inputted many operations very quickly
    // and the DB_server had other messages to process, the data in shared memory could be changed before the
    // DB_server can process the request using that data.
    int semid2 = sem_create((key_t) 2323);
    sleep(1);
    
    // Create primary message queue (used for all messages except state printing messages)
    int msgqid = msgq_create((key_t) 1224);

    // Get access to message queue for writing program state to a file
    int state_msgqid = msgq_create((key_t) 8888);

    // Get/create and attach shared memory for communicating with DB_server
    int shmid = shm_create((key_t) 5050, sizeof(account_t));
    account_t *shared_memory = (account_t *) shm_access(shmid);

    // Get/create and attach shared memory that holds a pointer to an array of binary semaphores to protect accounts
    sem_acquire(semid2);
    int shmid_account_sem = shm_create((key_t) 4567, sizeof(locks_t));
    locks_t *account_locks = (locks_t *) shm_access(shmid_account_sem);
    sem_release(semid2);
    
    bool continue_loop = true;
    account_t temp_account_info;
    char temp_float[20];
    char operation_input[3];
    char account_attribute[3];
    bool send_message;
    msg_t send_buffer;
    send_buffer.msg_type = 4;
    int message_size = sizeof(send_buffer.account_no) + sizeof(send_buffer.pin) + sizeof(send_buffer.amount);
    state_msg_t state_buffer;
    state_buffer.msg_type = 1;
    bool valid_inputs;
    int newline_locations[3];

    // Print state
    strcpy(state_buffer.message, "DB_editor -> Online");
    msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

    // Loop forever
    while (continue_loop) {
        // Ask the user which operation they would like to perform
        printf("\nWhat would you like to do?:\n[0] ADD ACCOUNT\n[1]");
        printf(" DELETE ACCOUNT\n[2] UNBLOCK ACCOUNT\n[3] MODIFY ACCOUNT\n[X] EXIT\n>>> ");
        fgets(operation_input, sizeof(operation_input), stdin);
        newline_locations[0] = locate_newline(operation_input);
        if (newline_locations[0] > 1) {
            empty_stdin_buffer();
            send_message = false;
            valid_inputs = false;
        } else if (newline_locations[0] < 1) {
            send_message = false;
            valid_inputs = false;
        } else {
            send_message = true;
            valid_inputs = true;
        }

        // Make sure that the operation input was the right length. If not, make the user try again.
        if (operation_input[1] != '\n') {
            operation_input[0] = '9';
            send_message = false;
        }

        if (operation_input[0] == '0') {
            // The user wants to add a new account

            printf("\nPlease input a 5-digit account number\n>>> ");
            fgets(temp_account_info.account_no, sizeof(temp_account_info.account_no), stdin);
            newline_locations[1] = locate_newline(temp_account_info.account_no);
            if (newline_locations[1] > 5) {
                empty_stdin_buffer();
            }

            printf("\nPlease input a 3-digit pin\n>>> ");
            fgets(temp_account_info.pin, sizeof(temp_account_info.pin), stdin);
            newline_locations[2] = locate_newline(temp_account_info.pin);
            if (newline_locations[2] > 3) {
                empty_stdin_buffer();
            }

            printf("\nPlease input the amount of funds available\n>>> ");
            fgets(temp_float, sizeof(temp_float), stdin);

            // Make sure that the user inputs are the right length
            if (newline_locations[0] != 5 || newline_locations[1] != 3 || newline_locations[2] != 5) {
                valid_inputs = false;
            }

            // Make sure that all characters in the inputs are numbers
            if (!all_int_char(temp_account_info.account_no)) {
                valid_inputs = false;
            } else if (!all_int_char(temp_account_info.pin)) {
                valid_inputs = false;
            } else if (!all_float_char(temp_float)) {
                valid_inputs = false;
            }

            if (valid_inputs) {
                // Get rid of newline character that fgets automatically adds
                temp_account_info.account_no[5] = '\0';
                temp_account_info.pin[3] = '\0';

                // fgets gets strings, convert the amount to a float
                temp_account_info.amount = atof(temp_float);
                if (temp_account_info.amount < 0.00) {
                    printf("\nYou cannot set the amount to be a negative number\n");
                    send_message = false;
                } else {
                    // Critical section starts, wait until it is okay to access memory and then set semaphore to wait
                    sem_acquire(semid2);
                    sem_acquire(semid1);

                    // Modify shared memory
                    shared_memory->operation = 0;
                    strcpy(shared_memory->account_no, temp_account_info.account_no);
                    strcpy(shared_memory->pin, temp_account_info.pin);
                    shared_memory->amount = temp_account_info.amount;

                    // Release semaphore
                    sem_release(semid1);

                    printf("\nThe following inputted values were successfully sent to the database\n%5s, %3s, %.2f\n\n", 
                        temp_account_info.account_no, temp_account_info.pin, temp_account_info.amount);
                }
            } else {
                printf("\nOne of your inputs was invalid, please try again\n");
                send_message = false;
            }


        } else if (operation_input[0] == '1') {
            // The user wants to delete an account
            printf("\nPlease input the 5-digit account number of the account you would like to delete\n>>> ");
            fgets(temp_account_info.account_no, sizeof(temp_account_info.account_no), stdin);
            newline_locations[0] = locate_newline(temp_account_info.account_no);
            if (newline_locations[0] > 5) {
                empty_stdin_buffer();
            }

            // Make sure that the user inputs are the right length
            if (newline_locations[0] != 5) {
                valid_inputs = false;
            }

            // Make sure that all characters in the inputs are numbers
            if (!all_int_char(temp_account_info.account_no)) {
                valid_inputs = false;
            }

            if (valid_inputs) {
                temp_account_info.account_no[5] = '\0';
                // Critical section starts, wait until it is okay to access memory and then set semaphore to wait
                sem_acquire(semid2);
                sem_acquire(semid1);

                // Modify shared memory
                shared_memory->operation = 1;
                strcpy(shared_memory->account_no, temp_account_info.account_no);

                // Release semaphore
                sem_release(semid1);
            } else {
                printf("\nOne of your inputs was invalid, please try again\n");
                send_message = false;
            }

        } else if (operation_input[0] == '2') {
            // The user wants to unblock an account
            printf("\nPlease input the 5-digit account number of the account you would like to unblock\n>>> ");
            fgets(temp_account_info.account_no, sizeof(temp_account_info.account_no), stdin);
            newline_locations[0] = locate_newline(temp_account_info.account_no);
            if (newline_locations[0] > 5) {
                empty_stdin_buffer();
            }

            // Make sure that the user inputs are the right length
            if (newline_locations[0] != 5) {
                valid_inputs = false;
            }

            // Make sure that all characters in the inputs are numbers
            if (!all_int_char(temp_account_info.account_no)) {
                valid_inputs = false;
            }

            if (valid_inputs) {
                temp_account_info.account_no[5] = '\0';

                // Critical section starts, wait until it is okay to access memory and then set semaphore to wait
                sem_acquire(semid2);
                sem_acquire(semid1);

                // Modify shared memory
                shared_memory->operation = 2;
                strcpy(shared_memory->account_no, temp_account_info.account_no);
                shared_memory->account_no[0] = 'X';

                // Release semaphore
                sem_release(semid1);

                printf("\nYour new account number is: '%5s'\n", temp_account_info.account_no);
                } else {
                    printf("\nOne of your inputs was invalid, please try again\n");
                    send_message = false;
                }
        
        } else if (operation_input[0] == '3') {
            // The user wants to modify an already existing account (done by deleting an old account and creating a new one)

            printf("\nPlease input the 5-digit account number of the account you would like to modify\n>>> ");
            fgets(send_buffer.account_no, sizeof(send_buffer.account_no), stdin);
            newline_locations[0] = locate_newline(send_buffer.account_no);
            if (newline_locations[0] > 5) {
                empty_stdin_buffer();
            }

            printf("\nPlease input the new 5-digit account number\n>>> ");
            fgets(temp_account_info.account_no, sizeof(temp_account_info.account_no), stdin);
            newline_locations[1] = locate_newline(temp_account_info.account_no);
            if (newline_locations[1] > 5) {
                empty_stdin_buffer();
            }

            printf("\nPlease input the new 3-digit pin\n>>> ");
            fgets(temp_account_info.pin, sizeof(temp_account_info.pin), stdin);
            newline_locations[2] = locate_newline(temp_account_info.pin);
            if (newline_locations[2] > 3) {
                empty_stdin_buffer();
            }

            printf("\nPlease input the new amount of funds available\n>>> ");
            fgets(temp_float, sizeof(temp_float), stdin);

            // Make sure that the user inputs are the right length
            if (newline_locations[0] != 5 || newline_locations[1] != 5 || newline_locations[2] != 3) {
                valid_inputs = false;
            }

            // Make sure that all characters in the inputs are numbers
            if (!all_int_char(temp_account_info.account_no)) {
                valid_inputs = false;
            } else if (!all_int_char(send_buffer.account_no)) {
                valid_inputs = false;
            } else if (!all_int_char(temp_account_info.pin)) {
                valid_inputs = false;
            } else if (!all_float_char(temp_float)) {
                valid_inputs = false;
            }

            if (valid_inputs) {
                // Get rid of newline character that fgets automatically adds
                send_buffer.account_no[5] = '\0';
                temp_account_info.account_no[5] = '\0';
                temp_account_info.pin[3] = '\0';

                // fgets gets strings, convert the amount to a float
                temp_account_info.amount = atof(temp_float);

                if (temp_float < 0) {
                    printf("\nYou cannot set the balance to be negative, please try again\n");
                } else {
                    // Critical section starts, wait until it is okay to access memory and then set semaphore to wait
                    sem_acquire(semid2);
                    sem_acquire(semid1);

                    shared_memory->operation = 1;
                    strcpy(shared_memory->account_no, send_buffer.account_no);

                    sem_release(semid1);

                    msgq_send(msgqid, (void *)&send_buffer, message_size);

                    // Try and acquire semaphore 2, requires waiting until DB_server has processed the first request
                    sem_acquire(semid2);
                    sem_acquire(semid1);

                    shared_memory->operation = 0;
                    strcpy(shared_memory->account_no, temp_account_info.account_no);
                    strcpy(shared_memory->pin, temp_account_info.pin);
                    shared_memory->amount = temp_account_info.amount;

                    // Release semaphore
                    sem_release(semid1);

                    msgq_send(msgqid, (void *)&send_buffer, message_size);


                    printf("\nThe following modified account information was successfully sent to the database\n%5s, %3s, %.2f\n\n", 
                        temp_account_info.account_no, temp_account_info.pin, temp_account_info.amount);
                
                    send_message = false;     // Because we sent messages differently than usual for this operation
                }
            } else {
                printf("\nOne of your inputs was invalid, please try again\n");
                send_message = false;
            }

        } else if (operation_input[0] == 'X') {
            // The user wants to exit
            send_message = false;
            continue_loop = false;

        } else {
            printf("\nThat was an invalid selection, please try again.\n");
            send_message = false;
        }

        // Send UPDATE_DB message to get the DB_server to look at the shared memory and process the modification
        if (send_message) {
            msgq_send(msgqid, (void *)&send_buffer, message_size);
        }
    }

    // Exit atms
    sem_release(exit_atms_semid);

    // Kill DB_server, interest and write_state

    // This is necessary because sometimes the system calls these processes made would return
    // error message after they were deleted. Also, for some reason sometimes the processes would 
    // also not be terminated at all when DB_server was done, so this is just as a precaution.
    kill(server_pid, SIGTERM);
    kill(state_pid, SIGTERM);
    kill(interest_pid, SIGTERM);

    // It will only let you re-acquire this resource when all atm's are exited
    sem_acquire(exit_atms_semid);

    // Delete individual account semaphores
    for (int i = 0; i < 10; i++) {
        sem_delete(sem_create((key_t) 5000 + (2*i)));
    }

    // Delete semaphores
    sem_delete(semid1);
    sem_delete(semid2);
    sem_delete(semid3);
    sem_delete(semid4);
    sem_delete(exit_atms_semid);

    // Detach and delete shared memory
    shm_detach(shared_memory);
    shm_delete(shmid);
    shm_detach(account_locks);
    shm_delete(shmid_account_sem);

    // Delete message queues
    msgq_delete(state_msgqid);
    msgq_delete(msgqid);
}


int main() {
    // Fork to create interest process
    pid_t state_pid = fork();

    if (state_pid == 0) {
        execlp("./write_state", "write_state", NULL);
    } else if (state_pid > 0) {

        // Fork to create DB_server process
        pid_t server_pid = fork();

        if (server_pid == 0) {
            execlp("./DB_server", "DB_server", NULL);
        } else if (server_pid > 0) {

            // Fork to create interest process
            pid_t interest_pid = fork();

            if (interest_pid == 0) {
                execlp("./interest", "interest", NULL);
            } else if (interest_pid > 0) {
                db_editor(state_pid, server_pid, interest_pid);
            } else if (interest_pid < 0) {
                fprintf(stderr, "Database server fork failed\n");
                exit(EXIT_FAILURE);
            }
        } else if (server_pid < 0) {
            fprintf(stderr, "Database editor fork failed\n");
            exit(EXIT_FAILURE);
        }
    } else if (state_pid < 0) {
        fprintf(stderr, "Database editor fork failed\n");
        exit(EXIT_FAILURE);
    }
}