#include <unistd.h> // For Unix Posix Interfaces like sleep(), pause(), read(), write(), exit(), getpid(), gettid() etc

#include <fcntl.h> // file control operation: like permissions, open , close etc

#include <stdio.h> // standard input ouput interfaces

#include <sys/types.h> // for the system programming, std types including clock, timer, pthreadtypes, etc

#include <signal.h> // for signal handling: set, mask, wait, kill, handler etc

#include <time.h> // for creating & managing timers to operations on time, date, calendar.

#include <stdio.h> // std input output

#include <stdlib.h> // std lib for atoi, atof, atox, mem: malloc, calloc, free; abs(), rand(), exit(MACROS),etc

#include <errno.h> // platform specific error number

#include <fenv.h> // floating point exception handling, link with -lm