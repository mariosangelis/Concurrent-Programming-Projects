typedef struct node {                                       /*Thread struct*/
	int position;
	struct node *next;
	struct node *prev;
	ucontext_t source_context;
	char mystack[SIGSTKSZ];
}thr_t;

typedef struct sem_n{                              /*Semaphore node struct*/
    int exit_flag;
    int value;
    struct sem_n *next;
    struct sem_n *prev;
}sem_node;

typedef struct semaphore{                                   /*Semaphore struct*/
	int sem_counter;
	int flag;
    int num_of_blocked_threads;
    sem_node *head; /* pointer to the fifo queue of the semaphore.Threads are blocked in this queue. */
}sem;

thr_t *head;
thr_t *current_node;
int position;
sigset_t s1;

void threads_list_init();
thr_t *insert_thread_list();
int mythreads_create(thr_t *t,void (*myfunction)(),void *args);
int mythreads_destroy(thr_t *mythr);
int mythreads_yield();
int mythreads_sem_init(sem *mysem,int init_value);
int mythreads_sem_up(sem *mysem);
int mythreads_sem_down(sem *mysem);
int mythreads_sem_destroy(sem *mysem);
sem_node * insert_semsqueue(sem *mysem);
void exit_semsqueue(sem *mysem);

void threads_list_init() {
	head=(thr_t*)malloc(sizeof(thr_t));
	head->source_context.uc_stack.ss_sp = head->mystack;           /*the stack for this context */
	head->source_context.uc_stack.ss_size = sizeof(head->mystack); /* the stack size */
	getcontext(&(head->source_context));
	head->next=head;
	head->prev=head;
}
/*Insert a thread at the end of the threads list*/
thr_t *insert_thread_list(){
	thr_t *new_node=(thr_t*)malloc(sizeof(thr_t));
	thr_t *curr;
	if(new_node==NULL) {
		printf("Problem with memory allocation.\n");
		exit(EXIT_FAILURE);
	}
	for(curr=head->next;;curr=curr->next) {
		if(curr->next==head) {
			break;
		}
	}
	new_node->position=position;
	position++;
	new_node->next=curr->next;
	new_node->prev=curr;
	new_node->next->prev=new_node;
	new_node->prev->next=new_node;
	return(new_node);
}
/*Create function*/
int mythreads_create(thr_t *t,void (*myfunction)(),void *args) {
	printf("Inside create\n");
	t=insert_thread_list();
	t->source_context.uc_stack.ss_sp = t->mystack;           /*the stack for this context */
	t->source_context.uc_stack.ss_size = sizeof(t->mystack); /* the stack size */
	getcontext(&(t->source_context));
	makecontext(&(t->source_context),(void(*)(void))myfunction,2,t,args);
	return 0;
}
/*Delete the first element from the thread's list*/
int mythreads_destroy(thr_t *mythr){
	thr_t *curr=head->next;
	curr->next->prev=head;
	curr->prev->next=curr->next;
	free(curr);
	return(0);
}
/*Swap from current_node to current_node->next*/
int mythreads_yield(){
	thr_t *temp_node;
	if(current_node->next==head){
		temp_node=current_node;
		current_node=head->next;
		printf("thread %d -> thread %d\n",temp_node->position,temp_node->next->next->position);
		swapcontext(&(temp_node->source_context),&(temp_node->next->next->source_context));
	}
	else{
		temp_node=current_node;
		current_node=current_node->next;
		printf("thread %d -> thread %d\n",temp_node->position,temp_node->next->position);
		swapcontext(&(temp_node->source_context),&(temp_node->next->source_context));
	}
	return (0);
}
/*Sem_init*/
int mythreads_sem_init(sem *mysem,int init_value){
	/*Initialize semaphore's elements*/
	mysem->head=(sem_node *)malloc(sizeof(sem_node));
	mysem->head->next=mysem->head;
	mysem->head->prev=mysem->head;
	mysem->sem_counter=init_value;
	mysem->num_of_blocked_threads=0;
	return(0);
}
/*Sem_down function*/
int mythreads_sem_down(sem *mysem){
    /*Decrease semaphore's counter*/
	sigprocmask(SIG_BLOCK,&s1,NULL);
	int first_spin=0;
	if(mysem->sem_counter >= 0){
		mysem->sem_counter--;
	}
	/*Block at the end of semaphore's queue*/
	if(mysem->sem_counter==-1){
		mysem->num_of_blocked_threads++;
		sem_node *new_node;
		new_node=insert_semsqueue(mysem);
		while(new_node->exit_flag==0){
			if(first_spin==0) {
				first_spin=1;
				sigprocmask(SIG_UNBLOCK,&s1,NULL);
			}
		}
		sigprocmask(SIG_BLOCK,&s1,NULL);
		exit_semsqueue(mysem);
		sigprocmask(SIG_UNBLOCK,&s1,NULL);
	}
	return(0);
}
/*Sem_up function*/
int mythreads_sem_up(sem *mysem){
	sigprocmask(SIG_BLOCK,&s1,NULL);
	/*Increase semaphore's counter when counteris up to 0 or if semaphores queue is empty*/
	if((mysem->sem_counter>0) || (mysem->sem_counter==0 && mysem->num_of_blocked_threads==0)){
		mysem->sem_counter++;
	}
	else if(mysem->sem_counter<=0){
		if(mysem->sem_counter==-1){
			mysem->sem_counter++;
		}
		mysem->num_of_blocked_threads--;
		mysem->head->next->exit_flag=1;    /*Wake up the first element of semaphores queue*/
	}
	sigprocmask(SIG_UNBLOCK,&s1,NULL);
	return(0);
}
/*Insert element inside sempahore's list*/
sem_node *insert_semsqueue(sem *mysem){
	sem_node *new_node=(sem_node*)malloc(sizeof(sem_node));
	sem_node *curr;
	if(new_node==NULL) {
		printf("Problem with memory allocation.\n");
		exit(EXIT_FAILURE);
	}
	/*Insert the element at the end of the queue*/
	for(curr=mysem->head->next;;curr=curr->next) {
		if(curr->next==mysem->head) {
			break;
		}
	}
	new_node->exit_flag=0;
	new_node->next=curr->next;
	new_node->prev=curr;
	new_node->next->prev=new_node;
	new_node->prev->next=new_node;
	return(new_node);
}
/*Delete first element from semaphores queue*/
void exit_semsqueue(sem *mysem){
	sem_node *curr=mysem->head->next;
	curr->next->prev=mysem->head;
	curr->prev->next=curr->next;
	free(curr);
}
/*Destroy an empty semaphores queue*/
int mythreads_sem_destroy(sem *mysem){
	free(mysem->head);
	return(0);
}
