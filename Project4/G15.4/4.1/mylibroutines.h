typedef struct coroutine {
	ucontext_t source_context;
	ucontext_t destination_context;
	char mystack[SIGSTKSZ];
}co_t;

ucontext_t closing_link;
int mycoroutines_create(co_t *mycot,void (*myfunction)(), co_t *mycot1,ucontext_t *tocontext,ucontext_t *main_context);
int mycoroutines_destroy(co_t *mycot);
int mycoroutines_switchto( co_t *mycot);

int mycoroutines_create(co_t *mycot,void (*myfunction)(),co_t *mycot1,ucontext_t *tocontext,ucontext_t *main_context) {

    printf("Inside mycoroutines_create()\n");
    mycot->source_context.uc_link = &closing_link;                   /*link for closing */
	mycot->source_context.uc_stack.ss_sp = mycot->mystack;           /*the stack for this context */
	mycot->source_context.uc_stack.ss_size = sizeof(mycot->mystack); /* the stack size */
	getcontext(&(mycot->source_context));
	makecontext(&(mycot->source_context),(void(*)(void))myfunction,3,mycot,tocontext,main_context);
	return 0;
}
int mycoroutines_destroy(co_t *mycot){
    getcontext(&(closing_link));
    if(flag_write==1 && flag_read==2){                          /*Both customer and producer have been destroyed,return*/
        printf("Main is up...\n");
        return 0;
    }
    else if(flag_read==1){                                      /*Customer has been destroyed,destroy producer's context*/
        flag_read=2;
        return 0;
    }
    mycoroutines_switchto(mycot);
    return 0;
}
int mycoroutines_switchto(co_t *mycot){
	swapcontext(&(mycot->source_context),&(mycot->destination_context));
	return 0;
}
