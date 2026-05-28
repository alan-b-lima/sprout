#include "work.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "slice.h"

typedef struct routine {
    routine_func func;
    void        *env;
} routine;

typedef struct thread {
    size_t    id;
    pthread_t thread;
    bool      exec;
    bool      alive;
} thread;

typedef struct task {
    size_t id;
    Group *group;
} task;

typedef struct queue {
    slice(routine) routines;
    size_t         len;
    size_t         head;
    size_t         tail;
} queue;

queue   queue_new__(size_t);
bool    queue_empty__(queue*);
void    queue_enqueue__(queue*, routine);
routine queue_dequeue__(queue*);
void    queue_free__(queue *q);

struct Group {
    size_t        maxprocs;
    slice(thread) threads;
    queue         routines;

    pthread_mutex_t mu;
};

int   group_thread_spawn__(Group*);
int   group_thread_wakeup__(Group*);
void *group_scheduler__(void *env);

Group *group_new(size_t numprocs) {
    Group *group = calloc(1, sizeof(Group));

    group->maxprocs = numprocs;
    group->threads = make(thread, 0, numprocs);
    group->routines = queue_new__(numprocs);
    pthread_mutex_init(&group->mu, NULL);

    return group;
}

void group_go(Group *group, routine_func rnfunc, void *rnenv) {
    pthread_mutex_lock(&group->mu);

    routine rn = { .func = rnfunc, .env = rnenv };
    queue_enqueue__(&group->routines, rn);

    size_t num_thread = 0;
    for (size_t i = 0; i < len(thread, group->threads); i++) {
        thread thread = at(thread, group->threads, i);

        if (thread.alive) {
            if (!thread.exec) {
                pthread_kill(thread.thread, SIGUSR1);
                goto Exit;
            }
            num_thread++;
        }
    }

    if (num_thread > group->maxprocs) {
        goto Exit;
    }

    int err = group_thread_spawn__(group);
    if (err < 0) {
        fprintf(stderr, "create thread\n");
        abort();
    }

Exit:
    pthread_mutex_unlock(&group->mu);
}

int group_thread_spawn__(Group* group) {
    size_t index = len(thread, group->threads);
    for (size_t i = 0; i < len(thread, group->threads); i++) {
        if (!at(thread, group->threads, i).alive) {
            index = i;
            break;
        }
    }
    if (index == len(thread, group->threads)) {
        group->threads = append(thread, group->threads, (thread) {});
    }

    struct task *task = calloc(1, sizeof(struct task));
    task->id = index;
    task->group = group;

    pthread_t pthread;
    int err = pthread_create(&pthread, NULL, group_scheduler__, task);
    if (err < 0) {
        return err;
    }

    struct thread thread = { 0 };
    thread.id = index;
    thread.thread = pthread;
    thread.exec = false;
    thread.alive = true;

    at(thread, group->threads, index) = thread;

    return 0;
}

void group_wait(Group *group) {
    pthread_mutex_lock(&group->mu);

    pthread_mutex_unlock(&group->mu);
    pthread_mutex_destroy(&group->mu);

    queue_free__(&group->routines);
    release(thread, group->threads);
    free(group);
}

#define STATE_START 1
#define STATE_READY 2
#define STATE_EXEC  3
#define STATE_WAIT  4
#define STATE_EXIT  0

const char *STATES[] = {
    [STATE_START] = "start",
    [STATE_READY] = "ready",
    [STATE_EXEC]  = "exec",
    [STATE_WAIT]  = "wait",
    [STATE_EXIT]  = "exit",
};

void *group_scheduler__(void *env) {
    struct task *task = env;
    size_t id = task->id;
    Group *group = task->group;
    free(task);

    sigset_t set;
    sigemptyset(&set);

    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGUSR2);

    routine next;
    int state = STATE_START;

    while (true) {
        printf("thread in state %s\n", STATES[state]);

        switch (state) {
        case STATE_START: {
            state = STATE_READY;
        } break;

        case STATE_READY: {
            pthread_mutex_lock(&group->mu);

            queue *routines = &group->routines;
            if (queue_empty__(routines)) {
                state = STATE_WAIT;
                break;
            }

            next = queue_dequeue__(routines);

            pthread_mutex_unlock(&group->mu);

            state = STATE_EXEC;
        } break;

        case STATE_EXEC: {
            pthread_mutex_lock(&group->mu);
            at(thread, group->threads, id).exec = true;
            pthread_mutex_unlock(&group->mu);

            next.func(next.env);

            pthread_mutex_lock(&group->mu);
            at(thread, group->threads, id).exec = false;
            pthread_mutex_unlock(&group->mu);

            state = STATE_READY;
        } break;

        case STATE_WAIT: {
            int signal;
            if (sigwait(&set, &signal) < 0) {
                perror("sigwait");
                abort();
            }

            switch (signal) {
            case SIGUSR1:
                state = STATE_READY;
                break;

            case SIGUSR2:
                state = STATE_EXIT;
                break;
            }
        } break;

        case STATE_EXIT: {
            pthread_mutex_lock(&group->mu);
            at(thread, group->threads, id).alive = false;
            pthread_mutex_unlock(&group->mu);
            return NULL;
        } break;
        }
    }
}

queue queue_new__(size_t size) {
    queue q = { 0 };
    q.routines = make(routine, size, size);
    return q;
}

bool queue_empty__(queue *q) {
    return q->len == 0;
}

void queue_enqueue__(queue *q, routine rn) {
    if (len(routine, q->routines) == q->len) {
        q->routines = append(routine, q->routines, rn);
        if (q->tail == 0) {
            q->head = q->len;
            goto Push;
        }

        size_t tail = len(routine, q->routines) - q->len + q->tail;
        slice(routine) src = reslice(routine, q->routines, q->tail, len(routine, q->routines));
        slice(routine) dst = reslice(routine, q->routines, tail, len(routine, q->routines));

        copy(routine, dst, src);
        q->tail = tail;
    }

Push:
    at(routine, q->routines, q->head) = rn;

    q->head++;
    q->len++;

    if (q->head >= len(routine, q->routines)) {
        q->head = 0;
    }
}

routine queue_dequeue__(queue *q) {
    routine rn = at(routine, q->routines, q->tail);

    q->tail++;
    q->len--;

    if (q->tail >= len(routine, q->routines)) {
        q->tail = 0;
    }

    return rn;
}

void queue_free__(queue *q) {
    release(routine, q->routines);
}
