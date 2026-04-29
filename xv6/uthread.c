#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"

#define MAX_THREADS 16
#define STACK_SIZE  4096

enum thread_state {
  T_FREE,
  T_RUNNABLE,
  T_RUNNING,
  T_ZOMBIE
};

struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

struct thread {
  tid_t tid;
  int state;
  void *stack;
  struct context *context;
};

static struct thread threads[MAX_THREADS];
static struct thread *current_thread;
static int initialized = 0;
static int next_tid = 1;

static void
thread_stub(void (*fn)(void *), void *arg)
{
  fn(arg);
  current_thread->state = T_ZOMBIE;
  thread_yield();
  exit();
}

void
thread_init(void)
{
  int i;

  if(initialized)
    return;

  for(i = 0; i < MAX_THREADS; i++){
    threads[i].tid = 0;
    threads[i].state = T_FREE;
    threads[i].stack = 0;
    threads[i].context = 0;
  }

  threads[0].tid = next_tid++;
  threads[0].state = T_RUNNING;
  threads[0].stack = 0;
  threads[0].context = 0;
  current_thread = &threads[0];

  initialized = 1;
}

tid_t
thread_create(void (*fn)(void *), void *arg)
{
  int i;
  char *stack;
  uint *sp;
  struct thread *t;

  if(!initialized)
    thread_init();

  for(i = 0; i < MAX_THREADS; i++){
    if(threads[i].state == T_FREE)
      break;
  }

  if(i == MAX_THREADS)
    return -1;

  t = &threads[i];
  stack = malloc(STACK_SIZE);
  if(stack == 0)
    return -1;

  sp = (uint *)(stack + STACK_SIZE);

  *--sp = (uint)arg;
  *--sp = (uint)fn;
  *--sp = 0;

  sp -= 5;
  t->context = (struct context *)sp;
  t->context->eip = (uint)thread_stub;

  t->tid = next_tid++;
  t->state = T_RUNNABLE;
  t->stack = stack;

  return t->tid;
}

void
thread_yield(void)
{
  int i;
  int curr_index;
  struct thread *prev;
  struct thread *next;

  if(!initialized)
    thread_init();

  prev = current_thread;
  next = 0;
  curr_index = current_thread - threads;

  for(i = 1; i < MAX_THREADS; i++){
    int idx = (curr_index + i) % MAX_THREADS;
    if(threads[idx].state == T_RUNNABLE){
      next = &threads[idx];
      break;
    }
  }

  if(next == 0)
    return;

  if(prev->state == T_RUNNING)
    prev->state = T_RUNNABLE;

  next->state = T_RUNNING;
  current_thread = next;
  uswtch(&prev->context, next->context);
}

int
thread_join(tid_t tid)
{
  int i;
  struct thread *t;

  for(;;){
    for(i = 0; i < MAX_THREADS; i++){
      t = &threads[i];
      if(t->tid == tid && t->state != T_FREE){
        if(t->state == T_ZOMBIE){
          if(t->stack)
            free(t->stack);
          t->stack = 0;
          t->context = 0;
          t->tid = 0;
          t->state = T_FREE;
          return 0;
        }
        break;
      }
    }

    if(i == MAX_THREADS)
      return -1;

    thread_yield();
  }
}