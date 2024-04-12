#ifndef BUFFERCACHE_H
#define BUFFERCACHE_H

#include "queue.h"
#include "stack.h"
#include <time.h>
#include <pthread.h>

#define DISK_BLOCKS 100 // also known as N
#define BLOCK_SIZE 4096
#define CACHE_SIZE 20

struct block {
    int block_nr;
    char data[BLOCK_SIZE];
    int dirty_bit; // 1 disk에 쓰이지 않음 0 쓰임
    int ref_count;// 참조횟수
};


typedef struct node {
    struct block *blk;
    struct node *next;
} Node;

typedef struct buffercache {
    Node *array[CACHE_SIZE];
    int items; // node의 개수 총합
    Queue *cachequeue; // FIFO에서 사용됨
    Stack *cachstack; // LRU에서 사용됨
} BufferCache;

typedef struct thread_args {
    int victim_block_nr;
    char data[BLOCK_SIZE];
} Args;

pthread_mutex_t lock;
volatile int exit_flag;

BufferCache *buffer_init();
void buffer_free(BufferCache *bc);
int hash(int input);
int buffered_read(BufferCache *buffercache, int block_nr, char *result);
void *direct_io(void *ptr);
void set_flag();
void *flush();
int delayed_write(BufferCache *buffercache, int block_nr, char *input, int mode);
int fifo(BufferCache *bc, char *data);
int lru(BufferCache* bc, char* data);
int lfu(BufferCache* bc, char* data);

//buffer.c
char *disk_buffer;
int disk_fd;
BufferCache *bc;

int os_read(int block_nr, char *user_buffer);
int os_write(int block_nr, char *user_buffer);
int lib_read(int block_nr, char *user_buffer);
int lib_write(int block_nr, char *user_buffer);
int init();

#endif
