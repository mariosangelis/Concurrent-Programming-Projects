#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <sched.h>
#include <fcntl.h>
#include "pipe.h"
#include "mylibroutines.h"
#include "compare_library.h"

void producer(co_t *mycot,ucontext_t *tocontext,ucontext_t *main_context);
void consumer(co_t *mycot,ucontext_t *tocontext,ucontext_t *main_context);
int mycoroutines_init(co_t *mycot);

int main(int args,char *argv[]) {

    FILE *f1,*f2; /*File pointers used for comparing file1 and file2 (diff)*/

    if(args<2){
		fprintf(stderr,"Error - Not enough arguments\n");
		exit(EXIT_FAILURE);
	}
    f1=NULL;
    f2=NULL;
	size=atoi(argv[1]); /*size of the pipe given as an argument*/
	pipe_init(size);
	co_t main_t;

	mycoroutines_init(&main_t);
	pipe_close();
    diff(f1,f2);

	return(0);
}

int mycoroutines_init(co_t *mycot){
	 co_t producer_cot;
	 co_t consumer_cot;

    /*Create 2 contexts,one for producer and one for consumer,initialize their stacks*/
    mycoroutines_create(&producer_cot,(void(*)(void))producer,&producer_cot,&(consumer_cot.source_context),&(mycot->source_context));
	mycoroutines_create(&consumer_cot,(void(*)(void))consumer,&consumer_cot,&(producer_cot.source_context),&(mycot->source_context));

    
	printf("Switch to producer context\n");
	mycot->destination_context=producer_cot.source_context;
	mycoroutines_switchto(mycot);

    mycot->destination_context=consumer_cot.source_context;     /*Customer has finished,destroy his context*/
    mycoroutines_destroy(mycot);
    mycot->destination_context=producer_cot.source_context;     /*Producer has finished,destroy his context*/
    mycoroutines_destroy(mycot);

	return 0;
}

void producer(co_t *mycot,ucontext_t *tocontext,ucontext_t *main_context){
	int fd;
	size_t nread;
	fd = open("file1.jpg",O_RDWR,S_IWUSR);
	if (fd<0) {
		fprintf(stderr,"Error at Opening file\n");
		exit(EXIT_FAILURE);
	}
	while(1){
		char byte;
        printf("writing...\n");
		nread = read(fd,&byte,1);
		if(nread<0){
			fprintf(stderr,"Error at reading from file\n");
			exit(EXIT_FAILURE);
		}
		if(nread==0){                                                /*Eof,write is clossing*/
            close_flag=1;
			printf("End of file\n");
			mycot->destination_context=*tocontext;
			mycoroutines_switchto(mycot);                            /*Switch to consumer*/
            flag_write=1;                                            /*Main is destroying producer*/
            printf("Exit from producer\n");
            break;
		}
		pipe_write(byte);                                            /*Producer is writing inside buffer*/
		index_write++;
		if(index_write==size){                                       /*Buffer is full,switch to consumer*/
			index_write=0;
			mycot->destination_context=*tocontext;
			mycoroutines_switchto(mycot);
		}
	}
	setcontext(mycot->source_context.uc_link);                       /*Producer has been destroyed,switch to main*/
}
void consumer(co_t *mycot,ucontext_t *tocontext,ucontext_t *main_context){
	int fd,nwrite;
	fd = open("file2.jpg", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		printf("Couldn't create new file \n");
		return ;
	}
	while(1){
        if(flag_read==1){                                            /*Consumer has been destroyed,switch to main*/
            setcontext(mycot->source_context.uc_link);
            break;
        }
        printf("reading...\n");
		if(close_flag==1){                                           /*Producer stopped writing to buffer*/
			while(1){                                                /*Read all the remaining indexes*/
				if(index_read==index_write){                         /*Buffer is empty,producer haw finished,switch to main*/
					mycot->destination_context=*main_context;
                    mycoroutines_switchto(mycot);
                    flag_read=1;
					printf("Exit from consumer\n");
                    break;
				}
				char read_byte;
				read_byte=buffer[index_read];
				nwrite=write(fd,&read_byte,1);
				if(nwrite<0){
					fprintf(stderr,"Error at writing to file\n");
					exit(EXIT_FAILURE);
				}
				index_read++;
			}
		}
		else{
			char read_byte;                                          /*Consumer is reading*/
			pipe_read(&read_byte);
			nwrite=write(fd,&read_byte,1);
			if(nwrite<0){
				fprintf(stderr,"Error at writing to file\n");
				exit(EXIT_FAILURE);
			}
			index_read++;
			if(index_read==size){                                    /*Buffer is empty,switch to producer*/
				index_read=0;
				mycot->destination_context=*tocontext;
				mycoroutines_switchto(mycot);
			}
		}
	}
}
