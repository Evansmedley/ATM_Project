ATM Concurrencies
[Version 2.0] 
April 27, 2021

#-----------------------------------------------------------------------------
===================
CONTACT INFORMATION
===================

Contact Name:		Trong Nguyen, 100848232
			Evan Smedley, 100148695
Affiliation: 		Carleton University - Systems and Computer Engineering
			SYSC 4001 Operating Systems

#-----------------------------------------------------------------------------
===========
DESCRIPTION
===========

- ATM Concurrencies is a simple prototype of a program that mimics the work of
an ATM system using concurrent processes and inter-process communication (IPC)
services.

- The application is composed of the following programs:

	ATM.c			(ATM interface for the customer)
	DB_server.c		(Database server connecting ATM and database)
	DB_editor.c		(Database editor for adminstrator privilege)
	write_state.c		(Receives message on states of processes)
	interest.c		(Sends interest message to the DB_server)

- The application is supported by three libraries that provide an ease-of-use
interface for IPCs:

	message_queue.c		(Implement functions in message_queue.h)
	message_queue.h		
	semaphore.c		(Implement functions in semaphore.h)
	semaphore.h
	shared_memory.c		(Implement functions in shared_memory.h)
	shared_memory.h
	semun.h			(Used in semaphore operations/initializing)

- The application has an additional header file that stores constants and 
structs that are used in multiple programs:

	structs.h

- The application makes use of three text files to record the state and store
account information:
	
	DB.txt			(Stores account information)
	newDB.txt		(Transient txt file for editing the database)
	system_state.txt 	(Overwritten state information by write_state)

- The application includes a shell script 'compile.sh' that will compile each
program.

Note: 	The executable files included have been compiled using Ubuntu-20.4
	and Fedora-33. For instances where the executables fail, use 
	'compile.sh' to recomplie as per system dependencies. Then give
	permission in the terminal with:
	
	$ chmod +x compile.sh

#-----------------------------------------------------------------------------
============
INSTALLATION
============

This program has been tested for Linux ubuntu-20.4 and Fedora-33 workstation.

-------------------------- Installation Dependencies -------------------------

The application should work with the most current version of C programming 
language update C17 ISO/IEC 9899:2018. 

No other supplementary libraries and testing file with modular dependencies
are required to run this application.

#-----------------------------------------------------------------------------
=====
USAGE
=====

------------------------------- Program Compilation --------------------------

The executable files provided have been compiled and tested on the lab 
computers via the ssh connection to the 'sysc network' as well as on local
Linux system.

For ease of use and because each process uses libraries so the compilation 
instruction is more complicated, we have created the shell script 'compile.sh'.
This script should compiles all required programs for the application. To
give it permission to execute with the following command:

	$ chmod +x compile.sh
	$ ./compile.sh


--------------------------------- Program Running ----------------------------

To run the program via terminal, run DB_editor with the following command:

	$ ./DB_editor

Once this is done, run as many ATM's as desired by repeating the following:

	1. Open a new terminal
	2. Run ATM with the command ./ATM

The program will now be fully functional.

Note: 	The DB_editor will work by itself even without any active ATM 
	processes.

------------------------------ System Architecture ---------------------------

The system is organized in several concurrent processes that interact with a 
database (DB.txt):

	ATM <-> DB_server <-> DB_editor <-> interest <-> write_state

DB (Database) ----------------------------------------------------------------

The database is a text file that contains information about the customer's
banking information in the following format (with a comma between each):

	Account Number		Encoded PIN		Funds availiable
	5 char			3 char			Float

This database has been initialized with the following information:

	00001,107,3443.22
	00011,323,10089.97
	00117,259,112.00

ATM functionality and components ---------------------------------------------

1. Requests a 5-char account number from the customer.
2. Requests an 3-char PIN from the customer.
3. The account number and PIN are transmitted to the DB server ("PIN" 
   message). If the DB server returns "PIN_WRONG", restart from 1.
4. After three wrong PIN entries, "account is blocked" is displayed.
5. If the BD server returns "OK", the customer may choose the following 
   banking operations:
	a. Show balance: sends a "BALANCE" message, the checks the balance
	   in the DB, and displays the balance.
	b. Withdraw: sends a "WITHDRAW" message, and the amount to withdraw.
	   If there are insufficient funds available, the DB server returns 
	   "NSF" (not sufficient funds). In this case, there are insufficient 
           funds available.
	   Otherwise, the DB server returns "FUNDS_OK" message, and the new
	   balance is displayed.
	c. Deposit: sends a "DEPOSIT" message and the amount to deposit.

DB_server functionality and components --------------------------------------

1. Receives messages indefinitely.
2. When a PIN message is received from both matching account number and PIN.
   It searches the DB. Adds 1 to the PIN for "encryption" in the DB. If the 
   account number is found and the PIN is correct, return an "OK" message, 
   and saves the information on the account to a local variable. If failure 
   on account number or PIN number, then return "PIN_WRONG" message. After 
   three wrong attempts, the account is blocked. The first digit in the 
   account number is converted to an "X" character.
3. If the message is "BALANCE", search the current account number and
   get the fund fields from the DB and return it.
4. If the message is "WITHDRAW", get the amount and checks funds available.
   If funds are sufficient in the account, return "FUNDS_OK", decrement funds
   available, and update the DB.
   If funds, are not sufficient in the account. Return "NSF".
5. If an "UPDATE_DB" message is received, the updated information is obtained
   and saved to the DB (adding 1 to the PIN prior to saving it for 
   encryption).
6. Sends messages to the write_state process using a message queue to record
   the current state of the program.
7. If an "INTEREST" message is received, it applies interest to each of the
   accounts in the database.

interest functionality and components ---------------------------------------

1. Sends messages to the DB_server with a message queue every 60 seconds
   prompting it to add interest to all accounts in the database.
2. Loops indefinitely

write_state functionality and components ---------------------------------------

1. Loops indefinitely, waiting to receive messages from other processes
   containing information on the state of the system to be written to 
   'system_state.txt'

DB_editor functionality and components ---------------------------------------

1. Loops indefinitely
2. Requests that the user input their desired operation (add account, delete
   account, modify account, unblock account, exit)
3. Get the user input needed to complete each operation
4. Update shared memory with the information
5. Lock the shared memory until the DB_server can unlock it after the modification
has been made
5. Send an "UPDATE_DB" message to prompt the DB server to check shared memory
6. When exiting, deletes all IPC's used in all of the programs

Note:	DB_editor uses the fork and execlp to run DB_server, interest and 
  	write_state as its child processes.

---------------------------------- System Close ------------------------------

Enter 'X' when prompted to choose your operation in the DB_editor. The DB_editor 
will exit every single process (including any ATM's), and delete all IPCs that
were used in all of the processes. We treated the DB_editor as a sort of
administrator process, so it has control of everything. If you wish to exit a 
single ATM, this can be done by entering 'X' in the ATM when prompted for an
account number.

Do not exit the DB_editor by closing the terminal, this will leave the IPC's
undeleted which can cause issues when running the program again in the future.
If you accidentally do this, it can be fixed by rerunning the DB_editor and
choosing the exit option right away.

#-----------------------------------------------------------------------------
=======
CREDITS
=======

Thanks to the support of TAs and Instructor during the development of this
application.

#-----------------------------------------------------------------------------
=======
LICENSE
=======

[MIT](https://choosealicense.com/licenses/mit/)

Copyright (c) 2021 Trong Nguyen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.