#include "template.h"
#include <sched.h>
#include <sys/syscall.h>
#include "mutex.h"
#include <signal.h>
#include <sys/time.h>

int test_var = 10;
int mutex = 0;

void test() {
	printf("Thread pid is %d\n", getpid());
	lock(mutex);
	test_var = 0;
	printf("Test var is %d\n", test_var);
	unlock(mutex);
	printf("Test thread id is %d\n", (int) syscall(SYS_gettid));
	return;
}
void FuckYou_SigSev(int sig) {
	printf("Malloc dammit! >_<\n");
	printf("Scheduler yielding for thread id %d\n", syscall(SYS_gettid));
	return;
}

void RR_switch(int sig) {
	printf("Scheduler yielding for thread id %d\n", syscall(SYS_gettid));
	sched_yield();
}

int main() {
	printf("Main pid is %d\n", getpid());
	struct itimerval *newtimer;
       	newtimer->it_value->tv_sec = 3;
	setitimer(ITIMER_REAL, newtimer, NULL); 
	struct sigaction new_action, old_action;
	new_action.sa_handler = RR_switch;
	sigaction(SIGALRM, &new_action, NULL);
	int pagesize = (int) getpagesize();
	void *t_stack = malloc(pagesize);
	int child_t = clone(&test, t_stack + pagesize, CLONE_THREAD || CLONE_VM || CLONE_CHILD_SETTID || CLONE_VFORK || CLONE_SIGHAND, NULL);
	printf("Child thread id from main is %d\n", child_t);	
	printf("Main thread id is %d\n", (int) syscall(SYS_gettid));
	return 1;
}

