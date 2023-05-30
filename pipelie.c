#include <math.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>


typedef struct node {
    void *data;
    struct node *next;
} node;

typedef struct queue {
    node *head;
    node *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} queue;

typedef struct activeObject {
    pthread_t pthr;
    queue *queue;
    void *(*func)(void *);
} activeObject;

activeObject *AO1, *AO2, *AO3, *AO4;

bool is_prime(unsigned int num)
{
    if (num < 2)
        return false;
    
    for (unsigned int i = 2; i * i <= num; i++)
    {
        if (num % i == 0)
            return false;
    }
    
    return true;
}


void enqueue(queue *que, void *item)
{
    node *new_node = malloc(sizeof(node));
    if (new_node == NULL)
    {
        fprintf(stderr, "Allocate Memmory failed.\n");
        exit(EXIT_FAILURE);
    }

    new_node->data = item;
    new_node->next = NULL;

    pthread_mutex_lock(&(que->mutex));
    
    if (que->head == NULL)
    {
        que->head = new_node;
        que->tail = new_node;
    }
    else
    {
        que->tail->next = new_node;
        que->tail = new_node;
    }
    
    pthread_cond_signal(&(que->cond));
    pthread_mutex_unlock(&(que->mutex));
}


void *dequeue(queue *que)
{
    pthread_mutex_lock(&(que->mutex));
    
    while (que->head == NULL)
    {
        pthread_cond_wait(&(que->cond), &(que->mutex));
    }
    
    node *head = que->head;
    void *item = head->data;
    que->head = head->next;
    
    if (que->head == NULL)
    {
        que->tail = NULL;
    }
    
    pthread_mutex_unlock(&(que->mutex));
    
    free(head);
    return item;
}


void *start(void *arg){
    activeObject *self = (activeObject *)arg;
    void *task;
    while ((task = dequeue(self->queue)) != NULL)
    {
        self->func(task);
    }
    return NULL;
}

activeObject *createActiveObject(void *(*func)(void *))
{
    activeObject *obj = malloc(sizeof(activeObject));
    if (obj == NULL)
    {
        fprintf(stderr, "Allocate Memmory failed.\n");
        exit(EXIT_FAILURE);
    }
    obj->queue = malloc(sizeof(queue));
    if (obj->queue == NULL)
    {
        fprintf(stderr, "Allocate Memmory failed.\n");
        exit(EXIT_FAILURE);
    }
    pthread_mutex_init(&(obj->queue->mutex), NULL);
    pthread_cond_init(&(obj->queue->cond), NULL);
    obj->queue->head = NULL;
    obj->queue->tail = NULL;
    obj->func = func;
    pthread_create(&(obj->pthr), NULL, start, obj);
    return obj;
}

void freeQueue(queue *queue)
{
    pthread_mutex_lock(&(queue->mutex));
    node *current = queue->head;
    while (current != NULL)
    {
        node *to_free = current;
        current = current->next;
        free(to_free);
    }
    pthread_mutex_unlock(&(queue->mutex));
    pthread_mutex_destroy(&(queue->mutex));
    pthread_cond_destroy(&(queue->cond));
    free(queue);
}

void stop(activeObject *obj)
{
    enqueue(obj->queue, NULL);
    pthread_join(obj->pthr, NULL);
    freeQueue(obj->queue);
    free(obj);
}

void stopAll(activeObject *ao1, activeObject *ao2, activeObject *ao3, activeObject *ao4)
{
    stop(ao1);
    stop(ao2);
    stop(ao3);
    stop(ao4);
}

queue *getQueue(activeObject *obj)
{
    return obj->queue;
}

void *firstTaskFunc(void *argument)
{
    unsigned int *number_ptr = (unsigned int *)argument;

    printf("%u\n%s\n", *number_ptr ,is_prime(*number_ptr) ? "true" : "false");

    enqueue(getQueue(AO2), number_ptr);

    return NULL;
}

void *secondTaskFunc(void *argument)
{
    unsigned int *number = (unsigned int *)argument;

    *number += 11;

    printf("%u\n%s\n", *number, is_prime(*number) ? "true" : "false");

    enqueue(getQueue(AO3), number);

    return NULL;

}

void *thirdTaskFunc(void *argument)
{
    unsigned int *number = (unsigned int *)argument;

    *number -= 13;

    printf("%u\n%s\n", *number, is_prime(*number) ? "true" : "false");

    enqueue(getQueue(AO4), number);

    return NULL;

}



void *fourthTaskFunc(void *argument)
{
    unsigned int *number = (unsigned int *)argument;

    *number += 2;

    printf("%u\n", *number);

    free(number);

    return NULL;

}

void initializeActiveObjects(activeObject **ao1, activeObject **ao2, activeObject **ao3, activeObject **ao4)
{
    *ao1 = createActiveObject(firstTaskFunc);
    *ao2 = createActiveObject(secondTaskFunc);
    *ao3 = createActiveObject(thirdTaskFunc);
    *ao4 = createActiveObject(fourthTaskFunc);
}

void generateAndEnqueueNumbers(activeObject *ao, int numIterations)
{
    for (int i = 0; i < numIterations; i++)
    {
        unsigned int *num = malloc(sizeof(unsigned int));
        *num = rand() % 900000 + 100000;
        enqueue(getQueue(ao), num);
        printf("\n");

        if (i < numIterations - 1) {
            sleep(1);
        }
    }
}

int main(int argc, char **argv)
{
    if(argc > 2)
    {
        printf("Too many Arguments! \n");
        return 0;
    }

    if (argc < 2)
    {
        printf("Please add positive number of iterations.\n");
        return 0;
    }

    int iterations = atoi(argv[1]);
    
    int seed = argc == 3 ? atoi(argv[2]) : time(NULL);

    srand(seed);

    initializeActiveObjects(&AO1,&AO2,&AO3, &AO4);

    generateAndEnqueueNumbers(AO1,iterations);

    stopAll(AO1,AO2,AO3,AO4);

    return 0;
}
