typedef struct node {                                       /*Thread struct*/
	int position;
	struct node *next;
	struct node *prev;
	int exit_flag;
	ucontext_t source_context;
	char mystack[SIGSTKSZ];
	int spin_sem;
}thr_t;

typedef struct semaphore{                                   /*Semaphore struct*/
	int sem_counter;
	int num_of_blocked_threads;
	thr_t *head;
}sem;
struct itimerval t;
thr_t *ready_queue_head;
thr_t *destroy_head;
thr_t *current_node;
thr_t *ready_node_next;
sem thread_sem,main_sem,mtx;
ucontext_t main_context;
ucontext_t closing_link;
sigset_t s1;
int counter,size,position,close_master=0,destroy_flag;
thr_t *threads_array;

thr_t *list_init(thr_t *head);
thr_t *insert_thread_list();
void insert_semaphores_queue(sem *mysem);
void insert_ready_queue(sem *mysem);
int mythreads_join();
int mythreads_join_reply();
void transfer_node();
int mythreads_create(thr_t *t,void (*myfunction)(),void *args);
int mythreads_destroy(thr_t *mythr);
int mythreads_sem_init(sem *mysem,int init_value);
int mythreads_sem_up(sem *mysem);
int mythreads_sem_down(sem *mysem);
int mythreads_sem_destroy(sem *mysem);
int mythreads_yield();

int mythreads_create(thr_t *thr,void (*myfunction)(),void *args) {

	printf("Inside create\n");
	thr=insert_thread_list();

	thr->source_context.uc_link = &(closing_link);
	thr->source_context.uc_stack.ss_sp = thr->mystack;           /*the stack for this context */
	thr->source_context.uc_stack.ss_size = sizeof(thr->mystack); /* the stack size */
	thr->exit_flag=0;
	thr->spin_sem=0;
	getcontext(&(thr->source_context));
	makecontext(&(thr->source_context),(void(*)(void))myfunction,2,thr,args);
	return (0);
}
/*Initialize list,head is the first empty node of the list*/
thr_t *list_init(thr_t *head) {
	head=(thr_t*)malloc(sizeof(thr_t));
	head->next=head;
	head->prev=head;
	return(head);
}
/*Insert a thread at the end of the threads list*/
thr_t *insert_thread_list(){

	thr_t *new_node=(thr_t*)malloc(sizeof(thr_t));
	thr_t *curr;
	if(new_node==NULL) {
		printf("Problem with memory allocation.\n");
		exit(EXIT_FAILURE);
	}
	for(curr=ready_queue_head->next;;curr=curr->next) {
		if(curr->next==ready_queue_head) {
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
/*Delete the first element from the thread's list*/
int mythreads_destroy(thr_t *mythr){

	thr_t *curr=destroy_head->next;
	curr->next->prev=destroy_head;
	curr->prev->next=curr->next;
	free(curr);
	return(0);
}
/*Swap from current_node to the next ready context*/
int mythreads_yield(){
	thr_t *temp_node;
	temp_node=current_node;
	if(current_node==ready_node_next){
		/*ready_node_next is not already changed from sem_down function(current_node==ready_node_next)*/
		if(ready_node_next->next==ready_queue_head){
			ready_node_next=ready_queue_head->next;
			current_node=ready_node_next;
		}
		else{
			ready_node_next=current_node->next;
			current_node=ready_node_next;
		}
		printf("thread %d -> thread %d\n",temp_node->position,ready_node_next->position);
		/*Swap to the next ready context*/
		swapcontext(&(temp_node->source_context),&(ready_node_next->source_context));
	}
	else{
		/*ready_node_next already changed from sem_down function*/
		if(ready_node_next==ready_queue_head){
			ready_node_next=ready_queue_head->next;
		}
		printf("thread %d -> thread %d\n",temp_node->position,ready_node_next->position);
		current_node=ready_node_next;
		/*Swap to the next ready context*/
		swapcontext(&(temp_node->source_context),&(ready_node_next->source_context));
	}
	return (0);
}
/*Sem_init*/
int mythreads_sem_init(sem *mysem,int init_value){
	/*Initialize semaphore's elements*/
	mysem->head=(thr_t *)malloc(sizeof(thr_t));
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
		/*ready_node_next is set to the next element of the empty queue*/
		ready_node_next=current_node->next;
		/*Transfer current_node element from ready queue to semaphores queue*/
		insert_semaphores_queue(mysem);
		while(current_node->spin_sem==0){
			if(first_spin==0) {
				first_spin=1;
				sigprocmask(SIG_UNBLOCK,&s1,NULL);
			}
		}
		current_node->spin_sem=0;
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
		/*Transfer current_node element from semaphores queue to ready queue*/
		insert_ready_queue(mysem);
		//printf("Sem up\n");
	}
	sigprocmask(SIG_UNBLOCK,&s1,NULL);
	return(0);
}
/*Transfer current_node element from ready queue to semaphores queue*/
void insert_semaphores_queue(sem *mysem){
	thr_t *curr;
	current_node->prev->next=current_node->next;
	current_node->next->prev=current_node->prev;
	for(curr=mysem->head->next;;curr=curr->next) {
		if(curr->next==mysem->head) {
			break;
		}
	}
	current_node->next=curr->next;
	current_node->prev=curr;
	current_node->next->prev=current_node;
	current_node->prev->next=current_node;
}
/*Transfer current_node element from semaphores queue to ready queue*/
void insert_ready_queue(sem *mysem){
	thr_t *curr=mysem->head->next;
	curr->next->prev=mysem->head;
	curr->prev->next=curr->next;
	thr_t * last_node;
	for(last_node=ready_queue_head->next;;last_node=last_node->next) {
		if(last_node->next==ready_queue_head) {
			break;
		}
	}
	curr->next=last_node->next;
	curr->prev=last_node;
	curr->next->prev=curr;
	curr->prev->next=curr;
	curr->spin_sem=1;
}
void transfer_node(){
	thr_t *curr;
	current_node->prev->next=current_node->next;
	current_node->next->prev=current_node->prev;
	for(curr=destroy_head->next;;curr=curr->next) {
		if(curr->next==destroy_head) {
			break;
		}
	}
	current_node->next=curr->next;
	current_node->prev=curr;
	current_node->next->prev=current_node;
	current_node->prev->next=current_node;
}
int mythreads_join(){
	int flag=0;
	ready_queue_head->prev->exit_flag=1;
	//printf("Join starts\n");
	getcontext(&(closing_link));
	//printf("Join returns\n");
	if(flag==1){
		return counter;
	}
	flag=1;
	mythreads_yield();
	return 0;
}
int mythreads_join_reply(){
    if(current_node->exit_flag==1){
        //printf("Thread %d is going to destroy list\n",current_node->position);
        ready_node_next=current_node->next;
        transfer_node();
        counter++;
        setcontext(current_node->source_context.uc_link);
    }
    else{mythreads_yield();}
    return 0;
}
/*Destroy an empty semaphores queue*/
int mythreads_sem_destroy(sem *mysem){
	free(mysem->head);
	return(0);
}
