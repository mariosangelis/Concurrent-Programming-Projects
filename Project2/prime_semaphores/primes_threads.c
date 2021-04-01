/*Angelis Marios-Kasidakis Theodoros*/
/*Prime number recognition with semaphores*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <sched.h>
#include <fcntl.h>
#include "semaphores.h"
struct T {
    pthread_t id;
    int iret;
    int job_flag;                                                    /*If job_flag==1,thread has work to do*/
    int position;                                                    /*Thread position in struct's array*/ 
    long int prime;                
    int sem;
};
struct T *info;
int main_sem,close_sem,sem,thread_sem;
int counter=0,size,close_master=0;
int primetest(long int v);
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
    fp = fopen("primes.txt", "r");
    if(fp==NULL){
		printf("Error with fopen().\n");
		exit(EXIT_FAILURE);
	}
    main_sem=mysem_create(main_sem,0);
    thread_sem=mysem_create(thread_sem,0);
    sem=mysem_create(sem,1);
    info = (struct T *)malloc(size*sizeof(struct T));
    for(i=0;i<size;i++){                                             /*Create "size" threads-workers*/
        info[i].position=i;
        info[i].job_flag=1;
        info[i].prime=-1;
        info[i].sem=mysem_create(info[i].sem,0);
        info[i].iret=pthread_create(&info[i].id, NULL,thread_func,(void *)(&info[i]));
        if(info[i].iret){
            fprintf(stderr,"Error - pthread_create() return code: %d\n",info[i].iret);
            exit(EXIT_FAILURE);
        }
    }
    while(1){
        char string[100];
        if (fgets(string,100,fp) == NULL){                           /*Reading numbers from a file,if EOF->break*/
            break;
        }
        number=atol(string);
        mysem_up(thread_sem);                                        /*Wake up a thread*/
        mysem_down(main_sem);                                        /*Waiting thread to react*/
        for(i=0;i<size;i++){                                         /*Main is trying to find an available worker*/
            if(info[i].job_flag==0){
                  info[i].prime=number;                              /*Main gives job to available worker*/
                  info[i].job_flag=1;                                /*Worker has job now*/
                  mysem_up(info[i].sem);
                  break;
            }
        }
    }
    mysem_down(sem);                                                 /*****************CS******************/
    close_master=1;                                                  /*Wait all threads to finish and close them*/
    mysem_up(sem);                                                   /*****************CS******************/
    mysem_up(thread_sem);                                            /*Wake up a thread to start closing*/
    mysem_down(main_sem);                                            /*Waiting last thread to close*/
    mysem_destroy(main_sem);                                         /*Destroy all semaphores and free memory*/
    mysem_destroy(thread_sem);
    mysem_destroy(sem);
    free(info);
    return(0);
}
void *thread_func(void *arg){                                        /*Worker function*/
    struct T *thread_struct=(struct T*)arg;
    while(1){
        mysem_down(thread_sem);                                      /*Waiting main to increase thread_sum*/
        mysem_down(sem);                                             /*****************CS******************/
        if(close_master==1 && counter<size-1){                       /*This is not the last thread,so wake up another thread and return*/
            counter++;
            mysem_up(thread_sem);
            mysem_up(sem);
            break;
        }
        else if(close_master==1 && counter==size-1){                 /*This is the last thread ,so wake up main and return*/
            mysem_up(main_sem);
            break;
        }
        mysem_up(sem);                                               /*****************CS******************/
        info[thread_struct->position].job_flag=0;                    /*Thread is available now*/
        mysem_up(main_sem);                                          /*Wake up main*/
        mysem_down(info[thread_struct->position].sem);               /*Waiting main to give a prime number*/
        primetest(info[thread_struct->position].prime);              /*Execute primetest function*/
    }
    return(NULL);
}
int primetest(long int v){                                                    
    int  k, flag = 0;
    
    for(k = 2; k <= v/2; ++k){
        // condition for nonprime number
        if(v%k == 0){
            flag = 1;
            break;
        }
    }
    if (v == 1) {
      printf("1 is neither a prime nor a composite number.\n");
    }
    else{ 
        if (flag == 0)
          printf("%ld is a prime number.\n", v);
        else
          printf("%ld is not a prime number.\n", v);
    }
    return 0;
}
