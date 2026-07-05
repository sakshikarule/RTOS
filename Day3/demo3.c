#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <pthread.h>

typedef struct shm {
    pthread_mutex_t m;
    pthread_mutexattr_t ma;
    pthread_cond_t c;
    pthread_condattr_t ca;
    int wakeup;
} shm_t;

shm_t *ptr;

void delay(void) {
    int i;
    for(i=0; i<10000000; i++)
        asm("nop");
}

void task_func(int cnt) {
    int i;

    pthread_mutex_lock(&ptr->m);
    while(ptr->wakeup == 0)
        pthread_cond_wait(&ptr->c, &ptr->m);
    pthread_mutex_unlock(&ptr->m);

    for(i=1; i<=cnt; i++) {
        printf("task (%d) : %d\n", getpid(), i);
        delay();
		if(i % 3 == 0)
			sched_yield();
    }

    shmdt(ptr);
    _exit(0);
}

void sigint_handler(int sig) {
    // do nothing
}

int main() {
    int i, ret, shmid;
    const int n = 3;
    struct sigaction sa;

    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = sigint_handler;
    ret = sigaction(SIGINT, &sa, NULL);
    if(ret < 0) {
        perror("sigaction() failed");
        _exit(1);
    }

    shmid = shmget(0x1234, sizeof(shm_t), IPC_CREAT | 0600);
    if(shmid < 0) {
        perror("shmget() failed");
        _exit(1);
    }

    ptr = shmat(shmid, NULL, 0);
    if(ptr == (void*)-1) {
        perror("shmctl() failed");
        shmctl(shmid, IPC_RMID, NULL);
        _exit(2);
    }

    ptr->wakeup = 0;
    pthread_mutexattr_init(&ptr->ma);
    pthread_mutexattr_setpshared(&ptr->ma, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&ptr->m, &ptr->ma);

    pthread_condattr_init(&ptr->ca);
    pthread_condattr_setpshared(&ptr->ca, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&ptr->c, &ptr->ca);

    shmctl(shmid, IPC_RMID, NULL);

    for(i = 1; i <= n; i++) {
        ret = fork();
        if(ret == 0)
            task_func(9);
    }
    printf("%d child tasks created.\npress ctrl+c to resume all tasks.\n", n);

    pause();

    ptr->wakeup = 1;
    pthread_cond_broadcast(&ptr->c);

    for(i = 1; i <= n; i++)
        wait(NULL);
    
    printf("bye!\n");
    pthread_cond_destroy(&ptr->c);
    pthread_mutex_destroy(&ptr->m);
    shmdt(ptr);
    return 0;
}

// sudo ./a.out
//      execute with parent's (bash) sched class i.e. TS with nice=0

// sudo chrt -f 50 ./a.out
//      execute with FF realtime sched class and priority=50 on same CPU (i.e. CPU0).

// sudo chrt -r 50 ./a.out
//      execute with RR realtime sched class and priority=50 on same CPU (i.e. CPU0).




