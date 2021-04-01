/*Angelis Marios-Kasidakis Theodoros*/
/*Prime number recognition*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <sched.h>
#include <fcntl.h>
struct T {
  pthread_t id;
  int iret;
  int job_flag;                   /*If job_flag==1,thread has work to do*/
  int close_flag;                 /*If close_flag==1,main is clossing this thread*/
  int position;                   /*Thread position in struct's array*/ 
  long int prime;                 
};
struct T *info;
int primetest(long int v);
void *thread_func(void *arg);
int main(int args,char* argv[]){
    if(args<2){
        fprintf(stderr,"Error - Not enough arguments\n");
        exit(EXIT_FAILURE);
    }
    int i,size,exit_flag,number_of_closed_threads=0;
    long int number;
    size=atoi(argv[1]);
    FILE* fp;
    fp = fopen("primes.txt", "r");
    info = (struct T *)malloc(size*sizeof(struct T));
    for(i=0;i<size;i++){                           /*Create "size" threads-workers*/
        info[i].position=i;
        info[i].job_flag=0;
        info[i].close_flag=0;
        info[i].iret=pthread_create(&info[i].id, NULL,thread_func,(void *)(&info[i]));
        if(info[i].iret){
            fprintf(stderr,"Error - pthread_create() return code: %d\n",info[i].iret);
            exit(EXIT_FAILURE);
        }
    }
    while(1){
        char string[100];
        if (fgets(string,100,fp) == NULL){              /*Reading numbers from a file,if EOF->break*/
            break;
        }
        number=atol(string);
        while(1){
            for(i=0;i<size;i++){                    /*Main is trying to find an available worker*/
                if(info[i].job_flag==0){
                    info[i].prime=number;               /*Main gives job to available worker*/
                    info[i].job_flag=1;                 /*Worker has job now*/
                    exit_flag=1;
                    break;
                }
            }
            if(exit_flag==1){
                exit_flag=0;
                break;
            }
        }
    }
    while(1){                                           /*Main is waiting for all threads to finish work*/
        for(i=0;i<size;i++){                        /*Main is closing all threads and returns*/
            if(info[i].job_flag==0 && info[i].close_flag==0){
                info[i].close_flag=1;
                number_of_closed_threads++;
            }
        }
        if(number_of_closed_threads==size){break;}
    }
    return(0);
}
void *thread_func(void *arg){                           /*Worker function*/
    struct T *thread_struct=(struct T*)arg;
    while(1){                                           /*Worker thread is spinning until main gives him work*/
        while(info[thread_struct->position].job_flag==0 && info[thread_struct->position].close_flag==0){}
        if(info[thread_struct->position].close_flag==1){     /*Main has no more work to give,all threads must close*/
            break;
        }
        primetest(info[thread_struct->position].prime); 
        info[thread_struct->position].job_flag=0;           /*Thread is available now*/
    }
    return(NULL);
}
int primetest(long int v){                                                    
    int  i, flag = 0;
    
    for(i = 2; i <= v/2; ++i){
        // condition for nonprime number
        if(v%i == 0){
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

