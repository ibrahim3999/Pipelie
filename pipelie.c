#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>

#define MAX_QUEUE_SIZE 10

int isPrime(unsigned int number) {
    if (number < 2) {
        return 0; 
    }
    
    double sqrtNumber = sqrt((double)number);
    for (unsigned int i = 2; i <= sqrtNumber; i++) {
        if (number % i == 0) {
            return 0; 
        }
    }
    
    return 1;  
}

typedef struct {
    void* data[MAX_QUEUE_SIZE];
    int size;
    int front;
    int rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ThreadSafeQueue;

typedef struct {
    ThreadSafeQueue* queue;
    void (*func)(void*);
    pthread_t thread;
    int active;
} ActiveObject;

ActiveObject* getQueue(void* obj);

void initQueue(ThreadSafeQueue* queue) {
    queue->size = 0;
    queue->front = 0;
    queue->rear = 0;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond, NULL);
}

void enqueue(ThreadSafeQueue* queue, void* item) {
    pthread_mutex_lock(&queue->mutex);

    while (queue->size >= MAX_QUEUE_SIZE) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }

    queue->data[queue->rear] = item;
    queue->rear = (queue->rear + 1) % MAX_QUEUE_SIZE;
    queue->size++;

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

void* dequeue(ThreadSafeQueue* queue) {
    pthread_mutex_lock(&queue->mutex);

    while (queue->size == 0) {
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }

    void* item = queue->data[queue->front];
    queue->front = (queue->front + 1) % MAX_QUEUE_SIZE;
    queue->size--;

    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);

    return item;
}

void* producer(void* arg) {
    ActiveObject* activeObj = (ActiveObject*)arg;
    ThreadSafeQueue* queue = activeObj->queue;
    int N = activeObj->active;

    srand(time(NULL));
    for (int i = 0; i < N; i++) {
        int* item = malloc(sizeof(int));
        *item = rand() % 900000 + 100000;  // Generate a random 6-digit number
        enqueue(queue, item);
        usleep(1000);  // Sleep for 1 millisecond
    }

    return NULL;
}

void processTask(void* task) {
    int* number = (int*)task;
    printf("Received number: %d\n", *number);
//    free(number);
}

ActiveObject* createActiveObject(ThreadSafeQueue* queue, void (*func)(void*)) {
    ActiveObject* activeObj = malloc(sizeof(ActiveObject));
    activeObj->queue = queue;
    activeObj->func = func;
    activeObj->active = 1;
    pthread_create(&activeObj->thread, NULL, (void* (*)(void*))func, activeObj);
    return activeObj;
}

ActiveObject* getQueue(void* obj) {
    return (ActiveObject*)obj;
}

void stopActiveObject(ActiveObject* activeObj) {
    activeObj->active = 0;
    pthread_join(activeObj->thread, NULL);
    free(activeObj);
}

void processTask1(void* task) {
    int* number = (int*)task;
    printf("AO1: Received number %d\n", *number);

    ActiveObject* nextAO = (ActiveObject*)dequeue(getQueue(task));
    enqueue(getQueue(nextAO), number + 11);

    free(number);
}

ThreadSafeQueue* queue2;
void processTask2(void* task) {
    ActiveObject* activeObj = (ActiveObject*)task;
    ThreadSafeQueue* queue = activeObj->queue;
    ActiveObject* nextAO = (ActiveObject*)dequeue(queue);
    int* number = (int*)dequeue(queue);

    printf("AO2: Received number %d, isPrime: %s\n", *number, isPrime(*number) ? "true" : "false");
    enqueue(queue, nextAO);
    enqueue(queue2, number + 13);

    free(number);
}


void processTask3(void* task) {
    ActiveObject* activeObj = (ActiveObject*)task;
    ThreadSafeQueue* queue = activeObj->queue;
    ActiveObject* nextAO = (ActiveObject*)dequeue(queue);
    int* number = (int*)dequeue(queue);

    printf("AO3: Received number %d, isPrime: %s\n", *number, isPrime(*number) ? "true" : "false");
    enqueue(getQueue(number - 2), nextAO);

    free(number);
}

void processTask4(void* task) {
    int* number = (int*)task;
    printf("AO4: Received number %d, new number: %d\n", *number, *number + 2);

   free(number);
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s N [seed]\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int seed = (argc > 2) ? atoi(argv[2]) : time(NULL);

    if (N <= 0) {
        printf("N must be a positive integer.\n");
        return 1;
    }

    ThreadSafeQueue* queue1;
    ThreadSafeQueue* queue2;
    ThreadSafeQueue* queue3;
    ThreadSafeQueue* queue4;

    queue1 = malloc(sizeof(ThreadSafeQueue));
    queue2 = malloc(sizeof(ThreadSafeQueue));
    queue3 = malloc(sizeof(ThreadSafeQueue));
    queue4 = malloc(sizeof(ThreadSafeQueue));

    initQueue(queue1);
    initQueue(queue2);
    initQueue(queue3);
    initQueue(queue4);

    ActiveObject* AO1 = createActiveObject(queue1, processTask1);
    ActiveObject* AO2 = createActiveObject(queue2, processTask2);
   ActiveObject* AO3 = createActiveObject(queue3, processTask3);
   ActiveObject* AO4 = createActiveObject(queue4, processTask4);

    printf("Starting the pipeline with N = %d\n", N);

    pthread_t *producerThread;
    ActiveObject producerAO;
    producerAO.queue = queue1;
    producerAO.active = N;
    pthread_create(&producerThread, NULL, producer, (void*)&producerAO);

    pthread_join(producerThread, NULL);


    stopActiveObject(AO1);
  stopActiveObject(AO2);
  stopActiveObject(AO3);
  stopActiveObject(AO4);

    printf("Pipeline completed.\n");

    return 0;
}
