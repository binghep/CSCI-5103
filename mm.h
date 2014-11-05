/* CSCI 4061 F 2013 Assignment 3
 * Date: 11/11/2013
 * Names: Nicholas Padilla,Nha Nguyen,Jiayi Peng
 * IDs: 4195660,4477882,4645102 
 */
#ifndef __MM_H
#define __MM_H

//#include <sys/time.h>
#include <errno.h>

//#define INTERVAL 0
//#define INTERVAL_USEC 500000

typedef struct {
	int chunk_size; //size of chunk of data
	int num_chunks; //number of chunks
	int num_used;	//Number of the chunks used
	void **free_memory; // an array storing memory address of all the chucks. before i=num_used are used chunks, after
	//i are free chuncks. When giving out free chunck, taking free_memory[num_used]. When is returned a memory chunk,
	//
	void *memory; //the memory space
} mm_t;

/* TODO - Implement these in mm_public.c */
//double comp_time(struct timeval time_s, struct timeval time_e);
int mm_init(mm_t *mm, int num_chunks, int chunk_size);
void *mm_get(mm_t *mm);
void mm_put(mm_t *mm, void *chunk);
void mm_release(mm_t *mm);



#endif
