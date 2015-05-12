#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define NP 2	//number of producers
#define NC 1	//number of consumers
#define NS 10	//number of slots in the buffer
#define ITEM_COUNT 20 //the total number of items to produce

int buffer[NS];	//this is the buffer itself
int iIndex = 0;	//this is the next in index
int oIndex = 0;	//this is the next out index
int occupied = 0;	//the number of occupied slots in the buffer

pthread_mutex_t mutex;	//the mutex
pthread_t threads[NP + NC];	//the pthreads
pthread_cond_t empty_buffer_cv;	//the empty buffer condition
pthread_cond_t full_buffer_cv;	//the full buffer condition

struct arg_struct {
    int arg1;
    int arg2;
};

/*---------------- Begin Produce -------------*/
void *Produce(void *t){

    struct arg_struct *args = (struct arg_struct*)t;
    int threadid = (*args).arg1;
    int numberOfItems = (*args).arg2;
	int produced = 0;
	int x = rand() % 1000000 + 1;
    printf("\t*CREATED* Producer: %d \tSleep: %d\n",threadid,x);

    while(1)
    {
	    while (produced<numberOfItems) //while we're allowed to produce more
	    {
	    	//Begin mutex lock
			//printf("Producer %d locking mutex\n", threadid);
			pthread_mutex_lock(&mutex);
			while(occupied>=NS)//while we DONT have the ability to produce, wait for signal
			{
				//printf("\tProducer %d waiting for condition\n", threadid);
				pthread_cond_wait(&empty_buffer_cv, &mutex);
				//printf("\tProducer %d received condition\n", threadid);
			}
	    	//Produce!
		   	printf("\tProducer: %d \tAdded %d\n",threadid, iIndex);
		   	buffer[iIndex] = iIndex;
		   	iIndex = (iIndex+1) % NS;
		   	occupied++;
		   	produced++;
			//printf("\tProducer %d is incrementing produced to %d\n", threadid, produced);
			if(occupied >= NS)
			{
				//the buffer is full send a signal saying so
				//printf("\tProducer %d has filled the buffer, and needs to produce %d items\n",threadid, numberOfItems-produced);
				pthread_cond_signal(&full_buffer_cv);
			}
			//Release mutex lock
			//printf("Producer %d unlocking mutex\n", threadid);
			pthread_mutex_unlock(&mutex);
			//Sleep for x cycles
			float tmp;
			for(int i=0; i<x; i++)
			{
				tmp = i/x;
			}
	    }
		break;
	}
	//printf("\tProducer %i is done producing\n",threadid);
	//the producer is done producing
    //printf("\tProducer %d is exiting\n", threadid);
	pthread_exit(NULL);
}

/*---------------- Begin Consume -------------*/
void *Consume(void *t)
{
	int threadid = (int)t;
	int x = rand() % 1000000 + 1;
    printf("\t*CREATED* Consumer: %d \tSleep: %d\n",threadid, x);

	while (1) 
	{
		//Begin mutex lock
		//printf("Consumer %d locking mutex\n", threadid);
		pthread_mutex_lock(&mutex);
		while(occupied<1) //while we cannot consume, wait until we can
		{
			//printf("\tConsumer %d waiting for condition\n", threadid);
			pthread_cond_wait(&full_buffer_cv, &mutex);
			//printf("\tConsumer %d received condition\n",threadid);
		}
		if(buffer[oIndex] == EOF) //if the next slot holds the EOF marker, we're done
		{
			//printf("\tConsumer %d has gotten the EOF at location %d\n",threadid, oIndex);
			pthread_mutex_unlock (&mutex);
			break;
		}
		//Consume!
		printf("\tConsumer: %d \tgot %d\n",threadid, buffer[oIndex]);
    	oIndex = (oIndex+1) % NS;
    	occupied--;
    	//printf("\tConsumer %d is seeing occupied as %d\n",threadid, occupied);
    	if(occupied<1) //send signal saying the buffer is empty
    	{
    		//printf("\tConsumer %d has emptied the buffer\n",threadid);
    		pthread_cond_signal(&empty_buffer_cv);
    	}
		//Release mutex lock
		//printf("Consumer %d unlocking mutex\n", threadid);
		pthread_mutex_unlock (&mutex);
		//Sleep for x cycles
		float tmp;
		for(int i=0; i<x; i++)
		{
			tmp = i/x;
		}
	}
	//printf("\tConsumer %d is exiting\n", threadid);
    pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
	printf("\n\tBeginning Pthread Assignment\n\t----------------------------\n");

	//create array of struct pointers
	struct arg_struct *args[NP+NC];
    for(int i=0; i<(NP+NC); i++){
    	args[i] = malloc(sizeof(struct arg_struct));
    }

	//Seed random number generator
	srand(time(NULL));
	//Create mutex
	pthread_mutex_init(&mutex, NULL);
	//Create the conditions
	pthread_cond_init (&full_buffer_cv, NULL);
	pthread_cond_init (&empty_buffer_cv, NULL);

    for(int i=0; i<(NP+NC); i++){
		if(i<NP){
			(*args[i]).arg1 = i;
			(*args[i]).arg2 = ITEM_COUNT/NP;
			//printf("We're creating a producer %d with numberOfItems %d\n", (*args[i]).arg1, (*args[i]).arg2);
			//create producers
			pthread_create(&threads[i], NULL, Produce, (void*) args[i]);
		}
		else{
			//printf ("We're creating a consumer %d\n", i);
			//create consumers
			pthread_create(&threads[i], NULL, Consume, (void*)i);
		}
    }

    for(int i=0; i<NP; i++){
    	//rejoin the threads
		pthread_join(threads[i], NULL);
    }
    //printf ("\tMain(): Finished with producer join.\n");
    //the producers are all done producing, place EOF in buffer and notify all consumers
    pthread_mutex_lock(&mutex);
	while(occupied>=NS)//if the buffer is full, wait until we can put the EOF into it
	{
		//printf("\tMain is waiting for condition\n");
		pthread_cond_wait(&empty_buffer_cv, &mutex);
		//printf("\tProducer %d received condition\n", threadid);
	}
	//printf("\tMain is putting EOF in buffer with occupied/NS ratio of %d/%d\n",occupied,NS);
    buffer[iIndex] = EOF;
    occupied++;
	pthread_cond_broadcast(&full_buffer_cv);
	pthread_mutex_unlock(&mutex);

    for(int i=0; i<(NP+NC); i++){
    	//rejoin the threads
		pthread_join(threads[i], NULL);	
    }
    /* Clean up and exit */
    for(int i=0; i<(NP+NC); i++){
    	free(args[i]);
    }
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&empty_buffer_cv);
	pthread_cond_destroy(&full_buffer_cv);
    pthread_exit(NULL);
}