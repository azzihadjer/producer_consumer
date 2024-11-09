#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define BUFFER_SIZE 5  // Number of items that can be stored in the buffer

// Shared buffer structure
struct Buffer {
    int buffer[BUFFER_SIZE];  // The actual buffer
    int in;                   // Index for producer
    int out;                  // Index for consumer
};

// Semaphore variables
sem_t empty;   // Semaphore to count empty slots
sem_t full;    // Semaphore to count full slots
sem_t mutex;   // Mutex for mutual exclusion

// Producer function
void produce(struct Buffer *buffer, int item) {
    sem_wait(&empty);   // Wait if buffer is full
    sem_wait(&mutex);   // Lock buffer (mutual exclusion)

    // Produce an item and add it to the buffer
    buffer->buffer[buffer->in] = item;
    buffer->in = (buffer->in + 1) % BUFFER_SIZE;  // Circular buffer

    printf("Produced: %d\n", item);

    sem_post(&mutex);   // Unlock buffer
    sem_post(&full);    // Signal that the buffer has an item
}

// Consumer function
void consume(struct Buffer *buffer) {
    sem_wait(&full);    // Wait if buffer is empty
    sem_wait(&mutex);   // Lock buffer (mutual exclusion)

    // Consume an item from the buffer
    int item = buffer->buffer[buffer->out];
    buffer->out = (buffer->out + 1) % BUFFER_SIZE;  // Circular buffer

    printf("Consumed: %d\n", item);

    sem_post(&mutex);   // Unlock buffer
    sem_post(&empty);   // Signal that the buffer has space
}

int main() {
    int shm_id;
    struct Buffer *buffer;

    // Create shared memory
    shm_id = shmget(IPC_PRIVATE, sizeof(struct Buffer), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    buffer = (struct Buffer *)shmat(shm_id, NULL, 0);  // Attach shared memory
    buffer->in = buffer->out = 0;  // Initialize buffer indices

    // Initialize semaphores
    if (sem_init(&empty, 1, BUFFER_SIZE) == -1) {
        perror("sem_init empty failed");
        exit(1);
    }
    if (sem_init(&full, 1, 0) == -1) {
        perror("sem_init full failed");
        exit(1);
    }
    if (sem_init(&mutex, 1, 1) == -1) {
        perror("sem_init mutex failed");
        exit(1);
    }

    // Fork process to create producer and consumer
    pid_t pid = fork();

    if (pid == 0) {
        // Child process (Consumer)
        for (int i = 0; i < 5; i++) {
            consume(buffer);  // Consume an item
            sleep(1);  // Simulate some delay
        }
        printf("Consumer done.\n");  // Print .done after consuming all items
        shmdt(buffer);  // Detach shared memory
        exit(0);
    } else if (pid > 0) {
        // Parent process (Producer)
        for (int i = 0; i < 5; i++) {
            produce(buffer, i);  // Produce an item with number i
            sleep(1);  // Simulate some delay
        }
        printf("Producer done.\n");  // Print .done after producing all items
        wait(NULL);  // Wait for child process to finish

        // Clean up semaphores and shared memory
        sem_destroy(&empty);
        sem_destroy(&full);
        sem_destroy(&mutex);
        shmdt(buffer);  // Detach shared memory
        shmctl(shm_id, IPC_RMID, NULL);  // Destroy shared memory
    } else {
        perror("fork failed");
        exit(1);
    }

    return 0;
}
