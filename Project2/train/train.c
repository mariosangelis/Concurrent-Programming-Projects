/*Angelis Marios,Kasidakis Theodoros*/
/*Train  with semaphores*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "semaphores.h"
#define SIZE 10
struct T {
  pthread_t id;
  int iret;
  int n;
};
struct T *passengers;
int train_sem,mtx,passengers_sem,empty_sem,main_sem;
long int file_size(FILE *f);
void *train_func(void *arg);
void *passengers_func(void *arg);
int train_size,train_capacity,close_flag,passengers_num,end_file,wait_or_on_train;

int main(int args,char *argv[]){

	if(args<2){
		fprintf(stderr,"Error - Not enough arguments\n");
		exit(EXIT_FAILURE);
	}
	pthread_t train_id;
	int i=0,train_iret,num_cars;
	FILE *fp;
	train_size=atoi(argv[1]);                 /*Indicates the capacity of the train*/
	train_sem=mysem_create(train_sem,0);
	main_sem=mysem_create(main_sem,0);
	passengers_sem=mysem_create(passengers_sem,1);
	mtx=mysem_create(mtx,1);
	empty_sem=mysem_create(empty_sem,0);

	train_capacity=0;                         /*Indicates each time the number of people who are inside train*/
	wait_or_on_train=0;                       /*Indicates the total number of people(inside and outside train) seen from threads*/
	close_flag=0;                             /*Flag which informs the train to terminate because the passengers are not enough*/
	passengers_num=0;                         /*Indicates the total number of people(inside and outside train) seen from main*/
	end_file=0;
    
	train_iret=pthread_create(&train_id, NULL,train_func,NULL); /*Train  thread's creation*/
	if(train_iret){
		fprintf(stderr,"Error - pthread_create() return code: %d\n",train_iret);
		exit(EXIT_FAILURE);
	}
	long int fsize;
	fp = fopen("passengers.txt", "r");
	if(fp==NULL){
		printf("Error with fopen().\n");
		exit(EXIT_FAILURE);
	}
	fsize=file_size(fp);
	num_cars=(((fsize+1)/2))+1;
	passengers=(struct T *)malloc(num_cars*sizeof(struct T));
	if(passengers == NULL){
		fprintf(stderr,"Error with memory allocation");
		exit(EXIT_FAILURE);
	}
	while(1){
		char string[100];
		if (fgets(string,100,fp) == NULL){                  /*Reading numbers from a file,if EOF->break*/
			mysem_down(mtx);                                /*Create a last thread to close all other threads*/
			end_file=1;
			passengers[i].n=i;
			passengers[i].iret=pthread_create(&passengers[i].id,NULL,passengers_func,(void *)(&passengers[i]));
			if(passengers[i].iret){
				fprintf(stderr,"Error - pthread_create() return code: %d\n",passengers[i].iret);
			}
			mysem_up(mtx);
			break;
		}
		else{
			mysem_down(mtx);
			passengers_num++;
			mysem_up(mtx);
		}
		passengers[i].n=i;
		passengers[i].iret=pthread_create(&passengers[i].id,NULL,passengers_func,(void *)(&passengers[i]));
		if(passengers[i].iret){
			fprintf(stderr,"Error - pthread_create() return code: %d\n",passengers[i].iret);
		}
		sleep(1);
		i++;
	}
	printf("Main is sleeping...\n");
	mysem_down(main_sem);                                   /*Main is sleeping until last thread wakes up her*/
	printf("Main is up...\n");
	mysem_destroy(main_sem);
	mysem_destroy(mtx);
	mysem_destroy(passengers_sem);
	mysem_destroy(empty_sem);
	mysem_destroy(train_sem);
	free(passengers);
	return(0);
}
void *train_func(void *arg){                                /*Train function*/

	while(1){
		mysem_down(train_sem);                              /*Train blocks until it is full*/
        mysem_down(mtx);
		if(close_flag==1){                                  /*Train is not full,but there are not any passengers*/
			mysem_up(main_sem);                             /*Wake up main and return*/
			break;
		}
		mysem_up(mtx);
		printf("Train is starting...\n");                   /*Train is starting*/
		sleep(3);
		printf("Train is finished...\n");                   /*Train is finished*/
		mysem_down(mtx);
		mysem_up(empty_sem);                                /*Train is back,so wake up the first passenger*/
        mysem_up(mtx);
	}
	return(NULL);
}
void *passengers_func(void *arg){

	struct T *thread_struct=(struct T*)arg;

    mysem_down(passengers_sem);                             /*Passenger is waiting*/
	mysem_down(mtx);
    wait_or_on_train++;
	if(train_capacity==train_size){                         /*Train is full,block until train is back*/
        mysem_up(mtx);
        mysem_down(empty_sem);
        mysem_down(mtx);
        train_capacity=0;                                   /*Train is back,first passenger goes into*/
    }
    if(train_capacity<train_size){                          /*Train is not full*/
		train_capacity++;
		if(end_file==0){                                    /*Main is still reading numbers from file*/
			/*Train is not full,passenger goes into*/
            if((wait_or_on_train <= passengers_num && train_capacity < train_size)){     
				printf("==Passenger(%d) is in the train==\n",thread_struct->n);
				mysem_up(passengers_sem);
			}
			/*Train is full,so train starts the ride*/
			else if(wait_or_on_train <= passengers_num && train_capacity == train_size){ 
				printf("==Passenger(%d) is in the train==\n",thread_struct->n);
				mysem_up(passengers_sem);
				mysem_up(train_sem);
			}
		}
		else{                                                              /*Main stopped reading numbers from file*/
            /*Train is not full,passenger goes into*/
			if(wait_or_on_train < passengers_num && train_capacity < train_size){        
				printf("==Passenger(%d) is in the train==\n",thread_struct->n);
				mysem_up(passengers_sem);
			}
			/*Train is full,so train starts the ride*/
			else if(wait_or_on_train < (passengers_num) && train_capacity == train_size){
				printf("==Passenger(%d) is in train==\n",thread_struct->n);
				mysem_up(passengers_sem);
				mysem_up(train_sem);
			}
			/*Last thread wakes up train and return*/
			else if(wait_or_on_train == passengers_num+1){
                /*Only the last thread was into else statement and train is not full,so terminate*/
                if(train_capacity<=train_size  && train_capacity!=1){
                    printf("Not enough passengers.Train is not starting.\n");
                }
				close_flag=1;
				mysem_up(train_sem);
			}
			/*The train is not full,so  the train does not starts,wake up the last thread to close*/
			else if(wait_or_on_train == passengers_num && train_capacity < train_size){  
				printf("==Passenger(%d) is in train==\n",thread_struct->n);
				//printf("Not enough passengers.Train is not starting.\n");
				mysem_up(passengers_sem);
			}
			/*Train starts the ride and terminates,wake up the last thread to close*/
			else if(wait_or_on_train == passengers_num && train_capacity == train_size){ 
				printf("==Passenger(%d) is in train==\n",thread_struct->n);
				mysem_up(passengers_sem);
				mysem_up(train_sem);
			}
		}
	}
    mysem_up(mtx);
    return(NULL);
}
long int file_size(FILE *f){
	
	long int size;
	
	fseek(f, 0L, SEEK_END);
	size=ftell(f);
	fseek(f, 0L, SEEK_SET);
	return(size);
}
