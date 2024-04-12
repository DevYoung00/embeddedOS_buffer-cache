#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "buffercache.h"
#include <pthread.h>
#include <unistd.h>

int disk_read(int block_nr) {
    char *temp;
    temp = malloc(BLOCK_SIZE);
    int ret;

	ret = lseek(disk_fd, block_nr * BLOCK_SIZE, SEEK_SET); 
	if (ret < 0)
		return ret;

	ret = read(disk_fd, temp, BLOCK_SIZE);
	if (ret < 0)
		return ret;

    fprintf(stdout, "[ DISK READ ]\tBlock number %d data: \"%.10s...\"\n", block_nr, temp);
    free(temp);
    return ret;
}

int main(int argc, char *argv[]) {
    int access_sequence_length = 30;
    int diskfile_block = 20;
    int buffer_block = CACHE_SIZE;
	clock_t start, end;
	char *sample = "sample#";
	int t=0;
	pthread_t flushing_thread;

    fprintf(stdout, "--------------------------------------------------------------------\n");
    fprintf(stdout, "Buffer Cache Simulator Test\n");
    fprintf(stdout, "--------------------------------------------------------------------\n");
    fprintf(stdout, "Initialize\n\n");

     char *algorithm = "FIFO";
    int algorithm_index = 0;
    if (argc > 1) {
      if (strcmp(argv[1], "FIFO") == 0) algorithm_index = 0;
        else if (strcmp(argv[1], "LRU") == 0) algorithm_index = 1;
       else algorithm_index = 2;
        algorithm = argv[1];
    }
    if (init() != 0) {
    	perror("initialize failed");
    	return -1;
    }
    bc = buffer_init();
    
    for (int i = 0; i < diskfile_block; i++) {
    	char *temp;
    	temp = malloc(BLOCK_SIZE);
    	sprintf(temp, "Initial data: block number %d", i);
    	os_write(i, temp);
    	free(temp);
    }
    
    fprintf(stdout, "Initial access sequence: ["); fflush(stdout);
    int seq_init[CACHE_SIZE] = {3, 19, 25, 14, 11, 5, 8, 54, 36, 28, 13, 56, 76, 61, 43, 52, 85, 40, 60};
    srand(time(NULL)+t); t++;
    for (int i = 0; i < buffer_block-1; i++) {
        // seq_init[i] = rand() % diskfile_block;
        fprintf(stdout, " %d", seq_init[i]); fflush(stdout);
    }
    // seq_init[buffer_block-2] = (seq_init[buffer_block-3] + 10) % diskfile_block;
    fprintf(stdout, " ]\n");
    
	for (int i = 0; i < buffer_block-1; i++) {
		char *data = (char *)malloc(strlen(sample) + 10);
 		strcpy(data, sample);
 		char number[5];
        sprintf(number, "%d", seq_init[i]);
		strcat(data, number);

		int temp = delayed_write(bc, seq_init[i], data, algorithm_index);
    	free(data);
		if (temp != 0)
			fprintf(stdout, "Write faild! Block numder: %d\n", seq_init[i]);
		else {
			char *temp_buffer = malloc(BLOCK_SIZE);
		    buffered_read(bc, seq_init[i], temp_buffer);
		    free(temp_buffer);
		}
    }

    printf("after init :");
    print_queue(bc->cachequeue);
    
    //flushing thread init
    if (pthread_create(&flushing_thread, NULL, flush, (void *)bc) != 0) {
        perror("thread creating failed");
    }
    pthread_detach(flushing_thread);
        
    fprintf(stdout, "--------------------------------------------------------------------\n");
    fprintf(stdout, "Buffered Read\n\n");
    
    fprintf(stdout, "Access sequence: ["); fflush(stdout);
    int seq_read[access_sequence_length];
    srand(time(NULL)+t); t++;
    for (int i = 0; i < access_sequence_length; i++) {
        seq_read[i] = rand() % diskfile_block;
        fprintf(stdout, " %d", seq_read[i]); fflush(stdout);
    }
    fprintf(stdout, " ]\n");
     int hit_count = 0;
    int miss_count = 0;
    // for (int i; i < access_sequence_length; i++) {
    // 	char *temp_buffer = malloc(BLOCK_SIZE);
    // 	buffered_read(bc, seq_read[i], temp_buffer);
    // 	free(temp_buffer);
    // }
     for (int i = 0; i < access_sequence_length; i++) {
        char *temp_buffer = malloc(BLOCK_SIZE);
        int ret = os_read(seq_read[i],temp_buffer);
        if(ret == 0) hit_count++;
        else miss_count++;
        // buffered_read(bc, access_seq[i], temp_buffer);
        //fprintf(stdout, "Write sus %d\n\n\n\n", ret);
        free(temp_buffer);
    }
    fprintf(stdout, "[Read] hit ratio : %.2f%% \n\n", (double)hit_count * 100 / 30);
    
    fprintf(stdout, "--------------------------------------------------------------------\n");
    fprintf(stdout, "Delayed Write\n\n");
    
    // fprintf(stdout, "Replacement Algorithm: FIFO\n");
        fprintf(stdout, "Replacement Algorithm: %s\n",algorithm);
    
    fprintf(stdout, "Access sequence: ["); fflush(stdout);
    int seq_fifo[access_sequence_length];
    srand(time(NULL)+t); t++;
    for (int i = 0; i < access_sequence_length; i++) {
        seq_fifo[i] = rand() % diskfile_block;
        fprintf(stdout, " %d", seq_fifo[i]); fflush(stdout);
    }
    fprintf(stdout, " ]\n");
    
    for (int i = 0; i < access_sequence_length; i++) {
    	char *data = (char *)malloc(strlen("sample FIFO data") + 10);
        sprintf(data, "#%d sample FIFO data", seq_fifo[i]);
    	delayed_write(bc, seq_fifo[i], data, algorithm_index);
    	
    	disk_read(seq_fifo[i]);
    	free(data);
    }
    
    // fprintf(stdout, "\nReplacement Algorithm: LRU\n");
    
    // fprintf(stdout, "Access sequence: ["); fflush(stdout);
    // int seq_lru[access_sequence_length];
    // srand(time(NULL)+t); t++;
    // for (int i = 0; i < access_sequence_length; i++) {
    //     seq_lru[i] = rand() % diskfile_block;
    //     fprintf(stdout, " %d", seq_lru[i]); fflush(stdout);
    // }
    // fprintf(stdout, " ]\n");
    
    // for (int i = 0; i < access_sequence_length; i++) {
    // 	char *data = (char *)malloc(strlen("sample LRU data") + 10);
    //     sprintf(data, "#%d sample LRU data", seq_lru[i]);
        
    // 	delayed_write(bc, seq_lru[i], data, 0);
    // 	free(data);
    // }
    
    // fprintf(stdout, "\nReplacement Algorithm: LFU\n");

    // fprintf(stdout, "Access sequence: ["); fflush(stdout);
    // int seq_lfu[access_sequence_length];
    // srand(time(NULL)+t); t++;
    // for (int i = 0; i < access_sequence_length; i++) {
    //     seq_lfu[i] = rand() % diskfile_block;
    //     fprintf(stdout, " %d", seq_lfu[i]); fflush(stdout);
    // }
    // fprintf(stdout, " ]\n");
    
    // for (int i = 0; i < access_sequence_length; i++) {
    // 	char *data = (char *)malloc(strlen("sample LFU data") + 10);
    //     sprintf(data, "#%d sample LFU data", seq_lfu[i]);
        
    // 	delayed_write(bc, seq_lfu[i], data, 0);
    // 	free(data);
    // }
    
    // exit_flag = 1;
    // if (pthread_join(flushing_thread, NULL) != 0) {
    //     perror("thread join failed");
    //     return 1;
    // }

    // flush thread 종료 조건
    set_flag();

    buffer_free(bc);
    free(disk_buffer);
    close(disk_fd);
    
    fprintf(stdout, "--------------------------------------------------------------------\n");
    fprintf(stdout, "Test Finished\n");
    
    return 0;
}
