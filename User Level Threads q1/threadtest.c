#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_THREADS 3
#define ITERATIONS 100000

volatile int counter = 0;

// Match the signature expected by clone: void function receiving void*
void worker(void *arg) {
  for(int i = 0; i < ITERATIONS; i++) {
    counter++; // Shared counter race condition
  }
  exit(); // Exit thread cleanly
}

int main(int argc, char *argv[]) {
  for(int i = 0; i < NUM_THREADS; i++) {
    // 1. Allocate 4096 bytes for user thread stack
    void *stack = malloc(4096);
    if(stack == 0){
      printf(1, "malloc failed\n");
      exit();
    }

    // 2. Call clone: 
    // Pass top of stack (stack + 4096) if your kernel clone expects top pointer,
    // OR pass base pointer if your clone() handles top pointer calculation internally.
    int pid = clone(worker, 0, stack);
    if(pid < 0){
      printf(1, "clone failed\n");
      exit();
    }
  }

  for(int i = 0; i < NUM_THREADS; i++) {
    join();
  }

  printf(1, "Final Counter Value: %d (Expected: %d)\n", counter, NUM_THREADS * ITERATIONS);
  exit();
}