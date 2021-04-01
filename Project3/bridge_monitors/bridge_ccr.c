/*Angelis Marios,Kasidakis Theodoros*/
/*Narrow bridge with monitors*/
/*Main is reading numbers from a file.If the first number is 1,car is blue,else car is red.The second number indicates how much time each car is sleeping when it's passing over the bridge*/
/*If there are unreadable numbers in the file but main loses the processor ,the last car  who sees the empty bridge and there are not any cars in both sides,must set his empty_flag to 1 */
/*Main will be read the next number from file and a red or a blue or a closing car will be the next thread.We tested all scenarios by putting a sleep after a "read" in main function*/
/*In general , we are changing side if change_counter is max(change_counter==bridge_size)*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "ccrlib.h"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
struct T {
	pthread_t id;
	int iret;
    int position;
	int type;                      /*0 for red ,1 for blue*/
	int sleep_time;
};
CCR_DECLARE(car_synchr)
long int file_size(FILE *f);
void *thread_func(void *arg);
int bridge_size,red_remain,blue_remain,blue_waiting,red_waiting,change_counter,close_flag,blue_change_side,red_change_side;
int main_waiting,blue_empty_flag,red_empty_flag;

int main(int args,char *argv[]){
	if(args<2){
		fprintf(stderr,"Error - Not enough arguments\n");
		exit(EXIT_FAILURE);
	}
	bridge_size=atoi(argv[1]);                     /*Size of bridge*/
	CCR_INIT(car_synchr)
	blue_remain=0;                                 /*How much blue cars are over the bridge*/
	blue_waiting=0;                                /*How much blue cars are on the right side*/
	red_waiting=0;                                 /*How much red cars are on the left side*/
	change_counter=0;                              /*To avoid starvation,when "bridge_size"cars pass the bridge,change side*/
	close_flag=0;                                  /*Only one thread wakes up main*/
	blue_change_side=0;
	red_change_side=0;
	main_waiting=0;
	blue_empty_flag=0;                             /*Blue thread does not see neither blue waiting cars nor red waiting cars*/
	red_empty_flag=0;
	int num_cars;                              /*Red thread does not see neither blue waiting cars nor red waiting cars*/
	FILE* fp;
	int i=0;
	long int fsize;
	fp = fopen("bridge.txt", "r");
	if(fp==NULL){
		printf("Error with fopen().\n");
		exit(EXIT_FAILURE);
	}
	fsize=file_size(fp);
	num_cars=(((fsize+1)/2)/2)+1;
	struct T *car=(struct T *)malloc(num_cars*sizeof(struct T));
    if(car == NULL){
		fprintf(stderr,"Error with memory allocation");
		exit(EXIT_FAILURE);
	}
	while(1){
		char string[100];
		if (fgets(string,100,fp) == NULL){
			CCR_EXEC(car_synchr,1,close_flag=1;
				if(blue_empty_flag==1 || red_empty_flag==1){ /*Create an extra thread for closing*/
					car[i].type=1;
					car[i].iret=pthread_create(&car[i].id, NULL,thread_func,(void *)(&car[i]));
					if(car[i].iret){
						fprintf(stderr,"Error - pthread_create() return code: %d\n",car[i].iret);
						exit(EXIT_FAILURE);
					}
				}
			)
			break;
		}
		car[i].position=i;
		car[i].type=atoi(string);
		fgets(string,100,fp);
		car[i].sleep_time=atoi(string);
		car[i].iret=pthread_create(&car[i].id, NULL,thread_func,(void *)(&car[i]));
		if(car[i].iret){
			fprintf(stderr,"Error - pthread_create() return code: %d\n",car[i].iret);
			exit(EXIT_FAILURE);
		}
		CCR_EXEC(car_synchr,1,printf("Main is reading...\n");
            if(car[i].type==1){blue_waiting++;}
            else if(car[i].type==0){red_waiting++;}
            i++;
        )
   //     sleep(2);
	}
	printf("Main is sleeping\n");
	CCR_EXEC(car_synchr,(main_waiting==1),printf("Main is closing\n");)
    free(car);
	return(0);
}
void *thread_func(void *arg){
	struct T *thread_struct=(struct T*)arg;
	if(thread_struct->type==1){                                                /*Blue car*/
		CCR_EXEC(car_synchr,((change_counter<bridge_size && red_remain==0) || blue_change_side==1 || blue_empty_flag==1 || red_empty_flag==1),blue_remain++;
		blue_waiting--;
		if(blue_empty_flag==1){blue_empty_flag=0;}
		else if(red_empty_flag==1){
			change_counter=0;
			red_empty_flag=0;
		}
		if(blue_change_side==1 && blue_remain==2){
			blue_change_side=0;
			blue_remain=1;
		}
		change_counter++;)
	}
	else{
		CCR_EXEC(car_synchr,((change_counter<bridge_size && blue_remain==0) || red_change_side==1 || blue_empty_flag==1 || red_empty_flag==1),red_remain++;
			red_waiting--;
			if(red_empty_flag==1){red_empty_flag=0;}
			else if(blue_empty_flag==1){
				change_counter=0;
				blue_empty_flag=0;
			}
			if(red_change_side==1 && red_remain==2){
				red_change_side=0;
				red_remain=1;
			}
			change_counter++;)
	}
    if(thread_struct->type==1){
		sleep(thread_struct->sleep_time);
		printf(ANSI_COLOR_BLUE"A blue car is passing over the bridge,thread %d\n"ANSI_COLOR_RESET,thread_struct->position);
	}
	else{
		sleep(thread_struct->sleep_time);
		printf(ANSI_COLOR_RED"A red car is passing over the bridge,thread %d\n"ANSI_COLOR_RESET,thread_struct->position);
	}
	if(thread_struct->type==1) {
		CCR_EXEC(car_synchr,1,blue_remain--;
			if((change_counter==bridge_size && blue_remain==0) || (blue_remain==0 && blue_waiting==0)){
				/*Change side from blue to blue beacuse counter=max or there are not red cars*/
				if(change_counter==bridge_size && red_waiting==0 && blue_waiting!=0){
					change_counter=0;
					blue_change_side=1;
					blue_remain++;
				}
				/*Change side from blue to red because counter=max*/
				else if(change_counter==bridge_size && red_waiting!=0){  
					change_counter=0;
					red_change_side=1;
					red_remain++;
				}
				/*Change side from blue to red because all blue cars passed the bridge*/
				else if(change_counter!=bridge_size && blue_remain==0 && blue_waiting==0 && red_waiting!=0){
					change_counter=0; 
					red_change_side=1;
					red_remain++;
				}
			}
			if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==0){
				printf("Blue car stacks...\n");
				//first=0;
				blue_empty_flag=1;
			}
			if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==1){
				main_waiting=1;
			}
		)
	}
	else {
		CCR_EXEC(car_synchr,1,red_remain--;
			if((change_counter==bridge_size && red_remain==0) || (red_remain==0 && red_waiting==0)){
				/*Change side from red to red beacuse counter=max or there are not blue cars*/
				if(change_counter==bridge_size && blue_waiting==0 && red_waiting!=0){
					change_counter=0; 
					red_change_side=1;
					red_remain++;
				}
				/*Change side from red to blue because counter=max*/
				else if(change_counter==bridge_size &&blue_waiting!=0){
					change_counter=0;
					blue_change_side=1;
					blue_remain++;
				}
				/*Change side from red to blue because all red cars passed the bridge*/
				else if(change_counter!=bridge_size && red_remain==0 && red_waiting==0 && blue_waiting!=0){
					change_counter=0;
					blue_change_side=1;
					blue_remain++;
				}
				if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==1){
					main_waiting=1;
				}
				if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==0){
					printf("Red car stacks...\n");
					red_empty_flag=1;
				}
			}
		)
	}
	return(NULL);
}
long int file_size(FILE *f){
	long int size;

	fseek(f, 0L, SEEK_END);
	size=ftell(f);
	fseek(f, 0L, SEEK_SET);
	return(size);
}
