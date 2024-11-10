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

// Producer function
void produce(struct Buffer *buffer, int semid, int item) {
    sem_wait(semid, 0);  // Wait for empty slot
    sem_wait(semid, 2);  // Mutex lock

    // Produce an item and add it to the buffer
    buffer->buffer[buffer->in] = item;
    buffer->in = (buffer->in + 1) % BUFFER_SIZE;  // Circular buffer

    printf("Produced: %d\n", item);

    sem_signal(semid, 2);  // Mutex unlock
    sem_signal(semid, 1);  // Signal that buffer has an item
}

int main() {
    int shm_id, sem_id;
    struct Buffer *buffer;

    // Create shared memory
    shm_id = shmget(IPC_PRIVATE, sizeof(struct Buffer), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    buffer = (struct Buffer *)shmat(shm_id, NULL, 0);  // Attach shared memory
    buffer->in = buffer->out = 0;  // Initialize buffer indices

    // Create semaphores
    sem_id = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);
    if (sem_id == -1) {
        perror("semget failed");
        exit(1);
    }

    // Initialize semaphores
    semctl(sem_id, 0, SETVAL, BUFFER_SIZE);  // Semaphore 0 (empty slots)
    semctl(sem_id, 1, SETVAL, 0);            // Semaphore 1 (full slots)
    semctl(sem_id, 2, SETVAL, 1);            // Semaphore 2 (mutex)

    // Produce items
    for (int i = 0; i < 5; i++) {
        produce(buffer, sem_id, i);  // Produce an item with number i
        sleep(1);  // Simulate some delay
    }
    printf("Producer done.\n");  // Print .done after producing all items

    // Clean up semaphores and shared memory
    semctl(sem_id, 0, IPC_RMID);  // Remove semaphore set
    shmdt(buffer);  // Detach shared memory
    shmctl(shm_id, IPC_RMID, NULL);  // Destroy shared memory

    return 0;
}
