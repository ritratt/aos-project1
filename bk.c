#include "template.h"
#include <sys/time.h>
#include <ucontext.h>

#define MEM 64000
#define RUNNING 1
#define READY 2
#define FINISHED 3
#define JOINING 4

typedef struct thread_t{
	int tid;
	struct	thread_t *next;
	ucontext_t context;
	int state;
	int joiner_count;
	void *ret;
	int joinee_tid;
}gtthread_t;

struct queue {
	gtthread_t *head;
	gtthread_t *tail;
};

//Declare a global task queue.
struct queue *task_queue;

//Some global variables
struct sigaction alarm_handler;
int thread_count = 1;
struct itimerval timer;
ucontext_t main_context;

void wrapper(void *(*fn) (void *), void *args) {
	void *retval = fn(args);
	task_queue->head->ret = retval;
	puts("Exit gonna be called from wrapper");
	gtthread_exit(retval);
}
	
gtthread_t* get_thread(int thread_id) {
	gtthread_t *t = task_queue->head;
	
	while(t->tid != thread_id) {
		t = t->next;
	}
	if(t != NULL) {
		return t;
	}
	else {
		printf("Thread not found. Returning NULL.\n");
	}
	return NULL;
}

gtthread_t* get_joinfree() {
	gtthread_t *t; 
	while(t != NULL) {
		t = task_queue->head->next;
		if(t->state == READY) 
			return t;
		else
			headtotail();
	}
	return t;
}

void fun_alarm_handler(int sig) {
	printf("Thread quantum expired.\n");
	gtthread_t *current_thread = task_queue->head;
	printf("Current running thread is %d\n", current_thread->tid);
	if(current_thread->next != NULL) {
		task_queue->head = get_joinfree();
		//task_queue->head = task_queue->head->next;
		task_queue->tail->next = current_thread;
	}
	else {
		printf("Only one thread in queue. Returning.\n");
		return;
	}
	printf("Thread %d moved to head.\n", task_queue->head->tid);
	if(task_queue->head->tid == 1)
		sleep(1);
	gtthread_t *tail_thread = task_queue->tail;
	task_queue->tail = current_thread;

	//Swap context
	ucontext_t current_context = current_thread->context;
	gtthread_t *current_head = task_queue->head;
	ucontext_t next_context = current_head->context;
	if(&current_context == NULL) {
		printf("Next tid is %d\n", current_head->tid);
		printf("Current context is null.\n");
	}
	swapcontext(&(task_queue->tail->context), &(task_queue->head->context));
	return;
}

void add_to_queue(gtthread_t *new_tail) {
	gtthread_t *current_tail = task_queue->tail;
	printf("Current tail tid is %d\n", task_queue->tail->tid);
	current_tail->next = new_tail;
	new_tail->next = NULL;
	task_queue->tail = new_tail;
}

void headtotail(){
	gtthread_t *current_head = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	if (task_queue->head->next == NULL) {
		printf("No thread to yield to.\n");
		return;
	}
	current_head = task_queue->head;
	if(current_head->state != JOINING)
		current_head->state = READY;
	task_queue->head = task_queue->head->next;
	task_queue->tail->next = current_head;
	task_queue->tail = current_head;
	task_queue->tail->next = NULL;
}
int gtthread_init(long period) {
	task_queue = (struct queue *) malloc(sizeof(struct queue));
	task_queue->head = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	task_queue->tail= (struct gtthread_t *) malloc(sizeof(gtthread_t));
	
	//Create dummy thread for main and make it as queue head.
	gtthread_t *main_thread = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	getcontext(&main_context);
	main_thread->tid = 1;
	main_thread->state = RUNNING;
	main_thread->context = main_context;
	main_thread->joiner_count = 0;
	main_thread->ret = NULL;
	main_thread->joinee_tid = 0;
	task_queue->head = main_thread;
	task_queue->tail= main_thread;
	printf("As per init, current tail tid is %d\n", task_queue->tail->tid);

	//Initialize timer struct
	timer.it_interval.tv_sec = period;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = period;
	timer.it_value.tv_usec = 0;


	/*Test timer values
        struct itimerval timerval;
	getitimer(ITIMER_VIRTUAL, &timerval);
	printf("New Timer val is %d\n", timerval.it_interval.tv_sec);*/
	
	//Register signal handler
	alarm_handler.sa_handler = fun_alarm_handler;
	sigaction(SIGALRM, &alarm_handler, NULL);

	//Set the timer
	setitimer(ITIMER_REAL, &timer, NULL);

	
//task_queue->head
	
}

int gtthread_create(gtthread_t *thread, void *(*fn) (void *), void *args) {
	
	//Create new context and swap with current context
	ucontext_t new_context;
	ucontext_t old_context;
	old_context = task_queue->head->context; //fix pointer
	getcontext(&new_context);
	getcontext(&old_context);
	new_context.uc_link=0;
	new_context.uc_stack.ss_sp=malloc(MEM);
	new_context.uc_stack.ss_size=MEM;
	new_context.uc_stack.ss_flags=0;
	//makecontext(&new_context, fn, 0);

	//Point makecontext to the wrapper function.
	makecontext(&new_context, (void (*)(void))wrapper, 2,(void (*)(void)) fn, args);

	//Increment thread count
	thread_count++;
	thread->context = new_context;
	thread->tid = thread_count;
	thread->state = READY;
	thread->next = NULL;
	thread->joiner_count = 0;
	thread->ret = NULL;
	thread->joinee_tid = 0;

	//Put thread at the end of the queue
	add_to_queue(thread);

	return 1;
}

int gtthread_join(gtthread_t joiner_thread, void **status) {
	
	//Mark the head as JOINING so that it waits
	task_queue->head->state = JOINING;

	//Increment joiners for current thread where join is called
	task_queue->head->joiner_count = task_queue->head->joiner_count + 1;
	
	//Update joiner tid
	gtthread_t *temp = task_queue->head;
	//substitute this searhc with function call.
	while(temp->tid != joiner_thread.tid)
		temp = temp->next;
	if(temp == NULL) {
		printf("No thread found.\n");
		return 2;
	}
	temp->joinee_tid = task_queue->head->tid;
	
	//Store status in joiner_thread
	
	//Reschedule to change executing thread
	fun_alarm_handler(1);
	
	//Put retval in status
	puts("Back in join");

	if(status != NULL) {
		*status = malloc(sizeof(int));
		*status = temp->ret;
	}
	//status = joiner.ret;
	//return status;

	return 0;
}

int gtthread_yield() {
	headtotail();
}
	
int gtthread_self() {
	return task_queue->head->tid;
}

int gtthread_exit(void *retval) {

	gtthread_t *current_head = task_queue->head;
	gtthread_t *new_head = task_queue->head->next; //get_joinfree();
	
	//Assign retval to the executing head before it exits
	task_queue->head->ret = retval;
	printf("Return value from exit is %d\n", (int) current_head->ret);

	//Get current head and mark it has FINISHED.
	current_head = task_queue->head;
	current_head->state = FINISHED;

	//Decrement joinee count for the thread which this thread wants to join
	gtthread_t *joinee_thread;// = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	int joinee_tid = current_head->joinee_tid;
	joinee_thread = get_thread(joinee_tid);
	joinee_thread->joiner_count = joinee_thread->joiner_count - 1;
	joinee_thread->state = READY;

	//New head is the head's next thread.
	task_queue->head = new_head;

	//Swap the new head in.
	swapcontext(&(current_head->context), &(new_head->context));
	return 0;
}
	
void crapfn() {
	int id = gtthread_self();
	printf("juz sum crap from %d\n", id);
	while(1);
}

void *morecrapfn() {
	printf("more crap\n");
	printf("Exiting peacefully.\n");
	return (void*)666;
}

int main() {
	gtthread_init(3);
	gtthread_t t1, t2;

	void *thread_return = NULL;
	//gtthread_t *t3 = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	gtthread_create(&t1, (void *) crapfn, NULL);
	gtthread_create(&t2, (void *) morecrapfn, NULL);
	gtthread_join(t2, &thread_return);
	printf("Thread return %d\n", *(long*)&thread_return);
	//gtthread_create(t3, (void *) crapdontstopfn, NULL);
	while(1);
	printf("AAAnd we're back\n");
	return 1;
}


