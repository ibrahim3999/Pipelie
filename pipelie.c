#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <time.h>

typedef struct Node {
    void* data;
    struct Node* next;
} Node;

typedef struct {
    Node* head;
    Node* tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Queue;

typedef struct {
    Queue* q;
    void* (*func)(void*);
    pthread_t worker;
    bool stop_requested;
} ActiveObject;

typedef struct {
    unsigned int number;
    bool isPrime;
} NumberResult;

Queue* createQueue() {
    Queue* q = malloc(sizeof(Queue));
    if (q == NULL) {
        fprintf(stderr, "Failed to allocate memory for queue\n");
        exit(EXIT_FAILURE);
    }
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
    return q;
}

void* enqueue(Queue* q, void* item) {
    Node* new_node = malloc(sizeof(Node));
    if (new_node == NULL) {
        fprintf(stderr, "Failed to allocate memory for new node\n");
        exit(EXIT_FAILURE);
    }
    new_node->data = item;
    new_node->next = NULL;

    pthread_mutex_lock(&q->mutex);
    if (q->tail != NULL) {
        q->tail->next = new_node;
    } else if(q->head == NULL) {
        q->head = new_node;
    }else if(q->tail  == NULL){
         q->tail = new_node;
    }
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

void* dequeue(Queue* q) {
    pthread_mutex_lock(&q->mutex);
    while (q->head == NULL) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    Node* head = q->head;
    void* item = head->data;
    q->head = head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }

    pthread_mutex_unlock(&q->mutex);

    free(head);
    return item;
}

void freeQueue(Queue* q) {
    pthread_mutex_lock(&q->mutex);
    Node* current = q->head;
    while (current != NULL) {
        Node* to_free = current;
        current = current->next;
        free(to_free);
    }
    pthread_mutex_unlock(&q->mutex);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
    free(q);
}

void* activeObjectThread(void* arg) {
    ActiveObject* self = (ActiveObject*)arg;
    if (self == NULL) {
        fprintf(stderr, "Invalid active object\n");
        return NULL;
    }

    void* task;
    while (true) {
        task = dequeue(self->q);
        if (task == NULL) {
            break;
        }
        if (self->stop_requested) {
            break;
        }
        self->func(task);
    }

    return NULL;
}

ActiveObject* createActiveObject(void* (*func)(void*)) {
    ActiveObject* obj = malloc(sizeof(ActiveObject));
    if (obj == NULL) {
        fprintf(stderr, "Failed to allocate memory for active object\n");
        exit(EXIT_FAILURE);
    }
    obj->q = createQueue();
    obj->func = func;
    obj->stop_requested = false;
    pthread_create(&obj->worker, NULL, activeObjectThread, obj);
    return obj;
}

void stop(ActiveObject* obj) {
    obj->stop_requested = true;
    enqueue(obj->q, NULL);
    pthread_join(obj->worker, NULL);
    freeQueue(obj->q);
    free(obj);
}

Queue* getQueue(ActiveObject* obj) {
    return obj->q;
}

bool isPrime(unsigned int num) {
    if (num < 2) return false;
    if (num == 2) return true;
    if (num % 2 == 0) return false;
    for (unsigned int i = 3; i <= sqrt(num); i += 2) {
        if (num % i == 0) return false;
    }
    return true;
}

void* first_func(void* arg) {
    ActiveObject* second_AO = (ActiveObject*)arg;
    NumberResult* result = malloc(sizeof(NumberResult));
    if (result == NULL || second_AO == NULL) {
        return NULL;
    }
    result->number = rand() % 900000 + 100000;
    result->isPrime = isPrime(result->number);
    printf("%u\n%s\n", result->number, result->isPrime ? "true" : "false");
    printf("Yes\n");
    enqueue(getQueue(second_AO), result);
    printf("Yes\n");
    return NULL;
}

void* second_func(void* arg) {
    printf("Yes\n");
    ActiveObject* third_AO = (ActiveObject*)arg;
    NumberResult* result = (NumberResult*)dequeue(getQueue(third_AO));
    if (result == NULL) {
        // Enqueue a placeholder task to keep the active object running
        enqueue(getQueue(third_AO), NULL);
        return NULL;
    }

    result->number += 11;
    result->isPrime = isPrime(result->number);
    printf("%u\n%s\n", result->number, result->isPrime ? "true" : "false");
    enqueue(getQueue(third_AO), result);

    return NULL;
}

void* third_func(void* arg) {
    ActiveObject* fourth_AO = (ActiveObject*)arg;

    while (true) {
        NumberResult* result = (NumberResult*)dequeue(getQueue(fourth_AO));
        if (result == NULL) {
            break;
        }

        result->number -= 13;
        result->isPrime = isPrime(result->number);
        printf("%u\n%s\n", result->number, result->isPrime ? "true" : "false");
        enqueue(getQueue(fourth_AO), result);
    }

    return NULL;
}

void* fourth_func(void* arg) {
    ActiveObject* arg_obj = (ActiveObject*)arg;

    while (true) {
        NumberResult* result = (NumberResult*)dequeue(getQueue(arg_obj));
        if (result == NULL) {
            break;
        }

        result->number += 2;
        printf("%u\n", result->number);
        free(result);
    }

    return NULL;
}

int main(int argc, char** argv) {
    // Check if argument is provided
    if (argc < 2) {
        printf("Please provide a number of iterations as an argument.\n");
        return 1;
    }

    // Parse argument
    int iterations = atoi(argv[1]);
    int seed = (argc == 3) ? atoi(argv[2]) : time(NULL);

    // Initialize the random number generator
    srand(seed);

    // Initialize the active objects
    ActiveObject* first_AO = createActiveObject(first_func);
    ActiveObject* second_AO = createActiveObject(second_func);
    ActiveObject* third_AO = createActiveObject(third_func);
    ActiveObject* fourth_AO = createActiveObject(fourth_func);

    // Generate random numbers and feed them to the first active object

    void* task = malloc(sizeof(ActiveObject*));
    if (task == NULL) {
        fprintf(stderr, "Failed to allocate memory for task\n");
        return -1;
    }
    *(ActiveObject**)task = second_AO;
    enqueue(getQueue(first_AO), task);

    stop(fourth_AO); // Stop the fourth active object
    fourth_AO = createActiveObject(fourth_func); // Restart the fourth active object
    sleep(1); // Wait 1 second

    // Stop the remaining active objects
    stop(first_AO);
    stop(second_AO);
    stop(third_AO);
    return 0;
}
