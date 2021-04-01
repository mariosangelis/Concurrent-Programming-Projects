/*Angelis Marios-Kasidakis Theodoros*/
/*Quicksort*/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <sched.h>
#include <fcntl.h>
#define SIZE 15

int quicksort(int *a,int left,int right);
void *thread_func(void *ptr);
int *a;
struct arguments{
    int left;
    int right;
    int flag;
};

int main(int argc,char *argv[]){
    int i;
    a=(int*)malloc(SIZE*sizeof(int));
    for( i=0;i<SIZE;i++){
        printf("Enter value for a[%d]:",i);
        scanf("%d",&a[i]);
    }
    printf("\n");
    printf("======== BEFORE QUICKSORT ============\n");
    for( i=0;i<SIZE;i++){
        printf("a[%d]=%d\n",i,a[i]);
    }
    quicksort(a,0,SIZE-1); // main is the main thread which has the whole table
     
    printf("======== AFTER QUICKSORT ============\n");
    for( i=0;i<SIZE;i++){
        printf("a[%d]=%d\n",i,a[i]);
    }
    return (0);
}

void *thread_func(void *ptr){
    quicksort(a,((struct arguments *)ptr)->left,((struct arguments *)ptr)->right);
    ((struct arguments *)ptr)->flag=1;
    return NULL;    
}
int quicksort(int *a,int left,int right){
    
    int i=left-1;
    int j=right;
    int temp;
    int o=a[right];
    struct arguments args1;
    struct arguments args2;
    args2.flag=1;
    args1.flag=1;
    while(1){
        while(a[++i]<o){}
        while(o<a[--j]){
            if(j==left){
                break;
            }
        }
        if(i>=j){
            break;
        }
        temp=a[i];
        a[i]=a[j];
        a[j]=temp;
    }
    temp=a[i];
    a[i]=a[right];
    a[right]=temp;
    
    if(left<i-1){                                                 /*Left recursion step*/                                     
        pthread_t t1;
        int iret1;
        args1.left=left;
        args1.right=i-1;
        args1.flag=0;
        iret1=pthread_create(&t1,NULL,thread_func,(void*)(&args1));
        if(iret1){
            printf("Error with thread creation(%d).\n",iret1);
        }
    }
    if(i+1<right){                                                /*Right recursion step*/  
        pthread_t t2;
        int iret2;
        args2.left=i+1;
        args2.right=right;
        args2.flag=0;
        iret2=pthread_create(&t2,NULL,thread_func,(void*)(&args2));
        if(iret2){
            printf("Error with thread creation(%d).\n",iret2);
        }
    }
    while(args1.flag==0 || args2.flag==0){}                       /*Wait left and right thread to return*/
    return(1);
}
