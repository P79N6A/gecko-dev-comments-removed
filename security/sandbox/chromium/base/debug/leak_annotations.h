



#ifndef BASE_DEBUG_LEAK_ANNOTATIONS_H_
#define BASE_DEBUG_LEAK_ANNOTATIONS_H_

#include "base/basictypes.h"
#include "build/build_config.h"












#if defined(LEAK_SANITIZER) && !defined(OS_NACL)


extern "C" {
void __lsan_disable();
void __lsan_enable();
void __lsan_ignore_object(const void *p);


void __lsan_do_leak_check();
}  

class ScopedLeakSanitizerDisabler {
 public:
  ScopedLeakSanitizerDisabler() { __lsan_disable(); }
  ~ScopedLeakSanitizerDisabler() { __lsan_enable(); }
 private:
  DISALLOW_COPY_AND_ASSIGN(ScopedLeakSanitizerDisabler);
};

#define ANNOTATE_SCOPED_MEMORY_LEAK \
    ScopedLeakSanitizerDisabler leak_sanitizer_disabler; static_cast<void>(0)

#define ANNOTATE_LEAKING_OBJECT_PTR(X) __lsan_ignore_object(X);

#else


#define ANNOTATE_SCOPED_MEMORY_LEAK ((void)0)
#define ANNOTATE_LEAKING_OBJECT_PTR(X) ((void)0)

#endif

#endif  
