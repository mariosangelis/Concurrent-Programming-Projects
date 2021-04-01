/*Angelis Marios-Kasidakis Theodoros*/
/*Fifo pipe*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <sched.h>
#include <fcntl.h>
#include "pipe.h"

void *thread_read(void *ptr);
void *thread_write(void *ptr);
int main(int args,char* argv[]){
    pthread_t thread1, thread2;
    int  iret1,iret2;
    if(args<2){
        fprintf(stderr,"Error - Not enough arguments\n");
        exit(EXIT_FAILURE);
    }
    size=atoi(argv[1]);
    pipe_init(size);
    iret1 = pthread_create( &thread1, NULL,thread_read,NULL);                                  /* Create first thread*/
    if(iret1)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
        exit(EXIT_FAILURE);
    }
    iret2 = pthread_create( &thread2, NULL,thread_write,NULL);                                 /*Create second thread*/
    if(iret2)
    {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret2);
        exit(EXIT_FAILURE);
    }
    while(flag_read==1 || flag_write==1){}                                                   /* Wait for threads to terminate*/
    printf("index_write=%d\n",index_write);
    printf("index_read=%d\n",index_read);
    pipe_close(); 
    return 0;
}
void *thread_read(void *ptr)
{
    int fd,nwrite;
    fd = open("image_after_test.pdf", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        printf("Couldn't create new file \n");
        return NULL;
    }
     while(index_write==0){}                                /*Read is waiting while buffer is empty*/
     while(close_flag==0){
        if(index_read==(index_write-1)){                    /*Do not overread,wait second thread to write in the buffer*/
            sched_yield();
        }
        else{
            char read_string;
            pipe_read(&read_string);
            nwrite=write(fd,&read_string,1);
            if(nwrite<0){
                fprintf(stderr,"Error at writing to file\n");
                exit(EXIT_FAILURE);
            }
            index_read++;
            if(index_read==size){                            /*Buffer is circular*/
                while(index_write==0){}                      /*Read is waiting while buffer is empty*/
                index_read=0;
            }
        }
     }
     if(close_flag==1){                                      /*Second thread stopped writing to buffer*/
        while(index_read!=index_write){                      /*Read all the remaining indexes*/
            char read_string;
            read_string=buffer[index_read];
            nwrite=write(fd,&read_string,1);
            if(nwrite<0){
                fprintf(stderr,"Error at writing to file\n");
                exit(EXIT_FAILURE);
            }
            index_read++;
            if(index_read==size){
                index_read=0;
            }
        }
    }
    close(fd);
    flag_read=0;
    return (NULL);
}
void *thread_write( void *ptr )
{
    int fd;
    size_t nread;
    fd = open("test.pdf",O_RDWR,S_IWUSR);
    if (fd<0) {
        fprintf(stderr,"Error at Opening file\n");
        exit(EXIT_FAILURE);
    }
    while(1){
        if(index_write==(index_read-1)){                      /*Do not overwrite,wait second thread to read from buffer*/
            sched_yield();
        }
        else{
            char string;
            nread = read(fd,&string,1);
            if(nread<0){
                fprintf(stderr,"Error at reading from file\n");
                exit(EXIT_FAILURE);
            }
            if(nread==0){                                     /*Eof,write is clossing*/
                close_flag=1;
                close(fd);
                break;
            }
            pipe_write(string);
            index_write++;
            if(index_write==size){                            /*Buffer is circular*/
                while(index_read==0){}                        /*Write is waiting while buffer is full*/
                index_write=0;
            }
        }
    }
    flag_write=0;
    return (NULL);
}
