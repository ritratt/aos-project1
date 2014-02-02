#include "template.h"
#include <sys/time.h>
#include <ucontext.h>

#define MEM 64000
#define RUNNING 1
#define READY 2

typedef struct thread_t{
	int tid;
	struct	thread_t *next;
	ucontext_t *context;
	int state;
}gtthread_t;

struct queue {
	gtthread_t *head;
	gtthread_t *tail;
};

//Declare a global task queue.
struct queue *task_queue;

//Some global variables
int thread_count = 1;

void fun_alarm_handler(int sig) {
	printf("Thread quantum expired.\n");
	gtthread_t *current_thread = task_queue->head;
	printf("Current running thread is %d\n", current_thread->tid);
	printf("Current tail is %d\n", task_queue->tail->tid);
	if(current_thread->next != NULL) {
		task_queue->head = current_thread->next;
		task_queue->tail->next = current_thread;
	}
	else {
		printf("Only one thread in queue. Returning.\n");
		return;
	}
	printf("Thread %d moved to head.\n", task_queue->head->tid);
	gtthread_t *tail_thread = task_queue->tail;
	task_queue->tail = current_thread;

	//Swap context
	ucontext_t *current_context = current_thread->context;
	gtthread_t *current_head = task_queue->head;
	ucontext_t *next_context = current_head->context;
	swapcontext(current_context, next_context);
}

void add_to_queue(gtthread_t *new_tail) {
	gtthread_t *current_tail = task_queue->tail;
	printf("Current tail tid is %d\n", task_queue->tail->tid);
	current_tail->next = new_tail;
	new_tail->next = NULL;
	task_queue->tail = new_tail;
}

int gtthread_init(long period) {
	task_queue = (struct queue *) malloc(sizeof(struct queue));
	task_queue->head = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	task_queue->tail= (struct gtthread_t *) malloc(sizeof(gtthread_t));
	
	//Create dummy thread for main and make it as queue head.
	gtthread_t *main_thread = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	ucontext_t *main_context;
	getcontext(main_context);
	main_thread->tid = 1;
	main_thread->state = RUNNING;
	main_thread->context = main_context;
	task_queue->head = main_thread;
	task_queue->tail= main_thread;
	printf("As per init, current tail tid is %d\n", task_queue->tail->tid);

	//Initialize timer struct
	struct itimerval timer;
	timer.it_interval.tv_sec = period;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = period;
	timer.it_value.tv_usec = 0;

	//Set the timer
	setitimer(ITIMER_REAL, &timer, NULL);


	/*Test timer values
        struct itimerval timerval;
	getitimer(ITIMER_VIRTUAL, &timerval);
	printf("New Timer val is %d\n", timerval.it_interval.tv_sec);*/
	
	//Register signal handler
	struct sigaction alarm_handler;
	alarm_handler.sa_handler = fun_alarm_handler;
	sigaction(SIGALRM, &alarm_handler, NULL);
	
	//task_queue->head
	
}

int gtthread_create(gtthread_t *thread, void *(*fn) (void *), void *args) {
	
	//Create new context and swap with current context
	ucontext_t new_context;
	ucontext_t old_context;
	old_context = *(task_queue->head->context); //fix pointer
	getcontext(&new_context);
	getcontext(&old_context);
	new_context.uc_link=0;
	new_context.uc_stack.ss_sp=malloc(MEM);
	new_context.uc_stack.ss_size=MEM;
	new_context.uc_stack.ss_flags=0;
	makecontext(&new_context, (void *) fn, 0);

	//Must not swap on creation. Line below incorrect.
	//swapcontext(&old_context, &new_context);
	
	//Increment thread count
	thread_count++;
	thread->context = &new_context;
	thread->tid = thread_count;
	thread->state = RUNNING;
	thread->next = NULL;

	//Put thread at the end of the queue
	add_to_queue(thread);

	//reset timer

	return 1;
}

void crapfn() {
	printf("juz sum crap\n");
	while(1);
}

int main() {
	gtthread_init(3);
	ucontext_t try;
	getcontext(&try);
	gtthread_t *t1 = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	//gtthread_t *t2 = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	//gtthread_t *t3 = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	gtthread_create(t1, (void *) crapfn, NULL);
	//gtthread_create(t2, (void *) morecrapfn, NULL);
	//gtthread_create(t3, (void *) crapdontstopfn, NULL);
	while(1);
	printf("AAAnd we're back\n");
	return 1;
}


