#include "template.h"
#include <sched.h>
#include <sys/syscall.h>
#include "mutex.h"
#include <signal.h>
#include <sys/time.h>

int test_var = 10;
int mutex = 0;

typedef struct {
	int tid;
	ucontext_t curr_cxt; 
} gtthread;

struct Node {
	gtthread thread;
	Node *next;
}

struct task_queue{
//add code
	Node head;
	Node 
};

//create global task queue?

void scheduler(gtthread *t) {
	
	//register sighandler
	//
}

void sched_sighandler(int sig, struct gtthread gtt) {
	update_queue(); //define this function
	struct t_head = queue->head;
	struct t_second = queue->head->next;
	int pid_head = t_head->tid;
	int pid_second = t_second->tid;
	kill(pid_head, SIGSTOP);
	kill(pid_second, SIGCONT);
	update_queue();
	//update itimer
}

void test() {
	printf("Thread pid is %d\n", getpid());
	while(1);
	/*struct itimerval curr;
	getitimer(ITIMER_REAL, &curr);
	printf("test() timer value is %d\n", curr.it_value.tv_usec);
	printf("test() timer value is %d\n", curr.it_interval.tv_usec);*/
	printf("mutex %d\n", mutex);
	lock(mutex);
	test_var = 0;
	printf("Test var is %d\n", test_var);
	unlock(mutex);
	printf("Test thread id is %d\n", (int) syscall(SYS_gettid));
	return;
}
void FuckYou_SigSev(int sig) {
	printf("Malloc dammit! >_<\n");
	exit(1);
}

void RR_switch(int sig) {
	printf("Scheduler yielding for thread id %d\n", syscall(SYS_gettid));
	sched_yield();
}

void main() {
	printf("Main pid is %d\n", getpid());
	struct itimerval newtimer;
	printf("Main timer value is %d\n", newtimer.it_interval.tv_usec);
	newtimer.it_interval.tv_usec = 0;
	newtimer.it_interval.tv_sec = 0;
       	newtimer.it_value.tv_sec = 3;
       	newtimer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &newtimer, NULL); 
	getitimer(ITIMER_REAL, &newtimer);
	printf("New main timer value is %u\n", newtimer.it_value.tv_sec);
	struct sigaction new_action, old_action;
	new_action.sa_handler = RR_switch;
	sigaction(SIGALRM, &new_action, NULL);
	int pagesize = (int) getpagesize();
	void *t_stack = malloc(pagesize);
	printf("Cloning now.\n");
	int child_t = clone(&test, t_stack + pagesize, CLONE_THREAD || CLONE_VM || CLONE_CHILD_SETTID || CLONE_VFORK, NULL);
	alarm(2);
	printf("Child thread id from main is %d\n", child_t);	
	//return 1;
}

