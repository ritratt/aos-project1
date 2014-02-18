#include "template.h"
#include <sys/time.h>
#include <ucontext.h>
#include "gtthread.h"

#define MEM 64000
#define RUNNING 1
#define READY 2
#define FINISHED 3
#define JOINING 4
#define CANCELLED 5

/*typedef struct thread_t{
	int tid;
	struct	thread_t *next;
	ucontext_t context;
	int state;
	int joiner_count;
	void *ret;
	int joinee_tid;
}gtthread_t;*/

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
	//puts("Exit gonna be called from wrapper");
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
/*
gtthread_t* get_joinfree() {
	gtthread_t *t; 
	t = task_queue->head;
	while(t != NULL) {
		t = task_queue->head->next;
		if(t->state == READY) 
			return t;
		else
		{
			printf("Calling headtotail\n");
			headtotail();
		}
	}
	return t;
}*/

int get_task_queue_length() {
	gtthread_t *temp = task_queue->head;
	int count = 0;
	while(temp != NULL) {
		count++;
		temp = temp->next;
	}
	//printf("Count is %d\n", count);
	return count;
}

gtthread_t* get_joinfree() {
        gtthread_t *t;
        t = task_queue->head;
	int c = get_task_queue_length();
	int i = 0;
        while(i < c) {
                t = task_queue->head->next;
                if(t->state == READY || t->state == RUNNING)
                        return t;
                else
                {
			i++;
                        headtotail();
                }
        }
        return NULL;
}

void fun_alarm_handler(int sig) {
	//printf("Rescheduling...\n");
	gtthread_t *current_thread = task_queue->head;
	//printf("Current running thread is %d\n", current_thread->tid);
	if(current_thread->next != NULL) {
		task_queue->head = get_joinfree();
		if (task_queue->head == NULL)
			exit(1);
		//task_queue->head = task_queue->head->next;
		task_queue->tail->next = current_thread;
	}
	else {
		printf("Only one thread in queue. Returning.\n");
		return;
	}
	//printf("Thread %d moved to head.\n", task_queue->head->tid);
//	gtthread_t *tail_thread = task_queue->tail;
	task_queue->tail = current_thread;
	task_queue->tail->next = NULL;
	//Swap context
	ucontext_t current_context = current_thread->context;
	ucontext_t next_context = task_queue->head->context;
	/*if(&current_context == NULL) {
		printf("Next tid is %d\n", current_head->tid);
		printf("Current context is null.\n");
	}*/
	
	//Reset timer
	setitimer(ITIMER_REAL, &timer, NULL);
	
	if(task_queue->tail->state == RUNNING)
		task_queue->tail->state = READY;
	task_queue->head->state = RUNNING;
	swapcontext(&(task_queue->tail->context), &(task_queue->head->context));
	return;
}

void add_to_queue(gtthread_t *new_tail) {
	gtthread_t *current_tail = task_queue->tail;
	//printf("Current tail tid is %d\n", task_queue->tail->tid);
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
	if(current_head->state != JOINING && current_head->state != CANCELLED && current_head->state != FINISHED) 
		current_head->state = READY;
	task_queue->head = task_queue->head->next;
	task_queue->tail->next = current_head;
	task_queue->tail = current_head;
	task_queue->tail->next = NULL;
}
void gtthread_init(long period) {
	task_queue = (struct queue *) malloc(sizeof(struct queue));
	task_queue->head = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	task_queue->tail= (struct gtthread_t *) malloc(sizeof(gtthread_t));
	
	//Create dummy thread for main and make it as queue head.
	gtthread_t *main_thread = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	ucontext_t init_context;
	getcontext(&init_context);
	main_context = (ucontext_t) *(init_context.uc_link);
	main_thread->tid = 1;
	main_thread->state = RUNNING;
	main_thread->context = main_context; //init_context.uc_link;
	main_thread->joiner_count = 0;
	main_thread->ret = NULL;
	main_thread->joinee_tid = 0;
	task_queue->head = main_thread;
	task_queue->tail= main_thread;
	printf("As per init, current tail tid is %d\n", task_queue->tail->tid);

	//Initialize timer struct
	timer.it_interval.tv_sec = 0;
	timer.it_interval.tv_usec = period;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = period;

	/*Test timer values
        struct itimerval timerval;
	getitimer(ITIMER_VIRTUAL, &timerval);
	printf("New Timer val is %d\n", timerval.it_interval.tv_sec);*/
	
	//Register signal handler
	alarm_handler.sa_handler = fun_alarm_handler;
	sigaction(SIGALRM, &alarm_handler, NULL);

	//Set the timer
	setitimer(ITIMER_REAL, &timer, NULL);
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
	
	if(joiner_thread.state == FINISHED) {
		if(status != NULL) {
			*status = malloc(sizeof(int));
			*status = joiner_thread.ret;
		}
	return 0;
	}
		
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

void gtthread_exit(void *retval) {

	gtthread_t *current_head = task_queue->head;
	printf("gtthread exiting %d\n", task_queue->head->tid);

	//Assign retval to the executing head before it exits
	task_queue->head->ret = retval;
	//printf("Return value from exit is %d as per thread struct and as per arg\n", (int) current_head->ret);

	//Mark current thread as FINISHED.
	task_queue->head->state = FINISHED;

	//Decrement joinee count for the thread which this thread wants to join
	gtthread_t *joinee_thread;// = (struct gtthread_t *) malloc(sizeof(gtthread_t));
	int joinee_tid = task_queue->head->joinee_tid;
	if(joinee_tid != 0) {
		joinee_thread = get_thread(joinee_tid);
		joinee_thread->joiner_count = joinee_thread->joiner_count - 1;
		if(joinee_thread->joiner_count == 0) {
			joinee_thread->state = READY;
		}
		if(joinee_thread->joiner_count < 0) {
			printf("Something sinister brews...Join is not working as expected.\n");
			return;
		}
	}
/*
	//New head is the head's next thread.
	task_queue->head = task_queue->head->next;
	gtthread_t *new_head = get_joinfree();
	task_queue->head = new_head;

	//Swap the new head in.
	swapcontext(&(current_head->context), &(new_head->context));*/
	setitimer(ITIMER_REAL, &timer, NULL);
	fun_alarm_handler(1);
	return;
}

int gtthread_yield(void) {
	headtotail();
	fun_alarm_handler(SIGALRM);
	return 0;
}

int gtthread_equal(gtthread_t t1, gtthread_t t2) {
	if (t1.tid == t2.tid)
		return 1;
	else
		return 0;
}

int gtthread_cancel(gtthread_t thread) {
	gtthread_t *temp = get_thread(thread.tid);
	if(temp != NULL) {
	if (task_queue->head->tid == temp->tid) {
		gtthread_exit(thread.ret);
		temp->state = CANCELLED;
		return 0;
	}
		else {
			temp->state = CANCELLED;
			return 0;
		}
	}
	else {
		printf("Thread not found.\n");
		return -1;
	}
}
gtthread_t gtthread_self(void) {
	gtthread_t self_t;
	self_t = *(task_queue->head);
	return self_t;
}

