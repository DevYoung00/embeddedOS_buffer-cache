#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_SIZE 21 // CACHE_SIZE + 1

typedef struct queue {
    int front;
    int rear;
    int block_nums[QUEUE_SIZE];
    pthread_mutex_t *l;
} Queue; // circular queue

Queue *init_queue();
void print_queue(Queue *q);
void free_queue(Queue *q);
int is_full(Queue *q);
int is_empty(Queue *q);
void enqueue(Queue *q, int block_nr);
void remove_value_from_queue(Queue *q,int value);
// int is_value_in_queue(Queue *q,int value);
int dequeue(Queue *q);

#endif