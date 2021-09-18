gcc ATM.c message_queue.c semaphore.c shared_memory.c -o ATM
gcc DB_editor.c message_queue.c semaphore.c shared_memory.c -o DB_editor
gcc DB_server.c message_queue.c semaphore.c shared_memory.c -o DB_server
gcc interest.c message_queue.c -o interest
gcc write_state.c message_queue.c -o write_state