











#ifndef SANDBOX_LINUX_SERVICES_LINUX_SYSCALLS_H_
#define SANDBOX_LINUX_SERVICES_LINUX_SYSCALLS_H_

#if defined(__x86_64__)
#include "x86_64_linux_syscalls.h"
#endif

#if defined(__i386__)
#include "x86_32_linux_syscalls.h"
#endif

#if defined(__arm__) && defined(__ARM_EABI__)
#include "arm_linux_syscalls.h"
#endif

#endif  

