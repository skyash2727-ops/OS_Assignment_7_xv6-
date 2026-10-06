#include "types.h"
#include "stat.h"
#include "user.h"

void cpu_work(void) {
  volatile double x = 0;
  for (int i = 0; i < 50000000; i++) {
    x += 3.14 * 2.71;
  }
}

int main(void) {
  int priorities[4] = {5, 10, 15, 20}; // Priority 5 is highest, 20 is lowest
  int pids[4];

  printf(1, "Starting Priority Scheduler Test...\n");

  for (int i = 0; i < 4; i++) {
    pids[i] = fork();
    if (pids[i] == 0) {
      // Child process sets its assigned priority
      int my_pid = getpid();
      set_priority(my_pid, priorities[i]);
      printf(1, "Child %d (PID: %d) started with priority %d\n", i + 1, my_pid, priorities[i]);
      
      cpu_work();

      printf(1, "Child %d (PID: %d) finished!\n", i + 1, my_pid);
      exit();
    }
  }

  for (int i = 0; i < 4; i++) {
    wait();
  }

  printf(1, "Priority Test completed.\n");
  exit();
}