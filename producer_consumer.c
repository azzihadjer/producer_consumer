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
    int in, out;
};

// Semaphores
sem_t empty, full, mutex;

// Shared memory IDs
int shmid;

// Producer function
void producer(struct Buffer *buffer) {
    int item;
    for (int i = 0; i < NUM_ITEMS; i++) {
        item = rand() % 100;  // Producing an item

        sem_wait(&empty);   // Wait for empty space
        sem_wait(&mutex);   // Wait for exclusive access

        // Add the item to the buffer
        buffer->data[buffer->in] = item;
        printf("Producer produced: %d\n", item);
        buffer->in = (buffer->in + 1) % BUFFER_SIZE;

        sem_post(&mutex);   // Release the mutex
        sem_post(&full);    // Signal that there is one more full slot

        sleep(1);  // Simulate time delay
    }
}

// Consumer function
void consumer(struct Buffer *buffer) {
    int item;
    for (int i = 0; i < NUM_ITEMS; i++) {
        sem_wait(&full);   // Wait for a full buffer
        sem_wait(&mutex);  // Wait for exclusive access

        // Remove the item from the buffer
        item = buffer->data[buffer->out];
        printf("Consumer consumed: %d\n", item);
        buffer->out = (buffer->out + 1) % BUFFER_SIZE;

        sem_post(&mutex);   // Release the mutex
        sem_post(&empty);   // Signal that there is one more empty slot

        sleep(2);  // Simulate time delay
    }
}

int main() {
    // Shared memory setup for buffer
    shmid = shmget(IPC_PRIVATE, sizeof(struct Buffer), IPC_CREAT | 0666);
    struct Buffer *buffer = (struct Buffer *)shmat(shmid, NULL, 0);
    buffer->in = buffer->out = 0;  // Initialize buffer indices

    // Initialize semaphores
    sem_init(&empty, 1, BUFFER_SIZE);   // BUFFER_SIZE empty slots
    sem_init(&full, 1, 0);              // 0 full slots initially
    sem_init(&mutex, 1, 1);             // Mutex for mutual exclusion

    // Fork to create producer and consumer processes
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(1);
    }

    if (pid == 0) {
        // Child process: consumer
        printf("Consumer started\n");
        consumer(buffer);
        printf("Consumer finished\n");
    } else {
        // Parent process: producer
        printf("Producer started\n");
        producer(buffer);
        printf("Producer finished\n");

        // Wait for the child process to finish
        wait(NULL);

        // Clean up
        sem_destroy(&empty);
        sem_destroy(&full);
        sem_destroy(&mutex);

        // Detach and remove shared memory
        shmdt(buffer);
        shmctl(shmid, IPC_RMID, NULL);
    }

    return 0;
} 