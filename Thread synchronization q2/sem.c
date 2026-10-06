#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define MAX_SEMS 32

struct semaphore {
  int value;
  int active;
  struct spinlock lock;
};

struct semaphore semtable[MAX_SEMS];

void
sem_init_table(void)
{
  for (int i = 0; i < MAX_SEMS; i++) {
    initlock(&semtable[i].lock, "semaphore");
    semtable[i].value = 0;
    semtable[i].active = 0;
  }
}

int
sem_alloc(int init_val)
{
  for (int i = 0; i < MAX_SEMS; i++) {
    acquire(&semtable[i].lock);
    if (!semtable[i].active) {
      semtable[i].active = 1;
      semtable[i].value = init_val;
      release(&semtable[i].lock);
      return i; // Return semaphore ID
    }
    release(&semtable[i].lock);
  }
  return -1; // No free semaphore available
}

int
sem_wait(int sem_id)
{
  if (sem_id < 0 || sem_id >= MAX_SEMS)
    return -1;

  struct semaphore *s = &semtable[sem_id];
  acquire(&s->lock);
  if (!s->active) {
    release(&s->lock);
    return -1;
  }

  while (s->value <= 0) {
    sleep(s, &s->lock);
  }
  s->value--;
  release(&s->lock);
  return 0;
}

int
sem_post(int sem_id)
{
  if (sem_id < 0 || sem_id >= MAX_SEMS)
    return -1;

  struct semaphore *s = &semtable[sem_id];
  acquire(&s->lock);
  if (!s->active) {
    release(&s->lock);
    return -1;
  }

  s->value++;
  wakeup(s);
  release(&s->lock);
  return 0;
}

int
sem_free(int sem_id)
{
  if (sem_id < 0 || sem_id >= MAX_SEMS)
    return -1;

  struct semaphore *s = &semtable[sem_id];
  acquire(&s->lock);
  s->active = 0;
  s->value = 0;
  release(&s->lock);
  return 0;
}