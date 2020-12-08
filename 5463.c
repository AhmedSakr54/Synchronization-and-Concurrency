
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#define SHARED 1
#define ITER 2000

void *mmonitor(void *); 
void *mcollector(void *);
void *mcounter(void *);

sem_t empty, full;    /* the global semaphores */
sem_t mutex_cnt;
sem_t mutex_buf;
int cnt = 0;
int * buf;
int in = 0;
int out = 0;
int b;

int main(int argc, char *argv[]) {

    int N = atoi(argv[1]);
    long t1 = atoi(argv[2]);
    b = atoi(argv[3]);

    pthread_t * mcounter_ids = (pthread_t *) malloc(N * sizeof(pthread_t));
    pthread_t mmonitor_id, mcollector_id;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    buf = (int*) malloc(b * sizeof(int));
    sem_init(&empty, SHARED, b);  
    sem_init(&full, SHARED, 0);   
    sem_init(&mutex_cnt, SHARED, 1);
    sem_init(&mutex_buf, SHARED, 1);
    long i;
    for (i = 0; i < N; i++) {
        pthread_create(&mcounter_ids[i], &attr, mcounter, (void *)i+1);
    }
    pthread_create(&mmonitor_id, &attr, mmonitor, (void *) t1);
    pthread_create(&mcollector_id, &attr, mcollector, (void *) NULL);
    for (i = 0; i < N; i++) {
        pthread_join(mcounter_ids[i], NULL);
    }
    pthread_join(mmonitor_id, NULL);
    pthread_join(mcollector_id, NULL);
    printf("main done");
}

void* mcounter(void * thread_data) {
    long thread_number;
    thread_number = (long) thread_data;
    int i = 0;
    while (i < ITER) {
        int sleep_time = rand() % 15;
        sleep(sleep_time);
        printf("Counter thread %ld: received a message\n", thread_number);
        int sem_value;
        sem_getvalue(&mutex_cnt, &sem_value);
        if (sem_value <= 0)
            printf("Counter thread %ld: waiting to write\n", thread_number);
        sem_wait(&mutex_cnt);
        sleep(1);
        cnt++;
        printf("Counter thread %ld: now adding to counter, counter value = %d\n", thread_number, cnt);
        sem_post(&mutex_cnt);
        i++;
    }
    pthread_exit(NULL);
}

void* mmonitor(void * thread_data) {
    long t1;
    t1 = (long) thread_data;
    int i = 0;
    while(i < ITER) {
        sleep(t1);
        int sem_value;
        sem_getvalue(&empty, &sem_value);
        if (sem_value <= 0)
            printf("Monitor thread: Buffer full!!\n");
        sem_wait(&empty);
        sem_getvalue(&mutex_cnt, &sem_value);
        if (sem_value <= 0)
            printf("Monitor thread: waiting to read counter\n");
        sem_wait(&mutex_cnt);
        printf("Monitor thread: reading a count value of %d\n", cnt);
        printf("Monitor thread: writing to buffer at position %d\n", in);
        sem_wait(&mutex_buf);
        buf[in] = cnt;
        in = (in + 1) % b;
        sem_post(&mutex_buf);
        cnt = 0;
        sem_post(&mutex_cnt);
        sem_post(&full);
        i++;
    }
    pthread_exit(NULL);
}

void * mcollector(void * thread_data) {
    int i = 0;
    int item;
    while (i < ITER) {
        int sleep_time = rand() % 5;
        sleep(sleep_time);
        int sem_value;
        sem_getvalue(&full, &sem_value);
        if (sem_value <= 0) 
            printf("Collector thread: nothing is in the buffer!\n");
        sem_wait(&full);
        sem_wait(&mutex_buf);
        printf("Collector thread: reading from buffer at position %d\n", out);
        item = buf[out];
        out = (out + 1) % b;
        sem_post(&mutex_buf);
        sem_post(&empty);
        i++;
    }
    pthread_exit(NULL);
}

