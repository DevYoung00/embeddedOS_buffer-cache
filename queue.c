#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"
#include <pthread.h>

pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;

Queue *init_queue() {
    Queue *q = (Queue *)malloc(sizeof(Queue));
    for (int i = 0; i < QUEUE_SIZE; i++) {
        q->block_nums[i] = -1;
    }
    q->front = 0;
    q->rear = 0;
    q->l = &q_lock;
    return q;
}

void print_queue(Queue *q) {
    int index;
    printf("Queue : [ ");
    for (int i = 0; i < QUEUE_SIZE; i++) {
        index = (q->front + i) % QUEUE_SIZE;
        printf("%d ", q->block_nums[index]);
    }
    printf("] \n");
}

void free_queue(Queue *q) {
    free(q);
}

int is_full(Queue *q) {
    return (q->rear+1)%(QUEUE_SIZE)==q->front; // front값이 rear값보다 하나 앞서는 경우
}

int is_empty(Queue *q)  {
    return q->rear==(q->front); //front값과 rear값이 같은 경우
}

void enqueue(Queue *q, int block_nr) {
    if(is_full(q)){
        perror("Queue is Full");
        return ;
    }
    pthread_mutex_lock(q->l);
    q->rear=(q->rear+1)%(QUEUE_SIZE);
    q->block_nums[q->rear]=block_nr;
    pthread_mutex_unlock(q->l);
}

// int dequeue(Queue *q) {
//     if(is_empty(q)){
//         perror("Queue is Empty");
//     }
//     q->front = (q->front + 1) % QUEUE_SIZE;
//     int result = q->block_nums[(q->front)&(QUEUE_SIZE)];
//     q->block_nums[(q->front)&(QUEUE_SIZE)] = 0;
//     return result;
// }

int dequeue(Queue *q) {
    if (is_empty(q)) {
        perror("Queue is Empty");
        return -11;
    }
    pthread_mutex_lock(q->l);
    q->front = (q->front + 1) % QUEUE_SIZE;
    int result = q->block_nums[q->front];
    q->block_nums[q->front] = -1;
    pthread_mutex_unlock(q->l);
    return result; //front값을 증가시킨 후 반환

    // int dequeuedValue = q->block_nums[q->front];
    // q->front = (q->front + 1) % QUEUE_SIZE;

    // printf("\ndequeue is %d ", dequeuedValue);
    // return dequeuedValue;
}

void remove_value_from_queue(Queue *q, int value) {
    int current = q->front;
    int found = 0;  // 값을 찾았는지 여부를 나타내는 변수

    // 큐를 front부터 rear까지 탐색
    while (current != q->rear) {
        current = (current + 1) % QUEUE_SIZE;

        // 현재 위치의 값이 찾고자 하는 값과 일치하면 삭제
        if (q->block_nums[current] == value) {
            found = 1;  // 값을 찾음
            break;
        }
    }

    if (found) {
        // 값을 찾았을 경우, 해당 위치부터 rear까지 값을 한 칸씩 앞으로 당김
        while (current != q->rear) {
            q->block_nums[current] = q->block_nums[(current + 1) % QUEUE_SIZE];
            current = (current + 1) % QUEUE_SIZE;
        }

        // rear 위치의 값을 -1으로 초기화하고 rear를 하나 감소
        q->block_nums[q->rear] = -1;
        q->rear = (q->rear - 1 + QUEUE_SIZE) % QUEUE_SIZE;
    }
}