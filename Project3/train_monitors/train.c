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

struct T {
  pthread_t id;
  int iret;
  int n;
};

struct T *passengers;
long int file_size(FILE *f);
void *train_func(void *arg);
void *passengers_func(void *arg);
/*routines in the philosophy of a monitor */
void train_starts();
void train_finishes();
void passenger_in_the_train(int id);
/**************************/
int train_size,train_capacity,close_flag,passengers_num,end_file,wait_or_on_train,first,num_blocked,train_blocked,main_blocked;
pthread_cond_t train_q,passengers_q,main_q,empty_passengers,train_passengers,last_one;
pthread_mutex_t mtx;

int main(int args,char *argv[]){

	if(args<2){
		fprintf(stderr,"Error - Not enough arguments\n");
		exit(EXIT_FAILURE);
	}
	pthread_t train_id;
	int i=0,train_iret,total_passengers;
	FILE *fp;
	train_size=atoi(argv[1]);                 /*Indicates the capacity of the train*/
	pthread_mutex_init(&mtx,NULL);
	pthread_cond_init(&main_q,NULL);
    pthread_cond_init(&train_q,NULL);
    pthread_cond_init(&passengers_q,NULL);
    pthread_cond_init(&empty_passengers,NULL);
    pthread_cond_init(&train_passengers,NULL);
    pthread_cond_init(&last_one,NULL);
	train_capacity=0;                         /*Indicates each time the number of people who are inside train*/
	wait_or_on_train=0;                       /*Indicates the total number of people(inside and outside train) seen from threads*/
	close_flag=0;                             /*Flag which informs the train to terminate because the passengers are not enough*/
	passengers_num=0;                         /*Indicates the total number of people(inside and outside train) seen from main*/
	end_file=0;
    first=0;
    train_blocked=0;
    num_blocked=0;
    main_blocked=0;
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
	total_passengers=(((fsize+1)/2))+1;
	passengers=(struct T *)malloc(total_passengers*sizeof(struct T));
	if(passengers == NULL){
		fprintf(stderr,"Error with memory allocation");
		exit(EXIT_FAILURE);
	}
	while(1){
		char string[100];
		if (fgets(string,100,fp) == NULL){                       /*Create an extra thread for closing*/
			pthread_mutex_lock(&mtx);
			end_file=1;
			passengers[i].n=i;
			passengers[i].iret=pthread_create(&passengers[i].id,NULL,passengers_func,(void *)(&passengers[i]));
			if(passengers[i].iret){
				fprintf(stderr,"Error - pthread_create() return code: %d\n",passengers[i].iret);
			}
			break;
		}
		else{
            pthread_mutex_lock(&mtx);
			passengers_num++;
            passengers[i].n=i;
            passengers[i].iret=pthread_create(&passengers[i].id,NULL,passengers_func,(void *)(&passengers[i]));
            if(passengers[i].iret){
                fprintf(stderr,"Error - pthread_create() return code: %d\n",passengers[i].iret);
            }
		}
		pthread_mutex_unlock(&mtx);
		//sleep(5);
		i++;
	}
	printf("Main is sleeping...\n");
    main_blocked++;
    if(main_blocked==1){pthread_cond_signal(&last_one);}
	pthread_cond_wait(&main_q,&mtx);                             /*Main is sleeping until last thread wakes up her*/
	printf("Main is up...\n");
	free(passengers);
	return(0);
}
void *train_func(void *arg){                                     /*Train function*/

	while(1){
        train_starts();
		if(close_flag==1){
			break;
		}
		printf("Train is starting...\n");                        /*Train is starting*/
		sleep(3);
		printf("Train is finished...\n");
		train_finishes();
	}
	return(NULL);
}
void *passengers_func(void *arg){
	struct T *thread_struct=(struct T*)arg;
    passenger_in_the_train(thread_struct->n);
    return(NULL);
}
void passenger_in_the_train(int id) {
	pthread_mutex_lock(&mtx);
    num_blocked++;
    if(num_blocked==1){pthread_cond_signal(&empty_passengers);}
    if(first==1){pthread_cond_wait(&passengers_q,&mtx);}
    else{first=1;}
    num_blocked--;
    wait_or_on_train++;
	if(train_capacity==train_size){train_capacity=0;}            /*Train is back,first passenger goes into*/
    if(train_capacity<train_size){                               /*Train is not full*/
		train_capacity++;
		if(end_file==0){                                         /*Main is still reading numbers from file*/
			/*Train is not full,passenger goes into*/
            if((wait_or_on_train <= passengers_num && train_capacity < train_size)){     
				printf("==Passenger(%d) is in the train==\n",id);
                if(num_blocked==0){pthread_cond_wait(&empty_passengers,&mtx);}
				pthread_cond_signal(&passengers_q);
			}
			/*Train is full,so train starts the ride*/
			else if(wait_or_on_train <= passengers_num && train_capacity == train_size){ 
				printf("==Passenger(%d) is in the train==\n",id);
                if(train_blocked==0){pthread_cond_wait(&train_passengers,&mtx);}
				pthread_cond_signal(&train_q);
			}
		}
		else{/*Main stopped reading numbers from file*/
            /*Train is not full,passenger goes into*/
			if(wait_or_on_train <= passengers_num && train_capacity < train_size){        
				printf("==Passenger(%d) is in the train==\n",id);
				if(num_blocked==0){pthread_cond_wait(&empty_passengers,&mtx);}
				pthread_cond_signal(&passengers_q);
			}
			/*Train is full,so train starts the ride*/
			else if(wait_or_on_train <= (passengers_num) && train_capacity == train_size){
				printf("==Passenger(%d) is in train==\n",id);
                if(train_blocked==0){pthread_cond_wait(&train_passengers,&mtx);}
				pthread_cond_signal(&train_q);
			}
			/*Last thread wakes up train and return*/
			else if(wait_or_on_train == passengers_num+1){
                /*Only the last thread was into else statement and train is not full,so terminate*/
                if(train_capacity<=train_size  && train_capacity!=1){printf("Not enough passengers.Train is not starting.\n");}
				close_flag=1;
				if(train_blocked==0){pthread_cond_wait(&train_passengers,&mtx);}
				pthread_cond_signal(&train_q);
			}
		}
	}
    pthread_mutex_unlock(&mtx);
}

void train_starts() {
	pthread_mutex_lock(&mtx);
	train_blocked++;
	if(train_blocked==1){pthread_cond_signal(&train_passengers);}
		pthread_cond_wait(&train_q,&mtx);
        train_blocked--;
		if(close_flag==1){                                       /*Train is not full,but there are not any passengers*/
			if(main_blocked==0){pthread_cond_wait(&last_one,&mtx);}
            pthread_cond_signal(&main_q);                        /*Wake up main and return*/
            pthread_mutex_unlock(&mtx);
			return;
		}
	pthread_mutex_unlock(&mtx);	
}
void train_finishes() {
	pthread_mutex_lock(&mtx);                                                         /*Train is finished*/
        if(num_blocked==0){pthread_cond_wait(&empty_passengers,&mtx);}
        pthread_cond_signal(&passengers_q);
		pthread_mutex_unlock(&mtx);
}
long int file_size(FILE *f){
	long int size;
	fseek(f, 0L, SEEK_END);
	size=ftell(f);
	fseek(f, 0L, SEEK_SET);
	return(size);
}
