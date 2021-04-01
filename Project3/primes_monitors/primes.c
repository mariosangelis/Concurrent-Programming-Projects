/*Angelis Marios-Kasidakis Theodoros*/
/*Prime number recognition with monitors*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <error.h>
#include <sched.h>
#include <fcntl.h>
struct T {
    pthread_t id;
    int iret;
    int job_flag;                                                    /*If job_flag==1,thread has work to do*/
    int position;                                                    /*Thread position in struct's array*/ 
    long int prime;                
    pthread_cond_t pcod;
};
struct T *info;
pthread_mutex_t mtx;
pthread_cond_t  thread_q,main_q;
int size,main_is_blocked,num_threads_blocked,close_flag,counter;
int primetest(long int v,int position);
void available_worker(void *args);/* routine in the philosophy of a monitor*/
void *thread_func(void *arg);
int main(int args,char* argv[]){
    if(args<2){
        fprintf(stderr,"Error - Not enough arguments\n");
        exit(EXIT_FAILURE);
    }
    int i;
    long int number;
    size=atoi(argv[1]);
    FILE* fp;
    main_is_blocked=0;
    num_threads_blocked=0;
    close_flag=0;
    counter=0;
    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&thread_q,NULL);
    pthread_cond_init(&main_q,NULL);
    fp = fopen("primes.txt", "r");
    if(fp==NULL){
		printf("Error with fopen().\n");
		exit(EXIT_FAILURE);
	}
    info = (struct T *)malloc(size*sizeof(struct T));
    for(i=0;i<size;i++){                                                    /*Create "size" threads-workers*/
        printf("Main is creating thread %d\n",i);
        info[i].position=i;
        info[i].job_flag=1;
        info[i].prime=-1;
        info[i].pcod = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
        info[i].iret=pthread_create(&info[i].id, NULL,thread_func,(void *)(&info[i]));
        if(info[i].iret){
            fprintf(stderr,"Error - pthread_create() return code: %d\n",info[i].iret);
            exit(EXIT_FAILURE);
        }
    }
    while(1){
        char string[100];
        if (fgets(string,100,fp) == NULL){                                  /*Reading numbers from a file*/
            pthread_mutex_lock(&mtx);
            close_flag=1;
            if(num_threads_blocked!=size){                                  
                pthread_cond_wait(&main_q,&mtx);                            /*Waiting all threads to get finished*/
            }
            break;
        }
        number=atol(string);
        pthread_mutex_lock(&mtx);
        if(num_threads_blocked!=0){                                         /*There is a thread inside wait queue*/
            pthread_cond_signal(&thread_q);                                 /*Wake up a thread*/
        }
        else{
            printf("Noone thread inside wait queue,main is blocking...\n");
            main_is_blocked=1;                                              /*There is noone thread inside wait queue*/
        }
        pthread_cond_wait(&main_q,&mtx);                                    /*Wait thread to react*/
        for(i=0;i<size;i++){                                     
            if(info[i].job_flag==0){
                info[i].prime=number;                                       /*Main is giving job to available worker*/
                info[i].job_flag=1;                                         /*Worker has job now*/
                pthread_cond_signal(&info[i].pcod);
                break;
            }
        }
        pthread_mutex_unlock(&mtx);
    }
    free(info);
    return(0);
}
void *thread_func(void *arg){
	struct T *thread_struct=(struct T*)arg;                                 /*Worker function*/
    while(1){
        available_worker(arg);
        primetest(info[thread_struct->position].prime,thread_struct->position);/*Execute primetest function*/
    }
    return(NULL);
}
void available_worker(void *args){
	pthread_mutex_lock(&mtx);
	struct T *thread_struct=(struct T*)args;
	if(main_is_blocked==0){                                                 /*Main is not blocked*/
		num_threads_blocked++;                                              /*A thread is in the waiting queue*/
		if(close_flag==1 && num_threads_blocked==size){                     /*Last thread wakes up main*/
			pthread_cond_signal(&main_q);
			pthread_mutex_unlock(&mtx); 
			pthread_exit(NULL);
		}
		pthread_cond_wait(&thread_q,&mtx);                                  /*Waiting main to react*/
		num_threads_blocked--;                                              /*A thread is out of the waiting queue*/
	}
	else{main_is_blocked=0;}
	info[thread_struct->position].job_flag=0; 
	pthread_cond_signal(&main_q);
	pthread_cond_wait(&info[thread_struct->position].pcod,&mtx);            /*Waiting main to give a prime number*/
	pthread_mutex_unlock(&mtx);
}
int primetest(long int v,int position){                                                    
    int  k,flag = 0;
    
    for(k = 2; k <= v/2; ++k){
        // condition for nonprime number
        if(v%k == 0){
            flag = 1;   
            break;
        }
    }
    if (v == 1) {printf("1 is neither a prime nor a composite number,from thread %d\n",position);}
    else{ 
        if (flag == 0){printf("%ld is a prime number,from thread %d\n",v,position);}
        else{printf("%ld is not a prime number,from thread %d\n",v,position);}
    }
    return 0;
}
