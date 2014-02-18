// Test9
// Mutex and gtthread_yield. The program should print like below.
//
// thread 1
// thread 1
// thread 1
// thread 2
// thread 2
// thread 2

#include <stdio.h>
#include <gtthread.h>

gtthread_mutex_t g_mutex;
gtthread_mutex_t *chopstick[5];

/*
void masksignal() {
        sigset_t sigmaskset;
        sigemptyset(&sigmaskset);
        sigaddset(&sigmaskset, SIGALRM);
        sigprocmask(SIG_BLOCK, &sigmaskset, NULL);
}

void unmasksignal() {
        sigset_t sigmaskset;
        sigemptyset(&sigmaskset);
        sigaddset(&sigmaskset, SIGALRM);
        sigprocmask(SIG_UNBLOCK, &sigmaskset, NULL);
}
*/
void think(int id) {
        int r = rand() % 4;
        printf("Philosopher %d is thinking for %d seconds.\n", id, r);
	masksignal();
        sleep(r);
	unmasksignal();
}

void eat(int id) {
        int c[2];
        int r = rand() % 4;

	if(id == 0) {
        	c[0] = 4;
        	c[1] = 0;
	}
	else if(id % 2 == 1) {
        	c[0] = id;
        	c[1] = (id - 1) % 5;
	}
	else if(id % 2 == 0) {
		c[0] = (id - 1) % 5;
		c[1] = id;
	}
	int l1 = gtthread_mutex_lock(chopstick[c[0]]);
	int l2 = gtthread_mutex_lock(chopstick[c[1]]);
	
        if (l1 == 0 && l2 == 0) {
                printf("Philosopher %d is eating for %d seconds.\n", id, r);
		masksignal();
                sleep(r);
		unmasksignal();
        }
       	gtthread_mutex_unlock(chopstick[c[0]]);
	gtthread_mutex_unlock(chopstick[c[1]]);
	printf("Philosopher %d is full.\n", id);
}
void whateverphilosophersaresupposedtodo(void* arg) {
        int id = (int) arg;
        while(1) {
                think(id);
                eat(id);
        }
}

int main() {
        gtthread_init(300000);
        gtthread_t philosopher[5];
        int i;
        for(i = 0; i < 5; i++) {
                gtthread_create(&philosopher[i], (void *) whateverphilosophersaresupposedtodo, (void *) i);
                chopstick[i] = malloc(sizeof(gtthread_mutex_t));
                gtthread_mutex_init(chopstick[i]);
        }
        while(1);
        return 0;
}

