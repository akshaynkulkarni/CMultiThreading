/*
 * Based on
 * https://docs.oracle.com/cd/E19455-01/806-5257/attrib-34610/index.html
 * https://docs.oracle.com/cd/E19455-01/806-5257/6je9h032j/index.html
 * https://man7.org/linux/man-pages/man3/pthread_attr_setscope.3.html
 * https://man7.org/linux/man-pages/man3/pthread_attr_setdetachstate.3.html
 * https://man7.org/linux/man-pages/man3/pthread_attr_setstack.3.html
 *
 *
 * - pthread_attr_init(): init the attribute object to default --> for creating
 * 						            default thread.
 *                        return 0 => sucessful.
 * - pthread_attr_destroy(): destory attribute object
 * ==========================================================
 * Attributes --> Possible values (d:default) : Explaination
 * ==========================================================
 * - contention scope --> scope of threads to competes for resources, ex: CPU.
 *           (*) PTHREAD_SCOPE_PROCESS: thread competes within the process.
 *           (*) PTHREAD_SCOPE_SYSTEM: thread competes with all threads in the
 *                                   system within the same scheduling domain.
 *  Default scope is implemenation dependent!
 *  The thread scheduling is based on the sched policy and priority.
 *  APIs: pthread_attr_setscope(), pthread_attr_getscope();
 *  In Ubuntu 20.04: Only system scope is supported.
 * --------------------------------------------------------------------
 * - detachstate --> state of thread as joinable or detached
 *            (*) PTHREAD_CREATE_JOINABLE(d): Thread is joinable, exit status &
 *                                         thread is preserved after its
 *                                         termination. Must be joined with
 *                                         pthread_join() or detached with
 *                                         pthread_detach();
 *            (*) PTHREAD_CREATE_DETACHED: Thread executes independent of other
 *                                     threads, No exit status is collected,
 *                                     resources are reclaimed by the system
 *                                     automatically on thread termination.
 *                                     Undefined behavior for calling
 *                                     pthread_join() or  pthread_detach();
 * APIs: pthread_attr_setdetachstate(), pthread_attr_getdetachstate();
 * -----------------------------------------------------------------------
 * - stackaddr --> (*) NULL (d): automatic allocation of stackaddr and size
 *                 (*) Custom: use API pthread_attr_setstack() with min size
 *                             PTHREAD_STACK_MIN. stackaddr must point to
 *                             lowest address of stack buffer.
 * APIs: pthread_attr_setstack(), pthread_attr_getstack()
 * Deprecated: pthread_attr_setstackaddr, pthread_attr_getstackaddr
 * Using these APIs, the user must provide the stack buffer and manage it
 * against stackover flow (allocate gaurd area). The stackbuffer must be
 * aligned as multiple of sys page size. Use posix_memalign() for buffer.
 * ------------------------------------------------------------------------
 * - stacksize: depends "$ ulimit -s"; OS dependent; Ubuntu : 8MegaBytes
 * APIs: pthread_attr_setstacksize(), pthread_attr_getstacksize().
 * Min size: PTHREAD_STACK_MIN (16kb), if 0 is specified, default size is
 * considered. PTHREAD_STACK_MIN is min size required to start a thread.
 * ------------------------------------------------------------------------
 * - scheduling policy: SCHED_FIFO
 *                      SCHED_RR
 *                      SCHED_OTHER (d)
 * APIs: pthread_attr_setschedpolicy(), pthread_attr_getschedpolicy()
 * --------------------------------------------------------------------------
 * - inherited scheduling policy --> Tells if the sched policy was inherited?
 *        (*) PTHREAD_INHERIT_SCHED (d): Inherited parent thread and attr in
 *                                       pthread_create() are ignored.
 *        (*) PTHREAD_EXPLICIT_SCHED : sched policy was explicitly set
 *                                     during the thread creation so, use the
 *                                     attr specified for pthread_create().
 * APIs: pthread_attr_setinheritsched(), pthread_attr_getinheritsched()
 * --------------------------------------------------------------------------
 * - scheduling priority -> Thread priority
 *               (*) Inherit parent thread priority(d)
 *               (*) explictly set it using struct sched_param
 *                   using API pthread_attr_setschedparam().
 * APIs: pthread_attr_getschedparam(), pthread_attr_setschedparam()
 *   Always get the param from the API, update the param and then set
 *   it using the API
 * ------------------------------------------------------------------------------
 * - guardsize: PAGESIZE --> Stack overflow protection
 */

// for pthread_getattr_np: warning: implicit declaration .....
#define _GNU_SOURCE

#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <sched.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ERROR_CHECK(X, WARN)                                                   \
  do {                                                                         \
    if (X) {                                                                   \
      printf("Error occured %d, %s @ line no. %d\n", X, strerror(X),           \
             __LINE__);                                                        \
      if (!WARN) {                                                             \
        exit(EXIT_FAILURE);                                                    \
      }                                                                        \
    }                                                                          \
  } while (0);

static sem_t sync_for_detatched_thread;

void *print_attr(void *param) {
  int *exit_code = (int *)malloc(sizeof(int));

  if (!param) {
    *exit_code = -1;
    pthread_exit(exit_code);
  }
  *exit_code = 0; // Assume everthing is okay..
  int thread_type = *((int *)param);

  printf("======================================\nThread no. %d\n",
         thread_type);
  if (!thread_type) {
    printf("Default Attributes are as follows:\n");
  } else {
    printf("Custom Attributes are as follows:\n");
  }
  printf("-------------------------------------\n");
  pthread_attr_t self_thread_attr;
  pthread_getattr_np(pthread_self(), &self_thread_attr);

  int scope = 0;
  int rc = pthread_attr_getscope(&self_thread_attr, &scope);
  ERROR_CHECK(rc, 1);

  printf("Contention scope is %s\n",
         scope ? "PTHREAD_SCOPE_PROCESS" : "PTHREAD_SCOPE_SYSTEM");

  int detach_state = 0;
  rc = pthread_attr_getdetachstate(&self_thread_attr, &detach_state);
  ERROR_CHECK(rc, 1);
  printf("Detach state is %s\n",
         detach_state ? "PTHREAD_CREATE_DETACHED" : "PTHREAD_CREATE_JOINABLE");

  void *stackaddr = NULL;
  size_t stacksize = 0;
  rc = pthread_attr_getstack(&self_thread_attr, &stackaddr, &stacksize);
  ERROR_CHECK(rc, 1);
  printf("Thread's stack @ %p (%lu Bytes)\n", stackaddr, stacksize);

  // deprecated getstackaddr()
  // rc |= pthread_attr_getstackaddr(&self_thread_attr, &stackaddr);
  rc = pthread_attr_getstacksize(&self_thread_attr, &stacksize);
  ERROR_CHECK(rc, 1);
  printf("Thread's stack @ %p (%lu kB)\n", stackaddr, stacksize / 1024);

  int sched_policy = 0;
  rc = pthread_attr_getschedpolicy(&self_thread_attr, &sched_policy);
  ERROR_CHECK(rc, 1);
  printf("Scheduling policy is %s\n",
         sched_policy ? (sched_policy == SCHED_FIFO ? "SCHED_FIFO" : "SCHED_RR")
                      : "SCHED_OTHER");

  int inherited_sched_policy = 0;
  rc = pthread_attr_getinheritsched(&self_thread_attr, &inherited_sched_policy);
  ERROR_CHECK(rc, 1);
  printf("Inheritedsched policy is %s\n", inherited_sched_policy
                                              ? "PTHREAD_EXPLICIT_SCHED"
                                              : "PTHREAD_INHERIT_SCHED");

  size_t guard_size = 0;
  rc = pthread_attr_getguardsize(&self_thread_attr, &guard_size);
  ERROR_CHECK(rc, 1);

  printf("Guard size = %lu bytes\n", guard_size);

  static struct sched_param sched_params = {0};

  rc = pthread_attr_getschedparam(&self_thread_attr, &sched_params);
  ERROR_CHECK(rc, 1);
  int min_priority = sched_get_priority_min(sched_policy);
  int max_priority = sched_get_priority_max(sched_policy);

  printf("sched_param.sched_priority: %d\nmin prio: %d, max prio: %d for %s\n",
         sched_params.sched_priority, min_priority, max_priority,
         sched_policy ? (sched_policy == SCHED_FIFO ? "SCHED_FIFO" : "SCHED_RR")
                      : "SCHED_OTHER");

  if (sched_policy != SCHED_OTHER) {
    /*
     * for SCHED_OTHER, the default and only prio is 0; Hence
     * min = max = 0, However, SCHED_OTHER makes the scheduler
     * consider nice value and ensure fairness among all the
     * threads running with this policy.
     * This is scheduling policy that is considered by the
     * scheduler and then other parametes for scheduling
     */
    // just some prio in range

    sched_params.sched_priority =
        min_priority + ((max_priority - min_priority) / 2) + 5;
    rc = pthread_attr_setschedparam(&self_thread_attr, &sched_params);
    ERROR_CHECK(rc, 1);
    rc = pthread_attr_getschedparam(&self_thread_attr, &sched_params);
    ERROR_CHECK(rc, 1);

    printf("after setting prio:sched_param: %d\n", sched_params.sched_priority);
  }
  printf("======================================\n");
  getchar();
  if (thread_type) {
    if (sem_post(&sync_for_detatched_thread)) {
      printf("Unable to post semaphore!\n");
      *exit_code = EDEADLK;
      pthread_exit(exit_code);
    }
  }
  return exit_code;
}

static int arg;

int main() {
  pthread_t print_attr_tid[3];
  pthread_attr_t print_attr_attr[3];
  void *sp;
  arg = 0;
  pthread_attr_t main_thread_attr;
  pthread_getattr_np(pthread_self(), &main_thread_attr);

  if (sem_init(&sync_for_detatched_thread, 0, 0)) {
    printf("error creating semaphore for sync_for_detatched_thread: %s\n",
           strerror(errno));
    exit(EXIT_FAILURE);
  }
  int rc = 0;

  for (int i = 0; i < 3; i++) {
    arg = i;
    rc = pthread_attr_init(&print_attr_attr[i]);
    ERROR_CHECK(rc, 0);

    if (i) { // custom thread attr

      int state = PTHREAD_CREATE_DETACHED;
      rc = pthread_attr_setdetachstate(&print_attr_attr[i], state);
      ERROR_CHECK(rc, 0);

      static struct sched_param sched_params = {0};

      int sched_policy = 0;
      rc = pthread_attr_getschedpolicy(&main_thread_attr, &sched_policy);
      printf("main: Def Scheduling policy for thread %d is %s\n", i,
             sched_policy
                 ? (sched_policy == SCHED_FIFO ? "SCHED_FIFO" : "SCHED_RR")
                 : "SCHED_OTHER");

      sched_policy = i == 1 ? SCHED_RR : SCHED_FIFO;
      printf("main: Setting Scheduling policy for thread %d to %s\n", i,
             sched_policy
                 ? (sched_policy == SCHED_FIFO ? "SCHED_FIFO" : "SCHED_RR")
                 : "SCHED_OTHER");
      rc = pthread_attr_setschedpolicy(&print_attr_attr[i], sched_policy);
      ERROR_CHECK(rc, 0);

      rc = pthread_attr_getschedparam(&main_thread_attr, &sched_params);
      ERROR_CHECK(rc, 0);

      int min_priority = sched_get_priority_min(sched_policy);
      int max_priority = sched_get_priority_max(sched_policy);
      sched_params.sched_priority =
          min_priority + ((max_priority - min_priority) / 2);

      printf("main: sched_param.sched_priority: %d\nmain: min prio: %d, max "
             "prio: %d "
             "for %s\n",
             sched_params.sched_priority, min_priority, max_priority,
             sched_policy
                 ? (sched_policy == SCHED_FIFO ? "SCHED_FIFO" : "SCHED_RR")
                 : "SCHED_OTHER");

      rc = pthread_attr_setschedparam(&print_attr_attr[i], &sched_params);
      ERROR_CHECK(rc, 0);

      int scope = PTHREAD_SCOPE_PROCESS;
      printf("main: Setting the thread contention to PTHREAD_SCOPE_PROCESS\n");
      rc = pthread_attr_setscope(&print_attr_attr[i], scope);
      ERROR_CHECK(rc, 1);

      size_t page_size = getpagesize();
      size_t stack_size = (1 << 16) + page_size;

      if (i == 1) {              // thread no 1, allocate custom stack mem
        sp = malloc(stack_size); // heap memory and no guard set!
        if (!sp) {
          ERROR_CHECK(ENOMEM, 0);
        }
        rc = pthread_attr_setstack(&print_attr_attr[i], sp, stack_size);
        ERROR_CHECK(rc, 0);
      } else {
        rc = pthread_attr_setstacksize(&print_attr_attr[i], stack_size);
        ERROR_CHECK(rc, 0);
      }
      // TBD for thread1
      rc = pthread_attr_setguardsize(&print_attr_attr[i], 2 * page_size);
      ERROR_CHECK(rc, 0);

      rc = pthread_attr_setinheritsched(&print_attr_attr[i],
                                        PTHREAD_EXPLICIT_SCHED);
      ERROR_CHECK(rc, 0);
    }

    rc = pthread_create(&print_attr_tid[i], &print_attr_attr[i], &print_attr,
                        &arg);
    ERROR_CHECK(rc, 0);
    if (!i) {
      int *ret_code = NULL;
      rc = pthread_join(print_attr_tid[i], (void **)&ret_code);
      ERROR_CHECK(rc, 0);
      if (ret_code) {
        printf("thread %d exited with code %x\n", i, *ret_code);
        free(ret_code);
      }
    } else {
      while (0 != sem_wait(&sync_for_detatched_thread))
        ;
      free(sp);
    }
    rc = pthread_attr_destroy(&print_attr_attr[i]);
    ERROR_CHECK(rc, 0);
  }

  return 0;
}