



#ifndef SANDBOX_LINUX_SECCOMP_BPF_SYSCALL_H__
#define SANDBOX_LINUX_SECCOMP_BPF_SYSCALL_H__

#include <stdint.h>

#include "base/macros.h"
#include "sandbox/sandbox_export.h"

namespace sandbox {



class SANDBOX_EXPORT Syscall {
 public:
  
  
  
  
  
  
  
  
  
  
  
  
  template <class T0,
            class T1,
            class T2,
            class T3,
            class T4,
            class T5,
            class T6,
            class T7>
  static inline intptr_t
  Call(int nr, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6, T7 p7) {
    return Call(nr,
                (intptr_t)p0,
                (intptr_t)p1,
                (intptr_t)p2,
                (intptr_t)p3,
                (intptr_t)p4,
                (intptr_t)p5,
                (intptr_t)p6,
                (intptr_t)p7);
  }

  template <class T0,
            class T1,
            class T2,
            class T3,
            class T4,
            class T5,
            class T6>
  static inline intptr_t
  Call(int nr, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5, T6 p6) {
    return Call(nr,
                (intptr_t)p0,
                (intptr_t)p1,
                (intptr_t)p2,
                (intptr_t)p3,
                (intptr_t)p4,
                (intptr_t)p5,
                (intptr_t)p6,
                0);
  }

  template <class T0, class T1, class T2, class T3, class T4, class T5>
  static inline intptr_t
  Call(int nr, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4, T5 p5) {
    return Call(nr,
                (intptr_t)p0,
                (intptr_t)p1,
                (intptr_t)p2,
                (intptr_t)p3,
                (intptr_t)p4,
                (intptr_t)p5,
                0,
                0);
  }

  template <class T0, class T1, class T2, class T3, class T4>
  static inline intptr_t Call(int nr, T0 p0, T1 p1, T2 p2, T3 p3, T4 p4) {
    return Call(nr, p0, p1, p2, p3, p4, 0, 0, 0);
  }

  template <class T0, class T1, class T2, class T3>
  static inline intptr_t Call(int nr, T0 p0, T1 p1, T2 p2, T3 p3) {
    return Call(nr, p0, p1, p2, p3, 0, 0, 0, 0);
  }

  template <class T0, class T1, class T2>
  static inline intptr_t Call(int nr, T0 p0, T1 p1, T2 p2) {
    return Call(nr, p0, p1, p2, 0, 0, 0, 0, 0);
  }

  template <class T0, class T1>
  static inline intptr_t Call(int nr, T0 p0, T1 p1) {
    return Call(nr, p0, p1, 0, 0, 0, 0, 0, 0);
  }

  template <class T0>
  static inline intptr_t Call(int nr, T0 p0) {
    return Call(nr, p0, 0, 0, 0, 0, 0, 0, 0);
  }

  static inline intptr_t Call(int nr) {
    return Call(nr, 0, 0, 0, 0, 0, 0, 0, 0);
  }

 private:
  
  
  
  
  
  
  static intptr_t Call(int nr,
                       intptr_t p0,
                       intptr_t p1,
                       intptr_t p2,
                       intptr_t p3,
                       intptr_t p4,
                       intptr_t p5,
                       intptr_t p6,
                       intptr_t p7);

  DISALLOW_IMPLICIT_CONSTRUCTORS(Syscall);
};

}  

#endif  
