#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define BUFFER_SIZE 5
#define NUM_ITEMS 10

// Buffer structure
struct Buffer {
    int data[BUFFER_SIZE];
    int count;  // Tracks number of items in the buffer
};

// Semaphores
sem_t empty, full, mutex;

// Shared memory IDs
int shmid;

// Producer function
void add_to_buffer(struct Buffer *buffer, int item) {
    buffer->data[buffer->count] = item;
    printf("Producer produced: %d\n", item);
    buffer->count++;  // Increment count as an item is added
}

void remove_from_buffer(struct Buffer *buffer, int *item) {
    *item = buffer->data[buffer->count - 1];
    printf("Consumer consumed: %d\n", *item);
    buffer->count--;  // Decrement count as an item is removed
}

void producer(struct Buffer *buffer) {
    int item;
    for (int i = 0; i < NUM_ITEMS; i++) {
        item = rand() % 100;  // Generate a random item

        sem_wait(&empty);
        sem_wait(&mutex);

        add_to_buffer(buffer, item);

        sem_post(&mutex);
        sem_post(&full);

        sleep(1);  // Simulate work
    }
}

void consumer(struct Buffer *buffer) {
    int item;
    for (int i = 0; i < NUM_ITEMS; i++) {
        sem_wait(&full);
        sem_wait(&mutex);

        remove_from_buffer(buffer, &item);

        sem_post(&mutex);
        sem_post(&empty);

        sleep(2);  // Simulate work
    }
}

void createProducerAndConsumerProcesses(struct Buffer *buffer) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }

    if (pid == 0) {
        // Child process: consumer
        consumer(buffer);
        exit(0);  // Ensure the child exits after consuming
    } else {
        // Parent process: producer
        producer(buffer);

        // Wait for the child process to finish
        wait(NULL);

        // Cleanup
        sem_destroy(&empty);
        sem_destroy(&full);
        sem_destroy(&mutex);

        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);  // Remove shared memory
    }
}

int main() {
    // Shared memory setup for buffer
    shmid = shmget(IPC_PRIVATE, sizeof(struct Buffer), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget failed");
        exit(1);
    }

    struct Buffer *buffer = (struct Buffer *)shmat(shmid, NULL, 0);
    if (buffer == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }
    buffer->count = 0;  // Initialize count to 0

    // Initialize semaphores
    sem_init(&empty, 1, BUFFER_SIZE);  // BUFFER_SIZE empty slots
    sem_init(&full, 1, 0);              // 0 full slots initially
    sem_init(&mutex, 1, 1);            // Mutex for mutual exclusion

    // Fork to create producer and consumer processes
    createProducerAndConsumerProcesses(buffer);

    return 0;
}
