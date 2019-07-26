



#ifndef SANDBOX_LINUX_SECCOMP_BPF_SYSCALL_H__
#define SANDBOX_LINUX_SECCOMP_BPF_SYSCALL_H__

#include <stdint.h>

#include "sandbox/linux/sandbox_export.h"

namespace sandbox {







SANDBOX_EXPORT intptr_t SandboxSyscall(int nr,
                                       intptr_t p0,
                                       intptr_t p1,
                                       intptr_t p2,
                                       intptr_t p3,
                                       intptr_t p4,
                                       intptr_t p5);
















#if __cplusplus >= 201103  

template <class T0 = intptr_t,
          class T1 = intptr_t,
          class T2 = intptr_t,
          class T3 = intptr_t,
          class T4 = intptr_t,
          class T5 = intptr_t>
SANDBOX_EXPORT inline intptr_t SandboxSyscall(int nr,
                                              T0 p0 = 0,
                                              T1 p1 = 0,
                                              T2 p2 = 0,
                                              T3 p3 = 0,
                                              T4 p4 = 0,
                                              T5 p5 = 0)
    __attribute__((always_inline));

template <class T0, class T1, class T2, class T3, class T4, class T5>
SANDBOX_EXPORT inline intptr_t
SandboxSyscall(int nr, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return SandboxSyscall(nr,
                        (intptr_t)p0,
                        (intptr_t)p1,
                        (intptr_t)p2,
                        (intptr_t)p3,
                        (intptr_t)p4,
                        (intptr_t)p5);
}

#else  





template <class T0, class T1, class T2, class T3, class T4, class T5>
SANDBOX_EXPORT inline intptr_t
    SandboxSyscall(int nr, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5)
    __attribute__((always_inline));
template <class T0, class T1, class T2, class T3, class T4, class T5>
SANDBOX_EXPORT inline intptr_t
SandboxSyscall(int nr, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
  return SandboxSyscall(nr,
                        (intptr_t)p0,
                        (intptr_t)p1,
                        (intptr_t)p2,
                        (intptr_t)p3,
                        (intptr_t)p4,
                        (intptr_t)p5);
}

template <class T0, class T1, class T2, class T3, class T4>
SANDBOX_EXPORT inline intptr_t
    SandboxSyscall(int nr, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4)
    __attribute__((always_inline));
template <class T0, class T1, class T2, class T3, class T4>
SANDBOX_EXPORT inline intptr_t
SandboxSyscall(int nr, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
  return SandboxSyscall(nr, p0, p1, p2, p3, p4, 0);
}

template <class T0, class T1, class T2, class T3>
SANDBOX_EXPORT inline intptr_t
    SandboxSyscall(int nr, T0 p0, T1 p1, T2 p2, T3 p3)
    __attribute__((always_inline));
template <class T0, class T1, class T2, class T3>
SANDBOX_EXPORT inline intptr_t
SandboxSyscall(int nr, T0 p0, T1 p1, T2 p2, T3 p3) {
  return SandboxSyscall(nr, p0, p1, p2, p3, 0, 0);
}

template <class T0, class T1, class T2>
SANDBOX_EXPORT inline intptr_t SandboxSyscall(int nr, T0 p0, T1 p1, T2 p2)
    __attribute__((always_inline));
template <class T0, class T1, class T2>
SANDBOX_EXPORT inline intptr_t SandboxSyscall(int nr, T0 p0, T1 p1, T2 p2) {
  return SandboxSyscall(nr, p0, p1, p2, 0, 0, 0);
}

template <class T0, class T1>
SANDBOX_EXPORT inline intptr_t SandboxSyscall(int nr, T0 p0, T1 p1)
    __attribute__((always_inline));
template <class T0, class T1>
SANDBOX_EXPORT inline intptr_t SandboxSyscall(int nr, T0 p0, T1 p1) {
  return SandboxSyscall(nr, p0, p1, 0, 0, 0, 0);
}

template <class T0>
SANDBOX_EXPORT inline intptr_t SandboxSyscall(int nr, T0 p0)
    __attribute__((always_inline));
template <class T0>
SANDBOX_EXPORT inline intptr_t SandboxSyscall(int nr, T0 p0) {
  return SandboxSyscall(nr, p0, 0, 0, 0, 0, 0);
}

SANDBOX_EXPORT inline intptr_t SandboxSyscall(int nr)
    __attribute__((always_inline));
SANDBOX_EXPORT inline intptr_t SandboxSyscall(int nr) {
  return SandboxSyscall(nr, 0, 0, 0, 0, 0, 0);
}

#endif  

}  

#endif  
