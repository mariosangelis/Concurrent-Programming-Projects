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
int enter_bridge(int color); /* routine in the philosophy of a monitor */
void exit_bridge(int color,int wake_up); /* routine in the philosophy of a monitor */
long int file_size(FILE *f);
void *thread_func(void *arg);
int bridge_size,red_remain,blue_remain,blue_waiting,red_waiting,change_counter,close_flag,red_empty_flag,blue_empty_flag,num_red_blocked,num_blue_blocked,first,blue_blocked_flag,red_blocked_flag;
pthread_mutex_t mtx;
pthread_cond_t red_q,blue_q,main_q,no_car_in_queue;

int main(int args,char *argv[]){
	if(args<2){
		fprintf(stderr,"Error - Not enough arguments\n");
		exit(EXIT_FAILURE);
	}
	bridge_size=atoi(argv[1]);                     /*Size of bridge*/
	pthread_mutex_init(&mtx,NULL);              /*Semaphore for closing all critical sections*/
	pthread_cond_init(&red_q,NULL);
	pthread_cond_init(&blue_q,NULL);
	pthread_cond_init(&main_q,NULL);
    pthread_cond_init(&no_car_in_queue,NULL);
	blue_remain=0;                                 /*How much blue cars are over the bridge*/
	blue_waiting=0;                                /*How much blue cars are on the right side*/
	red_waiting=0;                                 /*How much red cars are on the left side*/
	change_counter=0;                              /*To avoid starvation,when "bridge_size"cars pass the bridge,change side*/
	close_flag=0;                                  /*Only one thread wakes up main*/
	blue_empty_flag=0;                             /*Blue thread does not see neither blue waiting cars nor red waiting cars*/
	red_empty_flag=0;                              /*Red thread does not see neither blue waiting cars nor red waiting cars*/
	FILE* fp;
	int i=0,num_cars;
    first=0;
    num_red_blocked=0;
    num_blue_blocked=0;
    blue_blocked_flag=0;
    red_blocked_flag=0;
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
            pthread_mutex_lock(&mtx);
            close_flag=1;
            if(blue_empty_flag==1 || red_empty_flag==1){ /*Create an extra thread for closing*/
                car[i].type=1;
                car[i].iret=pthread_create(&car[i].id, NULL,thread_func,(void *)(&car[i]));
                if(car[i].iret){
                    fprintf(stderr,"Error - pthread_create() return code: %d\n",car[i].iret);
                    exit(EXIT_FAILURE);
                }
            }
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
		pthread_mutex_lock(&mtx);
        printf("Main is reading...\n");
		if(car[i].type==1){blue_waiting++;}
		else if(car[i].type==0){red_waiting++;}
		i++;
		pthread_mutex_unlock(&mtx);
     //   sleep(2);
	}
	printf("Main is closing...\n");
    pthread_cond_wait(&main_q,&mtx);
    free(car);
	return(0);
}
void *thread_func(void *arg){
	struct T *thread_struct=(struct T*)arg;
	int wflag; 
	
	wflag=enter_bridge(thread_struct->type);
	
	if(wflag==2){return(NULL);}
	if(thread_struct->type==1){
		sleep(thread_struct->sleep_time);
		printf(ANSI_COLOR_BLUE"A blue car is passing over the bridge,thread %d\n"ANSI_COLOR_RESET,thread_struct->position);
	}
	else{
		sleep(thread_struct->sleep_time);
		printf(ANSI_COLOR_RED"A red car is passing over the bridge,thread %d\n"ANSI_COLOR_RESET,thread_struct->position);
	}
	
	exit_bridge(thread_struct->type,wflag);
	return(NULL);
}
int enter_bridge(int color){
	int up_flag=0;
       
	if(color==1){                                                /*Blue car*/
		pthread_mutex_lock(&mtx);
        if(close_flag==1 && (red_empty_flag==1 || blue_empty_flag==1)){
            pthread_cond_signal(&main_q);                        /*Wake up main*/
            pthread_mutex_unlock(&mtx);
            return(2);                                           /*Return value for termination*/
        }
        num_blue_blocked++;
        if(blue_blocked_flag==1){
            blue_blocked_flag=0;
            pthread_cond_signal(&no_car_in_queue); 
        }
        if(first==1){pthread_cond_wait(&blue_q,&mtx);}           /*Not first car*/
        else{
            first=1;                                             /*First car*/
            if(blue_empty_flag==1){blue_empty_flag=0;}           /*Reading from main is slow,last car was blue*/
            else if(red_empty_flag==1){                          /*Reading from main is slow,last car was red*/
                red_empty_flag=0;                                /*Change side*/
                change_counter=0;
            }
        }
        num_blue_blocked--;
        blue_remain++;
		blue_waiting--;
		change_counter++;
    }
	else{                                                        /*Red car*/
		pthread_mutex_lock(&mtx);
        num_red_blocked++;
        if(red_blocked_flag==1){
            red_blocked_flag=1;
            pthread_cond_signal(&no_car_in_queue); 
        }
        if(first==1){pthread_cond_wait(&red_q,&mtx);}            /*Not first car*/
        else{
            first=1;                                             /*First car*/
            if(red_empty_flag==1){red_empty_flag=0;}             /*Reading from main is slow,last car was red*/
            else if(blue_empty_flag==1){                         /*Reading from main is slow,last car was blue*/
                blue_empty_flag=0;                               /*Change side*/
                change_counter=0;
            }
        }
        num_red_blocked--;
        red_remain++;
		red_waiting--;
		change_counter++;                
    }
	pthread_mutex_unlock(&mtx);
	return(up_flag);
}
void exit_bridge(int color,int wake_up) {
	pthread_mutex_lock(&mtx);
 	if(color==1){                                  
		blue_remain--;                                           /*A blue car just passed the bridge*/
		if((change_counter==bridge_size && blue_remain==0) || (blue_remain==0 && blue_waiting==0)){
			/*Change side from blue to blue beacuse counter=max or there are not red cars*/
			if(change_counter==bridge_size && red_waiting==0 && blue_waiting!=0){
				change_counter=0;
                if(num_blue_blocked==0){
                    blue_blocked_flag=1;
                    pthread_cond_wait(&no_car_in_queue,&mtx);
                }
				pthread_cond_signal(&blue_q); 
			}
			/*Change side from blue to red because counter=max*/
			else if(change_counter==bridge_size && red_waiting!=0){  
				change_counter=0;
                if(num_red_blocked==0){
                    red_blocked_flag=1;
                    pthread_cond_wait(&no_car_in_queue,&mtx);
                }
				pthread_cond_signal(&red_q); 
			}
			/*Change side from blue to red because all blue cars passed the bridge*/
			else if(change_counter!=bridge_size && blue_remain==0 && blue_waiting==0 && red_waiting!=0){
				change_counter=0;
				if(num_red_blocked==0){
                    red_blocked_flag=1;
                    pthread_cond_wait(&no_car_in_queue,&mtx);
                }
				pthread_cond_signal(&red_q); 
			}
		}
		/*Do not do extra up's.Only one time a car can make an up*/
		else if(change_counter!=bridge_size && blue_waiting!=0){
			if(num_blue_blocked==0){
                blue_blocked_flag=1;
                pthread_cond_wait(&no_car_in_queue,&mtx);
            }
			pthread_cond_signal(&blue_q);
		}
		/*Last car wakes up main and return*/
		if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==1){
			pthread_cond_signal(&main_q);
            pthread_mutex_unlock(&mtx); 
			return;
		}
		if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==0){
            printf("Blue car stacks...\n");
			first=0;
            blue_empty_flag=1;
		}
	}
	else{                                                                   
		red_remain--;                                            /*A red car just passed the bridge*/
		if((change_counter==bridge_size && red_remain==0) || (red_remain==0 && red_waiting==0)){
			/*Change side from red to red beacuse counter=max or there are not blue cars*/
			if(change_counter==bridge_size && blue_waiting==0 && red_waiting!=0){
				change_counter=0;
                if(num_red_blocked==0){
                    red_blocked_flag=1;
                    pthread_cond_wait(&no_car_in_queue,&mtx);
                }
				pthread_cond_signal(&red_q); 
			}
			/*Change side from red to blue because counter=max*/
			else if(change_counter==bridge_size &&blue_waiting!=0){
				change_counter=0;
                if(num_blue_blocked==0){
                    blue_blocked_flag=1;
                    pthread_cond_wait(&no_car_in_queue,&mtx);
                } 
				pthread_cond_signal(&blue_q);
			}
			/*Change side from red to blue because all red cars passed the bridge*/
			else if(change_counter!=bridge_size && red_remain==0 && red_waiting==0 && blue_waiting!=0){
				change_counter=0;
                if(num_blue_blocked==0){
                    blue_blocked_flag=1;
                    pthread_cond_wait(&no_car_in_queue,&mtx);
                }
				pthread_cond_signal(&blue_q); 
			}
		}
		/*Do not do extra up's.Only one time a car can make an up*/
		else if(change_counter!=bridge_size && red_waiting!=0){
			if(num_red_blocked==0){
                red_blocked_flag=1;
                pthread_cond_wait(&no_car_in_queue,&mtx);
            }
			pthread_cond_signal(&red_q);
		}
		if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==1){
			pthread_cond_signal(&main_q);
            pthread_mutex_unlock(&mtx); 
			return;
		}
		if(blue_waiting==0 && red_waiting==0 && blue_remain==0 && red_remain==0 && close_flag==0){
            printf("Red car stacks...\n");
			first=0;
            red_empty_flag=1;
		}
	}
	pthread_mutex_unlock(&mtx);
}
	
long int file_size(FILE *f){
	long int size;
	
	fseek(f, 0L, SEEK_END);
	size=ftell(f);
	fseek(f, 0L, SEEK_SET);
	return(size);
}
