#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define BUFFER_SIZE 5  


struct Buffer {
    int buffer[BUFFER_SIZE]; 
    int in;                   
    int out;                 
};


sem_t empty;   
sem_t full;    
sem_t mutex;   


void consume(struct Buffer *buffer) {
    sem_wait(&full);   
    sem_wait(&mutex);   


    int item = buffer->buffer[buffer->out];
    buffer->out = (buffer->out + 1) % BUFFER_SIZE; 

    printf("Consumed: %d\n", item);

    sem_post(&mutex);   
    sem_post(&empty);   
}

int main() {
    int shm_id;
    struct Buffer *buffer;

    
    shm_id = shmget(IPC_PRIVATE, sizeof(struct Buffer), 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    buffer = (struct Buffer *)shmat(shm_id, NULL, 0);  

s
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

   
    for (int i = 0; i < 5; i++) {
        consume(buffer);  
        sleep(1);  
    }

    printf("Consumer done.\n");  


    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&mutex);

    shmdt(buffer);  

    return 0;
}
