#ifndef SPROUT_WORK_H
#define SPROUT_WORK_H

#include <stddef.h>

typedef struct Group Group;

typedef void (*routine_func)(void *env);

Group *group_new(size_t);
void   group_go(Group*, routine_func, void*);
void   group_wait(Group*);

#endif