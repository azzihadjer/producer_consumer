#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 5  // Size of the buffer
#define NUM_ITEMS 10   // Number of items the producer/consumer will produce/consume


typedef struct {
    int buffer[BUFFER_SIZE];
    int in, out;
} SharedBuffer;

int semid;  
int shm_id;  
SharedBuffer *shm_ptr; 


struct sembuf sem_op;


void init_semaphores() {
    semid = semget(IPC_PRIVATE, 3, IPC_CREAT | 0666);
    semctl(semid, 0, SETVAL, 1);  
    semctl(semid, 1, SETVAL, BUFFER_SIZE);  
    semctl(semid, 2, SETVAL, 0);  
}


void P(int sem_num) {
    sem_op.sem_num = sem_num;
    sem_op.sem_op = -1; 
    sem_op.sem_flg = 0;
    semop(semid, &sem_op, 1);
}

void V(int sem_num) {
    sem_op.sem_num = sem_num;
    sem_op.sem_op = 1;  
    sem_op.sem_flg = 0;
    semop(semid, &sem_op, 1);
}


void producer() {
    for (int item = 0; item < NUM_ITEMS; item++) {
        P(1);  

        P(0);  
        shm_ptr->buffer[shm_ptr->in] = item ;  
        printf("Producer produced item %d at position %d\n", item+1, shm_ptr->in);
        shm_ptr->in = (shm_ptr->in + 1) % BUFFER_SIZE;
        V(0); 

        V(2);  
        sleep(1);  
    }
}

void consumer( ) {
    for (int item = 0; item < NUM_ITEMS; item++) {
        P(2); 
        P(0);
        
        item = shm_ptr->buffer[shm_ptr->out];  
        printf("Consumer consumed item %d from position %d\n", item+1, shm_ptr->out);
        shm_ptr->out = (shm_ptr->out + 1) % BUFFER_SIZE;  
        V(0);  
        V(1);  
        sleep(1); 
   }
}
void create_producer_and_consumer_processes(){
    pid_t pid;

    pid = fork();
    if (pid == 0) {
            producer(); 
            exit(0);
    }
    
    pid = fork();
    if (pid == 0) {
           consumer(); 
           exit(0);
    }
    
}

int main() {

    shm_id = shmget(IPC_PRIVATE, sizeof(SharedBuffer), IPC_CREAT | 0666);
    
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }

   
    shm_ptr = (SharedBuffer *)shmat(shm_id, NULL, 0);
    
    if (shm_ptr == (SharedBuffer *)-1) {
        perror("shmat failed");
        exit(1);
    }

    shm_ptr->in = 0;
    shm_ptr->out = 0;

    
    init_semaphores();
    create_producer_and_consumer_processes();
    
    wait(NULL);
    wait(NULL);

    
    shmctl(shm_id, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);

    return 0;
}
