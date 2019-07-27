







#ifndef SANDBOX_LINUX_SERVICES_LINUX_SYSCALLS_H_
#define SANDBOX_LINUX_SERVICES_LINUX_SYSCALLS_H_

#if defined(__x86_64__)
#include "sandbox/linux/services/x86_64_linux_syscalls.h"
#endif

#if defined(__i386__)
#include "sandbox/linux/services/x86_32_linux_syscalls.h"
#endif

#if defined(__arm__) && defined(__ARM_EABI__)
#include "sandbox/linux/services/arm_linux_syscalls.h"
#endif

#endif  

