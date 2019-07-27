





#ifndef mozilla_LinuxSched_h
#define mozilla_LinuxSched_h

#include <linux/sched.h>





#ifndef CLONE_NEWUTS
#define CLONE_NEWUTS  0x04000000
#endif
#ifndef CLONE_NEWIPC
#define CLONE_NEWIPC  0x08000000
#endif
#ifndef CLONE_NEWUSER
#define CLONE_NEWUSER 0x10000000
#endif
#ifndef CLONE_NEWPID
#define CLONE_NEWPID  0x20000000
#endif
#ifndef CLONE_NEWNET
#define CLONE_NEWNET  0x40000000
#endif
#ifndef CLONE_IO
#define CLONE_IO      0x80000000
#endif

#endif
