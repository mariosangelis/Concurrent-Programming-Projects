/*Angelis Marios-Kasidakis Theodoros*/
/*Coroutines and semaphores implementation,using round robin theory*/
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <ucontext.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <sys/time.h>
#include "mythread.h"

#define ALARM_TIME 5*100000
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

struct T {                                                  /*Worker struct*/
	int job_flag;
	int position;
	long int prime;
	sem personal_sem;
};

struct T *info;
struct itimerval t = { {0} };
FILE* fp;
sem thread_sem,main_sem,mtx;
ucontext_t main_context;
int counter,size,close_master=0,destroy_flag;
void handler(int sig);
int mythreads_init();
void thread_function(thr_t *mythr,void *args);
void main_function(thr_t *mythr,struct T *thread_struct);
int primetest(long int v,int id);

int main(int args,char *argv[]) {

	int i;
	if(args<2){
		fprintf(stderr,"Error - Not enough arguments\n");
		exit(EXIT_FAILURE);
	}
	size=atoi(argv[1]);
	fp = fopen("primes.txt", "r");
	info = (struct T *)malloc(size*sizeof(struct T));

	for(i=0;i<size;i++){                                             /*Create "size" threads-workers*/
		info[i].position=i;
		info[i].job_flag=1;
		info[i].prime=-1;
		mythreads_sem_init(&(info[i].personal_sem),0);
	}
	sigemptyset(&s1);
	sigaddset(&s1,SIGALRM);
	counter=0;
	position=0;
	threads_list_init();
	struct sigaction act= { {0} };
	act.sa_handler=handler;
	sigaction(SIGALRM,&act,NULL);
    
	mythreads_init();
	free(head);

	return(0);
}

void main_function(thr_t *mythr,struct T *thread_struct) {

	t.it_value.tv_sec = 0;
	t.it_value.tv_usec = ALARM_TIME;
	t.it_interval.tv_sec = 0;
	t.it_interval.tv_usec= ALARM_TIME;
	setitimer(ITIMER_REAL,&t,NULL);

	int i;
	long int number;
	while(1){
		char string[100];
		if (fgets(string,100,fp) == NULL){                                  /*Reading numbers from a file,if EOF->break*/
			break;
		}
		number=atol(string);

		mythreads_sem_up(&thread_sem);                                      /*Wake up a thread*/
		mythreads_sem_down(&main_sem);                                      /*Waiting thread to react*/
		for(i=0;i<size;i++){                                                /*Main is trying to find an available worker*/
			if(info[i].job_flag==0){
				info[i].prime=number;                                       /*Main gives job to available worker*/
				info[i].job_flag=1;                                         /*Worker has job now*/
				mythreads_sem_up(&(info[i].personal_sem));
                break;
			}
		}
	}
	mythreads_sem_down(&mtx);
	close_master=1;                                                         /*Close_master=1*/
	mythreads_sem_up(&mtx);
	mythreads_sem_up(&thread_sem);                                          /*Wake up a thread to start closing*/
	mythreads_sem_down(&main_sem);                                          /*Waiting last thread to close*/
	swapcontext(&(current_node->source_context),&(main_context));

	return ;
}

void thread_function(thr_t *mythr,void *args) {
	struct T *thread_struct=(struct T*)args;
	while(1){
		mythreads_sem_down(&thread_sem);                                    /*Waiting main to increase thread_sum*/
		mythreads_sem_down(&mtx);                                           /*****************CS******************/
		while(close_master==1){
			counter++;
			mythreads_sem_up(&thread_sem);
			mythreads_sem_up(&mtx);
			while(1){
				mythreads_yield();
				if(counter==size){
					counter=0;
					mythreads_sem_up(&main_sem);                            /*Last thread wakes up main*/
				}
			}
		}
		mythreads_sem_up(&mtx);                                             /*****************CS******************/
		info[thread_struct->position].job_flag=0;                           /*Thread is available now*/
		mythreads_sem_up(&main_sem);                                        /*Wake up main*/
		mythreads_sem_down(&(info[thread_struct->position].personal_sem));  /*Waiting main to give a prime number*/
		primetest(info[thread_struct->position].prime,thread_struct->position);                     /*Execute primetest function*/
	}
}

/*Primetest function*/
int primetest(long int v,int id){
	int  k, flag = 0;

	for(k = 2; k <= v/2; ++k){
		if(v%k == 0){
			flag = 1;
			break;
		}
	}
	if (v == 1) {
		printf(ANSI_COLOR_RED"[thread %d] 1 is neither a prime nor a composite number.\n"ANSI_COLOR_RESET,(id+1));
	}
	else{
		if (flag == 0){printf(ANSI_COLOR_RED"[thread %d]  %ld is a prime number.\n"ANSI_COLOR_RESET, (id+1),v);}
		else{printf(ANSI_COLOR_RED"[thread %d] %ld is not a prime number.\n"ANSI_COLOR_RESET,(id+1),v);}
	}
	return 0;
}
int mythreads_init(){
    int i;
    thr_t main_thread;
	thr_t threads_array[size];
    
    mythreads_sem_init(&thread_sem,0);
	mythreads_sem_init(&main_sem,0);
	mythreads_sem_init(&mtx,1);

	mythreads_create(&main_thread,(void(*)(void))main_function,NULL);
	for(i=0;i<size;i++){                                             /*Create "size" threads-workers*/
		mythreads_create(&(threads_array[i]),(void(*)(void))thread_function,&(info[i]));
	}
	current_node=head->next;
	swapcontext(&(main_context),&(head->next->source_context));
	printf("Main is up\n");
	mythreads_destroy(&main_thread);
	for(i=0;i<size;i++){                                             /*Create "size" threads-workers*/
		mythreads_destroy(&(threads_array[i]));
	}
	mythreads_sem_destroy(&thread_sem);
	mythreads_sem_destroy(&main_sem);
	mythreads_sem_destroy(&mtx);
    
    return 0;
}
/*Signal handler for SIGALRM,restarts the alarm and swaps from current_node to current_node->next*/
void handler(int sig) {
	printf("\n==Alarm triggers==\n");
	mythreads_yield();
}
