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

#include <malloc.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void *print_attr(void *param) {
  int *exit_code = (int *)malloc(sizeof(int));

  if (!param) {
    *exit_code = -1;
    pthread_exit(exit_code);
  }
  *exit_code = 0; // Assume everthing is okay..
  int thread_type = *((int *)param);

  if (!thread_type) {
    printf("Default Attributes are as follows:\n");
  } else {
    printf("Custom Attributes are as follows:\n");
  }

  pthread_attr_t self_thread_attr;
  pthread_getattr_np(pthread_self(), &self_thread_attr);

  int scope = 0;
  int rc = pthread_attr_getscope(&self_thread_attr, &scope);

  printf("Contention scope is %s\n",
         scope ? "PTHREAD_SCOPE_PROCESS" : "PTHREAD_SCOPE_SYSTEM");

  int detach_state = 0;
  rc |= pthread_attr_getdetachstate(&self_thread_attr, &detach_state);
  printf("Detach state is %s\n",
         detach_state ? "PTHREAD_CREATE_DETACHED" : "PTHREAD_CREATE_JOINABLE");

  void *stackaddr = NULL;
  size_t stacksize = 0;
  rc |= pthread_attr_getstack(&self_thread_attr, &stackaddr, &stacksize);
  printf("Thread's stack @ %p (%lu Bytes)\n", stackaddr, stacksize);

  // deprecated getstackaddr()
  // rc |= pthread_attr_getstackaddr(&self_thread_attr, &stackaddr);
  rc |= pthread_attr_getstacksize(&self_thread_attr, &stacksize);
  printf("Thread's stack @ %p (%lu kB)\n", stackaddr, stacksize / 1024);

  int sched_policy = 0;
  rc |= pthread_attr_getschedpolicy(&self_thread_attr, &sched_policy);
  printf("Scheduling policy is %s\n",
         sched_policy ? (sched_policy == SCHED_FIFO ? "SCHED_FIFO" : "SCHED_RR")
                      : "SCHED_OTHER");

  int inherited_sched_policy = 0;
  rc |=
      pthread_attr_getinheritsched(&self_thread_attr, &inherited_sched_policy);
  printf("Inheritedsched policy is %s\n", inherited_sched_policy
                                              ? "PTHREAD_EXPLICIT_SCHED"
                                              : "PTHREAD_INHERIT_SCHED");

  return exit_code;
}

static int arg;
int main() {
  pthread_t print_attr_tid[2];
  pthread_attr_t print_attr_attr[2];
  arg = 0;

  pthread_attr_init(&print_attr_attr[0]);

  pthread_create(&print_attr_tid[0], &print_attr_attr[0], &print_attr, &arg);

  static int ret_code = 0;
  pthread_join(print_attr_tid[0], (void **)&ret_code);

  pthread_attr_destroy(&print_attr_attr[0]);
}