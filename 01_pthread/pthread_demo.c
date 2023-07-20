#include <errno.h>
#include <fenv.h>    // floating point exception handling, link with -lm
#include <pthread.h> // POSIX thread related APIs, link with -lpthread
#include <signal.h> // contains all the signals from SIGHUP to SIGRTMAX etc (1 -64)
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h> // Unix Std lib: sleep, pause, exit etc

#define COMPILER_THREAD_CANCEL 0

#define THREAD_MAX_COUNTER 5U
#define THREAD_NUM 5U

static pthread_t thread_hello[THREAD_NUM] = {
    0}; // Thread descriptor: pthread Thread ID (not same as Linux TID)
static pthread_attr_t thread_attr_hello[THREAD_NUM] = {0};

pthread_once_t hello_thread_function_init_var = PTHREAD_ONCE_INIT;

/*
 * - pthread_key : common key, which stores data specific to thread (data is
 * unique to each thread).
 * - Usually used for storing thread specific configuration.
 * - Can be used as thread local storage(TLS) for cache, buffer, etc without
 * declaring global var; Ideally for TLS, a global variable with "__thread" attr
 * makes it unique variable instance for each thread where it is  used mainatin
 * thread specific context
 * - the destructor can be used to free all the thread resource at its
 *   termination against the key
 */

pthread_key_t hello_key;
// Dummy Thread Specific data (TSD)
struct hello_thread_spec_data {
  int task_no;
  const char *task_name;
};

// Destructor function for the thread key
static void hello_key_destructor(void *param) {
  free(param);
  return;
}

// Init function, which is called only once by a thread,
// while other threads wait until finish of init
static void hello_thread_function_init(void) {
  // Initialize all the shared resource
  // create a key with its destuctor
  pthread_key_create(&hello_key, &hello_key_destructor);
  printf("%s: init done!\n", __func__);
}

// A dummy thread function

static void *hello_thread_function(void *param) {

  int *exit_code = malloc(sizeof(int));
  if (!param) {
    printf("Thread: Invalid parameter sent! exiting...\n");

    *exit_code = 1;
    pthread_exit((void *)exit_code);
  }
  int task_no = *((int *)param);
  // Init function for hello_thread_function
  // Executed by only one (first) thread while all other wait
  // to complete the init
  pthread_once(&hello_thread_function_init_var, &hello_thread_function_init);

  char t_name[40];

  // if (task_no == 4) {
  //   feraiseexcept(FE_DIVBYZERO);
  //   int p = 0x99;
  //   float x = 0.0f;

  //   printf("Dividing something by 0 %d:%f", p, p / x);
  // }

  *exit_code = 0; // Assuming everything is ok
  // create and alloc TSD
  struct hello_thread_spec_data *t_data =
      (struct hello_thread_spec_data *)malloc(
          sizeof(struct hello_thread_spec_data));

  pthread_t self_thread =
      pthread_self(); // return the caller thread's pthread handle

  t_data->task_no = task_no;

  snprintf(t_name, sizeof(t_name), "task no %d", task_no);
  t_data->task_name = (const char *)t_name;
  pthread_setspecific(hello_key, t_data);
  int counter = 0;

  while (1) {
    if (counter > THREAD_MAX_COUNTER) {
      if (!(task_no % 2)) {
        // we shall free it as we are cancelling: valgrind pointed this memory
        // leak!! valgrind -s --leak-check=full --show-leak-kinds=all
        // ./pthread_demo
        free(exit_code);
        printf("Counter exceed for thread %d, cancelling...\n", task_no);
        /*
         * submit the cancel request to the queue the cancel request may be
         * immediately acted, based on
         * - thread cancellation state: enabled (default)/ disabled.
         * - thread cancellation type: deferred(default) or async (immdediate).
         * deferred: It looks for the cancellation points, i.e usually at
         * function calls like open, close, printf. Not all function calls act
         * as cancellation points, i.e. custom/ user function calls
         */
        pthread_cancel(self_thread);
      } else {
        printf("Counter exceed for thread %d, exiting...\n", task_no);
        pthread_exit(exit_code);
      }
    }

    // print task number and thread id with gettid(): returns TID of
    // caller thread and TSD
    printf("Hello from task %d(%u): Thread specfic data for \"hello_key\" : { "
           "%d, \"%s\"}; \n",
           task_no, (unsigned int)gettid(),
           ((struct hello_thread_spec_data *)pthread_getspecific(hello_key))
               ->task_no,
           ((struct hello_thread_spec_data *)pthread_getspecific(hello_key))
               ->task_name);
    // fflush(stdout); // if "newline" char is not present,
    // the stdout needs to be flushed manually
    sleep((task_no + 1));
    counter++;
  }

  return (void *)exit_code;
}

int main() {

  static int arg = 0; // must be static/ global / heap mem

  unsigned char unique_threads_flag = 1;
  for (int i = 0; i < THREAD_NUM; i++) {
    arg = i;
    printf("Creating thread %d\n", i);

    unsigned int failure = pthread_create(
        &thread_hello[i], &thread_attr_hello[i], &hello_thread_function, &arg);

    if (failure) {
      printf("Error in creating thread %d: %#x (%s)\n", i, failure,
             strerror(failure));
      // exit(EXIT_FAILURE);
    }

    if (i)
      if (pthread_equal(thread_hello[i], thread_hello[i - 1])) {
        printf("Thread %d and Thread %d are same\n", i, i - 1);
        unique_threads_flag = 0;
        break;
      }
    usleep(100U);
  }

  if (unique_threads_flag) {
    printf("All unique threads were created!\n");
  }

#if COMPILER_THREAD_CANCEL
  getchar();

  for (int i = 0; i < THREAD_NUM; i++) {
    pthread_cancel(thread_hello[i]);
  }
#else
  for (int i = 0; i < THREAD_NUM; i++) {

    /*
     * To capture the return status of the threat,
     * pass the below pointer to the pthread_join(TD, pointer);
     * In the thread routine, the status must be allocated on the
     * heap and pass it to pthread_exit() for example.
     * Once collected, free the pointer.
     * If NULL is passed as argument, return value is ignored.
     */
    if ((i % 2)) {
      // only for those threads that terminated with pthread_exit(exit_code)
      void *ret = NULL;
      // blocked until the corresponding thread returns
      int err = pthread_join(thread_hello[i], &ret);

      if (ret) {

        printf("Thread %d exited with error code %d\n", i, *((int *)ret));
        free(ret);
      }
      if (err) {
        printf("Error pthread join for %d %#x (%s)", i, err, strerror(err));
        exit(EXIT_FAILURE);
      }
    } else {
      /*
       * Detached threads: Work indepedent of main(here)/ Application.
       * i.e main/ App should not wait or be blocked for this thread.
       * Calling pthread_detach is a way to tell the pthread lib/OS to reclaim
       * the resource (mem, thread ID, etc) on its termination. The exit code is
       * not collected. So, if this thread is meant to do work independently and
       * its exit code is not relevant, then use this api. Also, a detached
       * thread must not be called for pthread_join, as it can lead to undefined
       * behaviour.
       */

      int err = pthread_detach(thread_hello[i]);
      if (err) {
        printf("Error pthread detach for %d %#x (%s)", i, err, strerror(err));
        exit(EXIT_FAILURE);
      }
    }
  }
  // This is for demo, lets wait for thread 4 to complete, inorder to avoid
  // SIGSEGV due to deletion of the key (as thread 4 is detached, key_delete
  // would be executed otherwise)
  getchar();
  pthread_key_delete(hello_key);
#endif

  printf("All threads finished executing...\n");
  // pause(); // just to finish all the thread exec.
  return 0;
}
