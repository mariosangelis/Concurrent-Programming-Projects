/*Angelis Marios,Kasidakis Theodoros*/
/*Narrow bridge with semaphores*/
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
#include "semaphores.h"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
struct T {
	pthread_t id;
	int iret;
	int type;                      /*0 for red ,1 for blue*/
	int sleep_time;
};

long int file_size(FILE *f);
void *thread_func(void *arg);
int bridge_size,red_remain,blue_remain,blue_waiting,red_waiting,change_counter,close_flag,red_empty_flag,blue_empty_flag;
int blue_sem,red_sem,main_sem,mtx;

int main(int args,char *argv[]){
	if(args<2){
		fprintf(stderr,"Error - Not enough arguments\n");
		exit(EXIT_FAILURE);
	}
	bridge_size=atoi(argv[1]);                     /*Size of bridge*/
	blue_sem=mysem_create(blue_sem,0);             /*Semaphore for blue cars*/
	red_sem=mysem_create(red_sem,0);               /*Semaphore for red cars*/
	mtx=mysem_create(mtx,1);                       /*Semaphore for closing all critical sections*/
	main_sem=mysem_create(main_sem,0);             /*With this semaphore,main is waiting until last thread returns */
	red_remain=0;                                  /*How much red cars are over the bridge*/
	blue_remain=0;                                 /*How much blue cars are over the bridge*/
	blue_waiting=0;                                /*How much blue cars are on the right side*/
	red_waiting=0;                                 /*How much red cars are on the left side*/
	change_counter=0;                              /*To avoid starvation,when "bridge_size"cars pass the bridge,change side*/
	close_flag=0;                                  /*Only one thread wakes up main*/
	blue_empty_flag=0;                             /*Blue thread does not see neither blue waiting cars nor red waiting cars*/
	red_empty_flag=0;                              /*Red thread does not see neither blue waiting cars nor red waiting cars*/
	FILE* fp;
	int i=0,first=0,num_cars;
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
        mysem_down(mtx);
		char string[100];
		if (fgets(string,100,fp) == NULL){               /*Reading numbers from a file,if EOF->break*/
			close_flag=1;
            if(blue_empty_flag==1 || red_empty_flag==1){ /*Create an extra thread for clossing*/
                car[i].type=1;
                car[i].iret=pthread_create(&car[i].id, NULL,thread_func,(void *)(&car[i]));
                if(car[i].iret){
                    fprintf(stderr,"Error - pthread_create() return code: %d\n",car[i].iret);
                    exit(EXIT_FAILURE);
                }
            }
			mysem_up(mtx);
			break;
		}
		car[i].type=atoi(string);
		fgets(string,100,fp);
		car[i].sleep_time=atoi(string);
		car[i].iret=pthread_create(&car[i].id, NULL,thread_func,(void *)(&car[i]));
		if(car[i].iret){
			fprintf(stderr,"Error - pthread_create() return code: %d\n",car[i].iret);
			exit(EXIT_FAILURE);
		}
		if(car[i].type==1){                              /*Blue car*/
			blue_waiting++;
			if(first==0){
				mysem_up(blue_sem);
				first=1;
			}
		}
		else if(car[i].type==0){                        /*Red car*/
			red_waiting++; 
			if(first==0){
				mysem_up(red_sem);
				first=1;
			}
		}
		i++;
		mysem_up(mtx);
	}
	printf("Main is sleeping...\n");
	mysem_down(main_sem);                          /*Waiting all threads to return*/
	printf("Main is closing...\n");
	mysem_destroy(main_sem);                       /*Delete all semaphores and free allocated memory*/
	mysem_destroy(blue_sem);
	mysem_destroy(red_sem);
	mysem_destroy(mtx);
    free(car);
	return(0);
}
void *thread_func(void *arg){
	struct T *thread_struct=(struct T*)arg;
	int up_flag=0;
       
	if(thread_struct->type==1){
        mysem_down(blue_sem);
        mysem_down(mtx);  
        /*A thread has been blocked and reading from file just stopped,so return*/
        if(close_flag==1 && (red_empty_flag==1 || blue_empty_flag==1)){
            mysem_up(main_sem);
            return(NULL);
        }
        if(red_empty_flag==1 || blue_empty_flag==1){
            /*Some red cars passed,all sides were empty,so thread was blocked(red_empty_flag=1) and now a blue car is coming*/
            if(red_empty_flag==1){
                change_counter=0;
                red_empty_flag=0;
                mysem_down(red_sem);
            }
            /*Some blue cars passed,all sides were empty,so thread was blocked(blue_empty_flag=1) and now a blue car is coming*/
            else if(blue_empty_flag==1){
                blue_empty_flag=0;
				mysem_down(red_sem);
            }
        }
    }
	else{
        mysem_down(red_sem);
        mysem_down(mtx);  
        if(red_empty_flag==1 || blue_empty_flag==1){
			/*Some blue cars passed,all sides were empty,so thread was blocked(blue_empty_flag=1) and now a red car is coming*/
            if(blue_empty_flag==1){
                blue_empty_flag=0;
                change_counter=0;
				mysem_down(blue_sem);
            }
            /*Some red cars passed,all sides were empty,so thread was blocked(red_empty_flag=1) and now a red car is coming*/
            else if(red_empty_flag==1){
                red_empty_flag=0;
                mysem_down(blue_sem);
            }
        }
    }
	if(thread_struct->type==1){                                  /*BLUE CAR*/
		blue_remain++;
		blue_waiting--;
		change_counter++;
		if(change_counter!=bridge_size && blue_waiting!=0){      /*Insert a blue car in the bridge*/
			up_flag=1;
			mysem_up(blue_sem);  
		}
	}
	else if(thread_struct->type==0){                             /*RED CAR*/
		red_remain++;
		red_waiting--;
		change_counter++;                
		if(change_counter!=bridge_size && red_waiting!=0){       /*Insert a red car in the bridge*/
			up_flag=1;
			mysem_up(red_sem);  
		}
	}
	mysem_up(mtx);                                         /*************************CS***************************/
	if(thread_struct->type==1){
		sleep(thread_struct->sleep_time);
		printf(ANSI_COLOR_BLUE"A blue car is passing over the bridge\n"ANSI_COLOR_RESET);
	}
	else{
		sleep(thread_struct->sleep_time);
		printf(ANSI_COLOR_RED"A red car is passing over the bridge\n"ANSI_COLOR_RESET);
	}
	mysem_down(mtx);                                       /*************************CS***************************/
	if(thread_struct->type==1){                                  
		blue_remain--;                                           /*A blue car just passed the bridge*/
		if((change_counter==bridge_size && blue_remain==0) || (blue_remain==0 && blue_waiting==0)){
			/*Change side from blue to blue beacuse counter=max or there are not red cars*/
			if(change_counter==bridge_size && red_waiting==0 && blue_waiting!=0){
				change_counter=0;
				mysem_up(blue_sem); 
			}
			/*Change side from blue to red because counter=max*/
			else if(change_counter==bridge_size && red_waiting!=0){  
				change_counter=0;
				mysem_up(red_sem); 
			}
			/*Change side from blue to red because all blue cars passed the bridge*/
			else if(change_counter!=bridge_size && blue_remain==0 && blue_waiting==0 && red_waiting!=0){
				change_counter=0;
				mysem_up(red_sem); 
			}
		}
		/*Do not do extra up's.Only one time a car can make an up*/
		else if(change_counter!=bridge_size && blue_waiting!=0 && up_flag==0){
			mysem_up(blue_sem); 
		}
		/*Last car wakes up main and return*/
		if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==1){
			mysem_up(main_sem);
			return(NULL);
		}
		/*Some blue cars passed the bridge an this thread does not see neither blue nor red cars*/
		else if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==0){
            printf("Blue car stacks...\n");
			blue_empty_flag=1;
            mysem_up(blue_sem); 
            mysem_up(red_sem); 
		}
	}
	else{                                                                   
		red_remain--;                                            /*A red car just passed the bridge*/
		if((change_counter==bridge_size && red_remain==0) || (red_remain==0 && red_waiting==0)){
			/*Change side from red to red beacuse counter=max or there are not blue cars*/
			if(change_counter==bridge_size && blue_waiting==0 && red_waiting!=0){
				change_counter=0;
				mysem_up(red_sem); 
			}
			/*Change side from red to blue because counter=max*/
			else if(change_counter==bridge_size &&blue_waiting!=0){
				change_counter=0;
				mysem_up(blue_sem);
			}
			/*Change side from red to blue because all red cars passed the bridge*/
			else if(change_counter!=bridge_size && red_remain==0 && red_waiting==0 && blue_waiting!=0){
				change_counter=0;
				mysem_up(blue_sem); 
			}
		}
		/*Do not do extra up's.Only one time a car can make an up*/
		else if(change_counter!=bridge_size && red_waiting!=0 && up_flag==0){
			mysem_up(red_sem); 
		}
		/*Last car wakes up main and return*/
		if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==1){
			mysem_up(main_sem);
			return(NULL);
		}
		/*Some red cars passed the bridge an this thread does not see neither blue nor red cars*/
		else if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==0){
            printf("Red car stacks...\n");
            red_empty_flag=1;
            mysem_up(blue_sem); 
            mysem_up(red_sem); 
		}
	}
	mysem_up(mtx);                                         /*************************CS***************************/
	return(NULL);
}
long int file_size(FILE *f){
	
	long int size;
	
	fseek(f, 0L, SEEK_END);
	size=ftell(f);
	fseek(f, 0L, SEEK_SET);
	return(size);
}
