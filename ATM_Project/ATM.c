/** 
 * Term Project - Concurrency, IPC, Semaphores, Shared Memory
 * 
 * ATM
 *
 * Carleton University
 * Department of System and Computer Engineering
 * SYSC 4001 - Operating Systems
 *
 * @authors Evan Smedley, 101148695 & Trong Nguyen, 100848232
 * @version v1.00
 * @release April 27, 2021
 *
 * The ATM component manages the interaction between the
 * customer and the database (DB) via an account number and a 
 * pin. The ATM allows the customers with various interactions
 * with the DB such as depositing and withdrawing a balance.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include "structs.h"
#include "message_queue.h"
#include "semaphore.h"
#include "shared_memory.h"

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

/**
 * Accesses a message queue with the key 1224. Walks the user through accessing account information.
 * Requires the user to input their account number and pin, if the user gets the pin wrong 3 times,
 * blocks the account. Takes user input to determine if the user would like to view their balance,
 * do a withdrawal or deposit.
 */
void atm() {

    // Create primary message queue
    int msgqid = msgq_create((key_t) 1224);

    // Get access to message queue for writing program state to a file
    int state_msgqid = msgq_create((key_t) 8888);

    // Get access to semaphore used for counting number of active ATM's
    int semid1 = sem_create((key_t) 7564);

    // Increment the semaphore because this is a new ATM
    sem_release(semid1);

    // Get access to semaphore that gives an ATM id to this ATM
    int semid2 = sem_create((key_t) 7565);
    int atm_id = sem_getval(semid2);
    sem_release(semid2);

    // Get access to shared memory that stores an array of semaphore keys for protecting accounts
    int shmid_account_sem = shm_create((key_t) 4567, sizeof(locks_t));
    locks_t *account_locks = (locks_t *) shm_access(shmid_account_sem);

    // Initialize variables
    msg_t send_buffer;
    msg_t receive_buffer;
    bool valid_acc = false;
    bool valid_pin = false;
    bool loop = true;
    bool get_account_no = true;
    int pin_attempts;
    char temp_float[20];
    char operation_input[3];
    int message_size = sizeof(send_buffer.account_no) + sizeof(send_buffer.pin) + sizeof(send_buffer.amount);
    state_msg_t state_buffer;
    state_buffer.msg_type = 1;
    int temp_semid;
    char temp_int[7];
    bool valid_input;
    int newline_location;
    bool cont;

    // Print state
    sprintf(state_buffer.message, "ATM %d -> Online\nNumber of atms online: %d", atm_id, sem_getval(semid1));
    msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

    // Loop until the user requests to exit
    while (loop) {
        // Take user input for the account number
        valid_input = true;
        valid_pin = false;
        printf("\n-------------------------------------------------\nATM %d\n-------------------------------------------------", atm_id);
        printf("\nPlease type your 5-digit account number >>> ");
        fgets(send_buffer.account_no, sizeof(send_buffer.account_no), stdin);
        if (send_buffer.account_no[0] == 'X') {
            printf("Thank you for using this ATM!\n");
            break;
        }
        newline_location = locate_newline(send_buffer.account_no);
        if (newline_location > 5) {
            empty_stdin_buffer();
        }

        // Make sure that the user input is the right length
        if (newline_location != 5) {
            valid_input = false;
            send_buffer.account_no[0] = '9';
        }

        // Make sure that all characters in the inputs are numbers
        if (!all_int_char(send_buffer.account_no)) {
            valid_input = false;
            send_buffer.account_no[0] = '9';
        }

        if (valid_input) {

            send_buffer.account_no[5] = '\0';

            // Set the send buffer message type to 1 to represent a PIN message
            send_buffer.msg_type = 1;
            pin_attempts = 0;
            // This must be set to a positive float because later on, if amount is negative, it is an indicator
            send_buffer.amount = 1.0;

            // Allow the user 3 tries to get the pin right
            while (pin_attempts < 3) {
                // Take user input for the pin
                printf("\nPlease type your 3-digit pin >>> ");
                fgets(send_buffer.pin, sizeof(send_buffer.pin), stdin);
                newline_location = locate_newline(send_buffer.pin);
                if (newline_location > 3) {
                    empty_stdin_buffer();
                }
                // Make sure that the user input is the right length
                if (newline_location != 3) {
                    printf("hi");
                    valid_input = false;
                }

                // Make sure that all characters in the inputs are numbers
                if (!all_int_char(send_buffer.pin)) {
                    valid_input = false;
                    printf("hi1");
                }

                send_buffer.pin[3] = '\0';
                if (send_buffer.pin[0] == 'X') {
                    valid_pin = false;
                    printf("hi2");
                    break;
                }

                // Send PIN message
                msgq_send(msgqid, (void *)&send_buffer, message_size);

                // Wait to receive PIN_WRONG or OK message
                msgq_receive(msgqid, (void *)&receive_buffer, message_size, 6);

                // Interpret message

                // Pin is correct
                if (receive_buffer.pin[0] == 'y') {
                    printf("\nPin correct!\n");
                    valid_pin = true;
                    for (int i = 1; i < NUM_LOCKS * 2; i += 2) {
                        strncpy(temp_int, send_buffer.account_no, 5);
                        temp_int[5] = '\0';
                        if (account_locks->locks[i] == atoi(temp_int)) {
                            temp_semid = sem_create((key_t) account_locks->locks[i - 1]);
                            if (sem_getval(temp_semid) == 0) {
                                printf("\nAnother user is using this account, you will have to wait\n");
                            }
                            sem_acquire(temp_semid);
                            break;
                        }
                    }
                    break;

                // Pin is incorrect
                } else if (receive_buffer.pin[0] == 'n') {
                    printf("\nPin incorrect.\n");
                    pin_attempts += 1;

                // There was a database server error
                } else {
                    printf("\nInvalid database server response");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // Valid_acc will be true if the user got the account number and pin correct
        if (valid_pin && valid_input) {

            // Print state
            sprintf(state_buffer.message, "ATM %d: signed in to account b", atm_id);
            msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

            // Loop forever
            while (true) {
                // Take user input for the desired operation
                printf("\nWould you like to:\n[0] VIEW BALANCE\n[1] WITHDRAW\n[2] DEPOSIT\n[X] EXIT\n>>> ");
                fgets(operation_input, sizeof(operation_input), stdin);
                newline_location = locate_newline(operation_input);
                if (newline_location > 1) {
                    empty_stdin_buffer();
                    operation_input[0] = '9';
                } else if (newline_location != 1) {
                    operation_input[0] = '9';
                }

                if (operation_input[0] == '0' || operation_input[0] == '1' || operation_input[0] == '2') {
                    cont = true;
                    if (operation_input[0] == '0') {
                        // Set up the send buffer with the BALANCE message
                        send_buffer.msg_type = 2;
                    } else if (operation_input[0] == '1') {
                        // Set up the send buffer with the WITHDRAW message
                        send_buffer.msg_type = 3;
                        printf("\nHow much would you like to withdraw? >>> ");
                        fgets(temp_float, sizeof(temp_float), stdin);
                        if (!all_float_char(temp_float)) {
                            cont = false;
                        } else {
                            send_buffer.amount = atof(temp_float);
                            if (send_buffer.amount < 0) {
                                cont = false;
                                printf("\nYou can't withdraw a negative amount\n");
                            }
                            
                        }
                    } else {
                        // Set up the send buffer with a DEPOSIT message
                        send_buffer.msg_type = 3;
                        printf("\nHow much would you like to deposit? >>> ");
                        fgets(temp_float, sizeof(temp_float), stdin);
                        if (!all_float_char(temp_float)) {
                            cont = false;
                        } else {
                            send_buffer.amount = atof(temp_float) * -1.0;
                            if (send_buffer.amount > 0) {
                                cont = false;
                                printf("\nYou can't deposit a negative amount\n");
                            }
                        }
                    }

                    if (cont) {
                        // Send the message (either BALANCE, WITHDRAW or DEPOSIT)
                        msgq_send(msgqid, (void *)&send_buffer, message_size);

                        // Wait for response
                        msgq_receive(msgqid, (void *)&receive_buffer, message_size, 7);

                        // Interpret response
                        if (operation_input[0] == '0') {
                            // Display balance
                            printf("\nYour balance is: %.2f\n", receive_buffer.amount);
                        } else {
                            // Display withdraw and deposit information
                            if (receive_buffer.account_no[0] == 'y') {
                                printf("\nSuccess!\nNew balance: %.2f\n", receive_buffer.amount);
                            } else if (receive_buffer.account_no[0] == 'n') {
                                if (send_buffer.amount < 0 && operation_input[0] == '1') {
                                    printf("\nWithdrawals must be positive\n");
                                } else if (send_buffer.amount > 0 && operation_input[0] == 2) {
                                    printf("\nDeposits must be positive\n");
                                } else {
                                    printf("\nNot enough funds available to process this withdrawal\n");
                                }
                            } else {
                                printf("\nInvalid database server response\n");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }

                // This if is true if the user wants to exit
                } else if (operation_input[0] == 'X') {
                    printf("\nThank you for using this ATM! Have a nice day!\n");
                    break;
                // If the user choice is invalid  they must try again
                } else {
                    printf("\nInvalid entry, please try again.\n");
                }
            }
            sem_release(temp_semid);

            // Print state
            sprintf(state_buffer.message, "ATM %d: signed out of account b", atm_id);
            msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

        } else if (pin_attempts == 3) {
            // This means that the PIN was incorrect 3 times so the account must be blocked
            // Set the send buffer amount to a negative float, this is how the server knows to block the account
            send_buffer.amount = -1.0;
            msgq_send(msgqid, (void *)&send_buffer, message_size);
            printf("\nAccount blocked.\nToo many incorrect attempts at inputting the pin\nPlease try another account.\n\n");

            // Print state
            sprintf(state_buffer.message, "ATM %d: account b is blocked", atm_id);
            msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));

        } else if (!valid_input) {
            printf("\nThat was not a valid input, please try again\n");
        } else {
            printf("\nThat was not a valid pin, please try again\n");
        }
    }
    // Decrement number of active ATM's because this one is closing
    sem_acquire(semid1);

    // Print state
    sprintf(state_buffer.message, "ATM %d -> Offline\nNumber of atms online: %d", atm_id, sem_getval(semid1));
    msgq_send(state_msgqid, (void *)&state_buffer, sizeof(state_buffer.message));
}

void parent(pid_t child_pid) {
    // Allows time for the DB_editor to initialize and acquire the semaphore
    sleep(1);

    // Get access to the semaphore, finish executing when the DB_editor releases the semaphore
    int exit_atms_semid = sem_create((key_t) 9090);
    sem_acquire(exit_atms_semid);
    sem_release(exit_atms_semid);
    kill(child_pid, SIGTERM);
    printf("\nATM Offline\n");
}

int main() {
    pid_t pid = fork();

    if (pid == 0) {
        atm(pid);
    } else if (pid > 0) {
        parent(pid);
    } else if (pid < 0) {
        fprintf(stderr, "Database server fork failed\n");
        exit(EXIT_FAILURE);
    }
}