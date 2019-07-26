



#ifndef BASE_CRITICAL_CLOSURE_H_
#define BASE_CRITICAL_CLOSURE_H_

#include "base/callback.h"

namespace base {














#if defined(OS_IOS)
base::Closure MakeCriticalClosure(const base::Closure& closure);
#else
inline base::Closure MakeCriticalClosure(const base::Closure& closure) {
  
  
  return closure;
}
#endif  

}  

#endif  
