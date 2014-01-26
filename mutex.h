#include <stdio.h>
#include <stdlib.h>
#include <sched.h>

void lock(int mutex);
void unlock(int mutex);

void lock(int mutex) {
	while(mutex != 0)
		sched_yield(); //Results in unfairness. Ideally should use spinlock. But netbook...fml.:
	mutex = 1;
}

void unlock(int mutex) {
	mutex = 0;
}

