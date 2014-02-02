#include "template.h"
#include <sys/time.h>
#include <ucontext.h>

#define MEM 64000
#define RUNNING 1
#define READY 1

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
	task_queue->head = current_thread->next;
	gtthread_t *tail_thread = task_queue->tail;
	task_queue->tail = current_thread;

	//Swap context
	//ucontext_t *current_context = getcontext 
}

int gtthread_init(long period) {
	//Create dummy thread for main and make it as queue head.
	gtthread_t main_thread;
	ucontext_t main_context;
	getcontext(&main_context);
	main_thread.tid = 1;
	main_thread.state = RUNNING;
	main_thread.context = main_context;
	task_queue->head = &main_thread;

	//Initialize timer struct
	struct itimerval timer;
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = period;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 0;

	//Set the timer
	setitimer(ITIMER_VIRTUAL, &timer, NULL);


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
	swapcontext(&old_context, &new_context);
	
	//Increment thread count
	thread_count++;
	thread->context = &new_context;
	thread->tid = thread_count;
	thread->state = RUNNING;
	thread->next = task_queue->head;

	//reset timer

	return 1;
}



