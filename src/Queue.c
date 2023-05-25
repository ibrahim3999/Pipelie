#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define MAX_QUEUE_SIZE 10

typedef struct {
    void* data[MAX_QUEUE_SIZE];
    int size;
    int front;
    int rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ThreadSafeQueue;
// Queue definition 
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
        // if size >= wait  when size-- 
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
    ThreadSafeQueue* queue = (ThreadSafeQueue*)arg;
    
    for (int i = 0; i < 20; i++) {
        int* item = malloc(sizeof(int));
        *item = i;
        enqueue(queue, item);
        printf("Produced item: %d\n", *item);
    }
    
    return NULL;
}

void* consumer(void* arg) {
    ThreadSafeQueue* queue = (ThreadSafeQueue*)arg;
    
    for (int i = 0; i < 20; i++) {
        int* item = (int*)dequeue(queue);
        printf("Consumed item: %d\n", *item);
        free(item);
    }
    
    return NULL;
}

int main() {
    ThreadSafeQueue queue;
    initQueue(&queue);
    
    pthread_t producerThread, consumerThread;
    pthread_create(&producerThread, NULL, producer, &queue);
    pthread_create(&consumerThread, NULL, consumer, &queue);
    
    pthread_join(producerThread, NULL);
    pthread_join(consumerThread, NULL);
    
    return 0;
}
