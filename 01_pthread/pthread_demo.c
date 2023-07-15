#include <pthread.h> // thread related APIs
#include <stdint.h>
#include <stdio.h>
#include <unistd.h> // sleep, pause, exit etc

#define COMPILER_THREAD_CANCEL 0

#define THREAD_MAX_COUNTER 100U
#define THREAD_NUM 5U
static pthread_t thread_hello[THREAD_NUM] = {0};
static pthread_attr_t thread_attr_hello[THREAD_NUM] = {0};

void *hello_thread_function(void *param) {
  int task_no = *((int *)param);
  pthread_t self_thread =
      pthread_self(); // return the caller thread's pthread handle

  int counter = 0;
  while (1) {
     if(counter > THREAD_MAX_COUNTER) {
        printf("Counter exceed for thread %d, cancelling...", task_no);
        pthread_cancel(self_thread); // submit the cancel request.
    }
    // print task number and thread id with gettid(): returns TID of
    // caller thread
    printf("Hello from task %d(%u)\n", task_no, gettid());
    fflush(stdout); // if '\n' is not present, the stdout needs to be flushed
                    // manually
    usleep((task_no + 1));
    counter++;


  }

  return NULL;
}

int main() {

  static int arg = 0; // must be static/ global / heap mem

  for (int i = 0; i < THREAD_NUM; i++) {
    arg = i;
    printf("Creating thread %d\n", i);
    unsigned int failure = pthread_create(
        &thread_hello[i], &thread_attr_hello[i], &hello_thread_function, &arg);
    if (failure) {
      printf("Error in creating thread %d", i);
    }

    usleep(100U);
  }
#if COMPILER_THREAD_CANCEL
  getchar();

  for (int i = 0; i < THREAD_NUM; i++) {
    pthread_cancel(thread_hello[i]);
  }
#else
  for (int i = 0; i < THREAD_NUM; i++) {
    pthread_join(thread_hello[i], NULL);
  }
#endif

  return 0;
}