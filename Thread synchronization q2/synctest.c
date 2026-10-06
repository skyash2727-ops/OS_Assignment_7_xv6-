#include "types.h"
#include "stat.h"
#include "user.h"

void
test_producer_consumer(void)
{
  printf(1, "\n--- Test 1: Producer / Consumer ---\n");

  int sem_mutex = sem_alloc(1);  // Binary semaphore for critical section
  int sem_full = sem_alloc(0);   // Counts items produced

  int pid = fork();
  if (pid < 0) {
    printf(1, "Fork failed\n");
    exit();
  }

  if (pid == 0) {
    // Child: Producer
    for (int i = 1; i <= 3; i++) {
      sem_wait(sem_mutex);
      printf(1, "[Producer] Producing item %d\n", i);
      sem_post(sem_mutex);

      sem_post(sem_full); // Signal item available
      sleep(10);          // Delay to simulate work
    }
    exit();
  } else {
    // Parent: Consumer
    for (int i = 1; i <= 3; i++) {
      sem_wait(sem_full); // Wait for item
      sem_wait(sem_mutex);
      printf(1, "[Consumer] Consumed item %d\n", i);
      sem_post(sem_mutex);
    }
    wait();
  }

  sem_free(sem_mutex);
  sem_free(sem_full);
  printf(1, "Test 1 Passed!\n");
}

void
test_ordering(void)
{
  printf(1, "\n--- Test 2: Enforced Parent-Child Execution Order ---\n");

  int sem_step1 = sem_alloc(0);
  int sem_step2 = sem_alloc(0);

  int pid = fork();
  if (pid == 0) {
    // Child process runs step 2
    sem_wait(sem_step1); // Must wait for parent to print Step 1
    printf(1, "Child: Executing Step 2\n");
    sem_post(sem_step2); // Unlock step 3 for parent
    exit();
  } else {
    // Parent process runs step 1 and step 3
    printf(1, "Parent: Executing Step 1\n");
    sem_post(sem_step1); // Unlock step 2 for child

    sem_wait(sem_step2); // Wait for child to complete step 2
    printf(1, "Parent: Executing Step 3\n");
    wait();
  }

  sem_free(sem_step1);
  sem_free(sem_step2);
  printf(1, "Test 2 Passed!\n");
}

int
main(int argc, char *argv[])
{
  printf(1, "Starting Semaphore Synchronization Tests...\n");

  test_producer_consumer();
  test_ordering();

  printf(1, "\nAll Semaphore Tests Completed Successfully!\n");
  exit();
}