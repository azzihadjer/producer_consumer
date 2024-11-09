#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>

#define BUFFER_SIZE 5  


struct Buffer {
    int buffer[BUFFER_SIZE];  
    int in;                   
    int out;                 
};


sem_t empty;  
sem_t full;    
sem_t mutex;   


void produce(struct Buffer *buffer, int item) {
    sem_wait(&empty);   
    sem_wait(&mutex);   

  
    buffer->buffer[buffer->in] = item;
    buffer->in = (buffer->in + 1) % BUFFER_SIZE;  

    printf("Produced: %d\n", item);

    sem_post(&mutex);   
    sem_post(&full);    
}

int main() {
    int shm_id;
    struct Buffer *buffer;


    shm_id = shmget(IPC_PRIVATE, sizeof(struct Buffer), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

    buffer = (struct Buffer *)shmat(shm_id, NULL, 0);  
    buffer->in = buffer->out = 0; 

  
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
        produce(buffer, i);  
        sleep(1);  
    }

    printf("Producer done.\n");  


    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&mutex);

    shmdt(buffer); 

    return 0;
}
