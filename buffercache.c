#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "buffercache.h"
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
volatile int exit_flag = 0;

BufferCache *buffer_init()
{
    BufferCache *bc = (BufferCache *)malloc(sizeof(BufferCache));
    ;
    bc->items = 0;
    memset(bc->array, 0, sizeof(bc->array));
    Queue *q = init_queue(); // FIFO 용 큐 생성
    bc->cachequeue = q;

    Stack *s = init_stack(); // LRU 용 스택 생성
    bc->cachstack = s;

    return bc;
}

void buffer_free(BufferCache *bc)
{
    // 각 노드에 대한 메모리 해제
    for (int i = 0; i < CACHE_SIZE; i++)
    {
        Node *current = bc->array[i];
        while (current != NULL)
        {
            if (current->blk != NULL)
                free(current->blk);
            Node *next = current->next;
            free(current);
            current = next;
        }
    }

    free_queue(bc->cachequeue);
    // BufferCache 구조체에 대한 메모리 해제
    free(bc);
}

/* 해시함수 */
int hash(int input)
{
    return input % CACHE_SIZE;
}

int buffered_read(BufferCache *buffercache, int block_nr, char *result)
{
    // 시간 측정
    clock_t start, end;
    start = clock();

    if (block_nr < 0 || block_nr > DISK_BLOCKS - 1)
    {
        return -1;
    }
    int index = hash(block_nr);
    Node *current = buffercache->array[index];
    while (current != NULL)
    {
        // found
        if (current->blk->block_nr == block_nr)
        {
            memcpy(result, current->blk->data, BLOCK_SIZE);
            current->blk->ref_count++;

            // 스택의 맨 위로 push
            push(buffercache->cachstack, block_nr);

            end = clock();
            fprintf(stdout, "[ Hit ]\t\tBlock numder: %d, data: \"%.10s...\" time: %d ms\n", block_nr, result, (int)(end - start));
            return 0;
        }
        current = current->next;
    }
    end = clock();

    fprintf(stdout, "[ Miss ]\tBlock numder: %d, time: %d ms\n", block_nr, (int)(end - start));
    // direct_IO
    char *test_buf = (char *)malloc(BLOCK_SIZE);

    // int disk_fd = open("diskfile", O_RDWR|O_DIRECT);
    // if (lseek(disk_fd, 0 * BLOCK_SIZE, SEEK_SET) < 0)
    //     perror("lseek");
    // if (read(disk_fd, test_buf, BLOCK_SIZE) < 0)
    //     perror("read");
    // fprintf(stdout, "[ Miss ]\tdata: \"%.10s...\"", test_buf);
    // printf("[DIRECT I/O] : %s\n", test_buf);

    pthread_mutex_unlock(&lock);
    return -1;
}

// 스레드가 실행할 함수
void *direct_io(void *ptr)
{
    pthread_mutex_lock(&lock); // lock
    // int disk_fd = open("diskfile", O_RDWR|O_DIRECT);
    Args *args = (Args *)ptr;
    int block_nr = args->victim_block_nr;
    char *data = args->data;
    fprintf(stdout, "[ DIRECT I/O ]\tData: \"%.20s...\"\n", data);
    if (lseek(disk_fd, block_nr * BLOCK_SIZE, SEEK_SET) < 0)
        perror("disk is not mounted");
    if (write(disk_fd, data, BLOCK_SIZE) < 0)
    {
        perror("write - direct_io");
        pthread_mutex_unlock(&lock);
        return NULL;
    }
    fprintf(stdout, "[ DIRECT I/O ]\tBlock number %d has been written on disk.\n", block_nr);
    // close(disk_fd);
    pthread_mutex_unlock(&lock); // unlock
    return NULL;
}

void set_flag()
{
    exit_flag = 1;
}

// function for flush thread (every 15 secs)
void *flush()
{ // void *ptr) {
    // BufferCache *bc = (BufferCache *) ptr;
    fprintf(stdout, "\n[ FLUSH ]\tFlushing thread is running...\n");
    while (exit_flag == 0)
    {
        for (int i = 0; i < CACHE_SIZE; i++)
        {
            Node *current = bc->array[i];
            while (current != NULL)
            {
                if (current->blk->dirty_bit == 1)
                {
                    if (exit_flag != 0)
                        return NULL;
                    pthread_mutex_lock(&lock); // lock
                    current->blk->dirty_bit = 0;
                    int block_nr = current->blk->block_nr;
                    fprintf(stdout, "[ FLUSH ]\tDirty bit of Block number %d is now turned off.\n", block_nr);
                    char *data = current->blk->data;
                    fprintf(stdout, "[ FLUSH ]\tData: \"%.10s...\"\n", data);
                    // int disk_fd = open("diskfile", O_RDWR|O_DIRECT);
                    if (lseek(disk_fd, block_nr * BLOCK_SIZE, SEEK_SET) < 0)
                        perror("disk is not mounted");
                    if (write(disk_fd, data, BLOCK_SIZE) < 0)
                        perror("write - flush");
                    pthread_mutex_unlock(&lock); // unlock
                }
                current = current->next;
            }
        }
        sleep(15);
    }
    return NULL;
}

/* mode (버퍼캐시가 꽉 찼을때 한정) :: victim 선정 알고리즘
0 : FIFO
1 : LRU
2 : LFU */
int delayed_write(BufferCache *buffercache, int block_nr, char *input, int mode)
{
    if (block_nr < 0 || block_nr > DISK_BLOCKS - 1)
    {
        return -1;
    }

    int ret = 0;
    pthread_t thread;
    int victim_block_nr;

    // 버퍼캐시가 꽉참 -> victim 선정
    if (buffercache->items == CACHE_SIZE)
    {
        ret = -1;
        fprintf(stdout, "[ Write ]\tThere are no empty spaces.\n");
        char *data = (char *)malloc(BLOCK_SIZE);
        switch (mode)
        {
        case 0:
            victim_block_nr = fifo(buffercache, data);
            // printf("victim : %s\n", data);
            break;
        case 1:
            victim_block_nr = lru(buffercache, data);
            // printf("victim : %s\n", data);
            break;
            break;
        case 2:
            victim_block_nr = lfu(buffercache, data);
            // printf("victim : %s\n", data);
            break;
            break;
        default:
            return -1;
        }
        fprintf(stdout, "[ Write ]\tVictim block number is %d.\n", victim_block_nr);

        // 스레드 생성 -> direct_io
        Args *a = (Args *)malloc(sizeof(Args));
        a->victim_block_nr = victim_block_nr;
        strcpy(a->data, data);
        int tid = pthread_create(&thread, NULL, direct_io, (void *)a);
        if (tid < 0)
        {
            perror("thread creating failed");
            // return -1;
        }
        pthread_join(thread, NULL);
        free(a);
        free(data);
        // return -1;
    }
    else
        fprintf(stdout, "[ Write ]\tThere are empty spaces.\n");

    // buffer cache에 쓰기
    pthread_mutex_lock(&lock);

    fprintf(stdout, "[ Write ]\tBlock number: %d, data: \"%s\"\n", block_nr, input);
    struct block *blk = (struct block *)malloc(sizeof(struct block));
    blk->block_nr = block_nr;
    strcpy(blk->data, input);
    blk->dirty_bit = 1;
    blk->ref_count = 0;
    fprintf(stdout, "\t\tDirty bit of Block number %d is now turned on.\n", block_nr);
    Node *node = (Node *)malloc(sizeof(Node));
    node->blk = blk;
    node->next = NULL;

    int index = hash(block_nr);
    fprintf(stdout, "[ HASH ]\tBlock number %d has index %d\n", block_nr, index);

    Node *current = buffercache->array[index];
    if (current == NULL)
    {
        buffercache->array[index] = node;
    }
    else
    { // not NULL
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = node;
    }
    buffercache->items++;
    enqueue(buffercache->cachequeue, block_nr); // block_nr를 큐에 집어넣음
    push(buffercache->cachstack, block_nr);     // block_nr을 스택에 집어넣음

    pthread_mutex_unlock(&lock);
    return ret;
}

int fifo(BufferCache *bc, char *placeholder)
{
    Queue *q = bc->cachequeue;
    print_queue(q);
    int block_nr = dequeue(q);
    print_queue(q);
    int index = hash(block_nr);

    fprintf(stdout, "[ FIFO ]\tBlock number %d will be deleted.\n", block_nr);

    Node *current = bc->array[index];

    if (current == NULL) {
        printf(" !!!!!! Segmentation Fault occurs because current is NULL !!!!!!!!!!\n");
        return -1;
    }
    // victim이 첫번째 노드인 경우
    if (current->blk->block_nr == block_nr && current->next == NULL)
    {
        strcpy(placeholder, current->blk->data);
        free(current->blk);
        bc->array[index] = NULL;
        bc->items--;
        // 찾은 블록의 번호를 스택에서 삭제
        for (int i = 0; i <= bc->cachstack->top; ++i)
        {
            if (bc->cachstack->items[i] == block_nr)
            {
                for (int j = i; j < bc->cachstack->top; ++j)
                {
                    bc->cachstack->items[j] = bc->cachstack->items[j + 1];
                }
                --(bc->cachstack->top);
                break;
            }
        }
        return block_nr;
    } else if (current->blk->block_nr == block_nr) {
        strcpy(placeholder, current->blk->data);
        free(current->blk);
        bc->array[index] = current->next;
        bc->items--;
        return block_nr;
    }
    else
    {
        while (current->next != NULL)
        {
            // found
            if (current->next->blk->block_nr == block_nr)
            {
                strcpy(placeholder, current->blk->data);
                free(current->next->blk);
                current->next = NULL;
                bc->items--;
                // 찾은 블록의 번호를 스택에서 삭제
                for (int i = 0; i <= bc->cachstack->top; ++i)
                {
                    if (bc->cachstack->items[i] == block_nr)
                    {
                        for (int j = i; j < bc->cachstack->top; ++j)
                        {
                            bc->cachstack->items[j] = bc->cachstack->items[j + 1];
                        }
                        --(bc->cachstack->top);
                        break;
                    }
                }

                return block_nr;
            }
            current = current->next;
        }
    }
    perror("[ ERROR ]\tEntry Not found");
    return -1;
}

int lru(BufferCache *buffercache, char *placeholder)
{
    int oldest_block_nr = -1;

    if (buffercache->cachstack->top >= 0)
    {
        // 가장 오래된 블록은 스택의 젤 아래있음
        oldest_block_nr = buffercache->cachstack->items[0];
    }
    fprintf(stdout, "[ LRU ]\tBlock number %d will be deleted.\n", oldest_block_nr);

    if (oldest_block_nr != -1)
    {
        // 스택에서 victim 제거
        for (int i = 0; i <= bc->cachstack->top; ++i)
        {
            if (bc->cachstack->items[i] == oldest_block_nr)
            {
                for (int j = i; j < bc->cachstack->top; ++j)
                {
                    bc->cachstack->items[j] = bc->cachstack->items[j + 1];
                }
                --(bc->cachstack->top);
                break;
            }
        }

        int index = hash(oldest_block_nr);
        Node *current = buffercache->array[index];

        if (current->blk->block_nr == oldest_block_nr)
        {
            strcpy(placeholder, current->blk->data);
            free(current->blk);
            bc->array[index] = NULL;
            bc->items--;
            // dequeue(buffercache->cachequeue);
            remove_value_from_queue(buffercache->cachequeue, oldest_block_nr);
            return oldest_block_nr;
        }
        else
        {
            while (current->next != NULL)
            {
                // found
                if (current->next->blk->block_nr == oldest_block_nr)
                {
                    strcpy(placeholder, current->blk->data);
                    free(current->next->blk);
                    current->next = NULL;
                    bc->items--;
                    remove_value_from_queue(buffercache->cachequeue, oldest_block_nr);
                    return oldest_block_nr;
                }
                current = current->next;
            }
        }
    }

    perror("Entry Not found");
    return -1;
}

int lfu(BufferCache *buffercache, char *placeholder)
{
    int min_ref_count = 10000;
    int victim_block_nr = -1;

    for (int i = 0; i < CACHE_SIZE; i++)
    {
        Node *current = buffercache->array[i];
        while (current != NULL)
        {
            if (current->blk->ref_count < min_ref_count)
            {
                min_ref_count = current->blk->ref_count;
                victim_block_nr = current->blk->block_nr;
            }
            current = current->next;
        }
    }
    fprintf(stdout, "[ LFU ]\tBlock number %d will be deleted.\n", victim_block_nr);
    // Check if victim found
    if (victim_block_nr != -1)
    {
        int index = hash(victim_block_nr);

        Node *current = buffercache->array[index];

        if (current->blk->block_nr == victim_block_nr)
        {
            strcpy(placeholder, current->blk->data);
            free(current->blk);
            bc->array[index] = NULL;
            bc->items--;
            dequeue(buffercache->cachequeue);
            return victim_block_nr;
        }
        else
        {
            while (current->next != NULL)
            {
                // found
                if (current->next->blk->block_nr == victim_block_nr)
                {
                    strcpy(placeholder, current->blk->data);
                    free(current->next->blk);
                    current->next = NULL;
                    bc->items--;
                    dequeue(buffercache->cachequeue);
                    return victim_block_nr;
                }
                current = current->next;
            }
        }
    }

    perror("Entry Not found");
    return -1;
}
