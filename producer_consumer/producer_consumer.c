#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

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

    // Fork process to create producer and consumer
    pid_t pid = fork();

    if (pid == 0) {
        // Child process (Consumer)
        for (int i = 0; i < 5; i++) {
            consume(buffer, sem_id);  // Consume an item
            sleep(1);  // Simulate some delay
        }
        printf("Consumer done.\n");  // Print .done after consuming all items
        shmdt(buffer);  // Detach shared memory
        exit(0);
    } else if (pid > 0) {
        // Parent process (Producer)
        for (int i = 0; i < 5; i++) {
            produce(buffer, sem_id, i);  // Produce an item with number i
            sleep(1);  // Simulate some delay
        }
        printf("Producer done.\n");  // Print .done after producing all items
        wait(NULL);  // Wait for child process to finish

        // Clean up semaphores and shared memory
        semctl(sem_id, 0, IPC_RMID);  // Remove semaphore set
        shmdt(buffer);  // Detach shared memory
        shmctl(shm_id, IPC_RMID, NULL);  // Destroy shared memory
    } else {
        perror("fork failed");
        exit(1);
    }

    return 0;
}
