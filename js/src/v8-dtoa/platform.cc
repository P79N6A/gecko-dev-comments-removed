































#include <math.h>
#include <signal.h>
#include <stdlib.h>

#include "v8.h"

namespace v8 {
namespace internal {

double ceiling(double x) {
#if defined(__APPLE__)
  
  
  
  
  
  
  if (-1.0 < x && x < 0.0) {
    return -0.0;
  } else {
    return ceil(x);
  }
#else
  return ceil(x);
#endif
}




void OS::Abort() {
#if defined(WIN32)
    



    *((volatile int *) NULL) = 0;
    exit(3);
#elif defined(__APPLE__)
    



    *((volatile int *) NULL) = 0;  
    raise(SIGABRT);  
#else
    raise(SIGABRT);  
#endif
}

} }  





#ifdef _MSC_VER

#include <float.h>


int fpclassify(double x) {
  
  int flags = _fpclass(x);

  
  
  if (flags & (_FPCLASS_PN | _FPCLASS_NN)) return FP_NORMAL;
  if (flags & (_FPCLASS_PZ | _FPCLASS_NZ)) return FP_ZERO;
  if (flags & (_FPCLASS_PD | _FPCLASS_ND)) return FP_SUBNORMAL;
  if (flags & (_FPCLASS_PINF | _FPCLASS_NINF)) return FP_INFINITE;

  
  ASSERT(flags & (_FPCLASS_SNAN | _FPCLASS_QNAN));
  return FP_NAN;
}


#endif  

#ifdef SOLARIS

#include <ieeefp.h>


int fpclassify(double x) {

  fpclass_t rv = fpclass(x);

  switch (rv) {
    case FP_SNAN:
    case FP_QNAN: return FP_NAN;
    case FP_NINF:
    case FP_PINF: return FP_INFINITE;
    case FP_NDENORM:
    case FP_PDENORM: return FP_SUBNORMAL;
    case FP_NZERO:
    case FP_PZERO: return FP_ZERO;
    default:
    ASSERT(rv == FP_NNORM || rv == FP_PNORM);
    return FP_NORMAL;
  }

}
#endif 
