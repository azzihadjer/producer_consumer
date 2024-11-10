#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define BUFFER_SIZE 5  // Number of items that can be stored in the buffer

// Shared buffer structure
struct Buffer {
    int buffer[BUFFER_SIZE];  // The actual buffer
    int in;                   // Index for producer
    int out;                  // Index for consumer
};

// Semaphore operation wrappers for convenience
void sem_wait(int semid, int sem_num) {
    struct sembuf sb = {sem_num, -1, 0};
    semop(semid, &sb, 1);
}

void sem_signal(int semid, int sem_num) {
    struct sembuf sb = {sem_num, 1, 0};
    semop(semid, &sb, 1);
}

// Consumer function
void consume(struct Buffer *buffer, int semid) {
    sem_wait(semid, 1);  // Wait for full slot
    sem_wait(semid, 2);  // Mutex lock

    // Consume an item from the buffer
    int item = buffer->buffer[buffer->out];
    buffer->out = (buffer->out + 1) % BUFFER_SIZE;  // Circular buffer

    printf("Consumed: %d\n", item);

    sem_signal(semid, 2);  // Mutex unlock
    sem_signal(semid, 0);  // Signal that buffer has an empty slot
}

int main() {
    int shm_id, sem_id;
    struct Buffer *buffer;

    // Get shared memory
    shm_id = shmget(IPC_PRIVATE, sizeof(struct Buffer), 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    buffer = (struct Buffer *)shmat(shm_id, NULL, 0);  // Attach shared memory

    // Get semaphores
    sem_id = semget(IPC_PRIVATE, 3, 0666);
    if (sem_id == -1) {
        perror("semget failed");
        exit(1);
    }

    // Consume items
    for (int i = 0; i < 5; i++) {
        consume(buffer, sem_id);  // Consume an item
        sleep(1);  // Simulate some delay
    }
    printf("Consumer done.\n");  // Print .done after consuming all items

    // Clean up shared memory
    shmdt(buffer);  // Detach shared memory

    return 0;
}
