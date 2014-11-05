/* CSCI 4061 F 2013 Assignment 3
 * Date: 11/11/2013
 * Names: Nicholas Padilla,Nha Nguyen,Jiayi Peng
 * IDs: 4195660,4477882,4645102 
 */
#include <stdio.h>
#include <stdlib.h> //malloc
#include <signal.h>
#include <sys/types.h>
#include "mm.h"

/* Return usec */
double comp_time(struct timeval time_s, struct timeval time_e) {

  double elap = 0.0;

  if (time_e.tv_sec > time_s.tv_sec) {
    elap += (time_e.tv_sec - time_s.tv_sec - 1) * 1000000.0;
    elap += time_e.tv_usec + (1000000 - time_s.tv_usec);
  }
  else {
    elap = time_e.tv_usec - time_s.tv_usec;
  }
  return elap;
}

/*
 * Name:			mm_init
 * Input arguments:	'mm'-address where a block of user defined length
 *						memory will be allocated. 
 *						Size = [num_chunks*chunk_size] BYTES
 *			 		'num_chunks'-number of spaces in memory of a given size.
 *					'chunk_size'-The number of bbytes in one chunk.
 * Output arguments:'0'-if mm is initiallized successfully.
 *					'1'-if memory cannot be allocated for the requested memory or
 *					 	or array for free memory address stack. 
 * Function:		Initializes mm be allocating memory for requested size data 
 *						storage and a array for a stack of unused memory addressed.
 *						It also stores the chunk size and the number of chunks in 
 *						in mm->chunk_size and mm->num_chunks respectively.
 */
int mm_init(mm_t *mm, int num_chunks, int chunk_size)
{	
	if( !(mm->memory = malloc( num_chunks * chunk_size  ) ) )
	{	
		//failure to allocate space for mm->memory
		errno = ENOMEM; //Flag for not enough memory
		return -1;
	}
	/*(1)mm->memory is single pointer, so when indexing it, we get the indexth byte (which is of no use).
	  (2)void ** mm->free_memory, so when indexing it, it automatically decrement or increment by 
	     the size of *void.
	  (3)malloc allocates what free_memory points to, not *free_memory points to. double pointer only ensures the incrementation is 
	     of single pointer size.
	     array[i]=*(array+i)
	
	  (4)For more, see double pointer: http://stackoverflow.com/questions/13974001/two-dimensional-array-implementation-using-double-pointer
	*/
	if( !(mm->free_memory = (void**)malloc( num_chunks*sizeof(void*) ) ) )
	{
		//failure to allocate space for free_memory stack array[mm->free_memory]
		free(mm->memory);
		errno = ENOMEM;//Flag for not enough memory
		return -1;
	}
	//Set mm internal constants
	mm->chunk_size = chunk_size;
	mm->num_chunks = num_chunks;
	mm->num_used = 0;	
	
	//assign pointer value for each element of the main array double pointer points to
	int i;
	for( i = 0 ; i < num_chunks; i++ )
	{
		mm->free_memory[i] = mm->memory + chunk_size * i;
	}
	return 0;
}

/*
 * Name:			get_mm
 * Input arguments:	'mm'-structure holding memory address for user defined "dynamic"
 *						address space.
 * Output arguments:'void*'-pointer to free memory address in mm->memory.
 *					'NULL'-All preallocated memory has been used.
 * Function:		Returns the next free memory address from the free memory stack.
 *						If stack is empty return NULL.
 */
void *mm_get(mm_t *mm) {
	if( mm->num_used == mm->num_chunks )
	{
		errno = ENOMEM;
		return NULL;
	}
	return mm->free_memory[(mm->num_used)++];
}

/*
 * Name:		   	mm_put
 * Input arguments:	'mm'-structure holding memory address for user defined "dynamic"
 *						address space.
 *			 		'chunk'-pointer to address in memory
 * Output arguments:void
 * Function:		adds the given pointer back to the stack of free memory, and sets 
 *						chunk pointer to NULL so the user no longer has access to that
 *						memory through that specific vairable.
 */
void mm_put(mm_t *mm, void *chunk) {
	mm->free_memory[--(mm->num_used)] = chunk;
	chunk = NULL;
}

/*
 * Name:			mm_release
 * Input arguments:	'mm'-structure holding memory address for user defined "dynamic"
 *						address space.
 * Output arguments:void
 * Function:		Frees all the "dynamic" memory space and free memory stack array 
 *						allocated by mm_init.
 */
void mm_release(mm_t *mm) {
	free(mm->free_memory);
	free(mm->memory);
}

/*
 * TODO - This is just an example of how to use the timer.  Notice that
 * this function is not included in mm_public.h, and it is defined as static,
 * so you cannot call it from other files.  Instead, just follow this model
 * and implement your own timing code where you need it.
 */
 
//static void timer_example() {
  //struct timeval time_s, time_e;

  ///* start timer */
  //gettimeofday (&time_s, NULL);

  ///* TODO - code you wish to time goes here */

  //gettimeofday(&time_e, NULL);

  //fprintf(stderr, "Time taken = %f msec\n",
          //comp_time(time_s, time_e) / 1000.0);
//}








