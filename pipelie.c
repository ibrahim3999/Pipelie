#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

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
    pthread_mutex_t activeMutex; // Added mutex to protect 'active' flag
} ActiveObject;

ActiveObject* getActiveObject(void* obj);

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

typedef struct {
    ActiveObject* producerAO;
    int active;
    int seed;
} ProducerArguments;

void* producer(void* arg) {
    ProducerArguments* args = (ProducerArguments*)arg;
    ActiveObject* activeObj = args->producerAO;
    ThreadSafeQueue* queue = activeObj->queue;
    int N = args->active;
    int seed = args->seed;

    srand(seed);

    for (int i = 0; i < N; i++) {
        int* item = malloc(sizeof(int));
        *item = (rand() % 900000) + 100000;  // Generate a random 6-digit positive number
        enqueue(queue, item);
        usleep(1000);  // Sleep for 1 millisecond
    }

    return NULL;
}


void processTask(void* task) {
    int* number = (int*)task;
    printf("AO1: Generated number %d\n", *number);
    ActiveObject* nextAO = getActiveObject(task);
    enqueue(nextAO->queue, task);
}

void processTask2(void* task) {
    int* number = (int*)task;
    printf("AO2: Received number %d, isPrime: %s\n", *number, isPrime(*number) ? "true" : "false");
    *number += 11;
    printf("AO2: Updated number: %d\n", *number);
    ActiveObject* nextAO = getActiveObject(task);
    enqueue(nextAO->queue, task);
}

void processTask3(void* task) {
    int* number = (int*)task;
    printf("AO3: Received number %d, isPrime: %s\n", *number, isPrime(*number) ? "true" : "false");
    *number -= 13;
    printf("AO3: Updated number: %d\n", *number);
    ActiveObject* nextAO = getActiveObject(task);
    enqueue(nextAO->queue, task);
}

void processTask4(void* task) {
    int* number = (int*)task;
    printf("AO4: Received number %d, new number: %d\n", *number, *number + 2);
    *number += 2;
    ActiveObject* nextAO = getActiveObject(task);
    enqueue(nextAO->queue, task);
}

ActiveObject* createActiveObject(ThreadSafeQueue* queue, void (*func)(void*)) {
    ActiveObject* obj = malloc(sizeof(ActiveObject));
    obj->queue = queue;
    obj->func = func;
    obj->active = 0;
    pthread_mutex_init(&obj->activeMutex, NULL); // Initialize active mutex

    pthread_create(&obj->thread, NULL, obj->func, obj);

    return obj;
}

ActiveObject* getActiveObject(void* obj) {
    return ((ActiveObject*)obj);
}

void stopActiveObject(ActiveObject* activeObj) {
    pthread_mutex_lock(&activeObj->activeMutex); // Lock the active mutex
    if (activeObj->active) {
        activeObj->active = 0;
        pthread_mutex_unlock(&activeObj->activeMutex); // Unlock the active mutex
        pthread_join(activeObj->thread, NULL);
    } else {
        pthread_mutex_unlock(&activeObj->activeMutex); // Unlock the active mutex
    }
}




int main(int argc, char** argv) {
    int N = 2;
    if (argc > 1) {
        N = atoi(argv[1]);
    }

    printf("Starting the pipeline with N = %d\n", N);

    // ThreadSafeQueue queue;
    // initQueue(&queue);

    // ActiveObject* ao1 = createActiveObject(&queue, processTask);
    // ActiveObject* ao2 = createActiveObject(&queue, processTask2);
    // ActiveObject* ao3 = createActiveObject(&queue, processTask3);
    // ActiveObject* ao4 = createActiveObject(&queue, processTask4);

    // pthread_mutex_lock(&ao1->activeMutex);
    // if (!ao1->active) {
    //     ao1->active = N;
    // }
    // pthread_mutex_unlock(&ao1->activeMutex);



    // ProducerArguments producerArgs = { ao1, N, time(NULL) };
    // pthread_t producerThread;
    // pthread_create(&producerThread, NULL, producer, &producerArgs);

    // stopActiveObject(ao1);
    // stopActiveObject(ao2);
    // stopActiveObject(ao3);
    // stopActiveObject(ao4);

    // pthread_join(producerThread, NULL);

    return 0;
}
