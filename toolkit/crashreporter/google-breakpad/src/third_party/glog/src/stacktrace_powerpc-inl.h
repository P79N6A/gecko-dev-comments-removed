




































#include <stdio.h>
#include <stdint.h>   
#include "stacktrace.h"

_START_GOOGLE_NAMESPACE_





template<bool STRICT_UNWINDING>
static void **NextStackFrame(void **old_sp) {
  void **new_sp = (void **) *old_sp;

  
  
  if (STRICT_UNWINDING) {
    
    
    if (new_sp <= old_sp) return NULL;
    
    if ((uintptr_t)new_sp - (uintptr_t)old_sp > 100000) return NULL;
  } else {
    
    
    if (new_sp == old_sp) return NULL;
    
    if ((new_sp > old_sp)
        && ((uintptr_t)new_sp - (uintptr_t)old_sp > 1000000)) return NULL;
  }
  if ((uintptr_t)new_sp & (sizeof(void *) - 1)) return NULL;
  return new_sp;
}


void StacktracePowerPCDummyFunction() __attribute__((noinline));
void StacktracePowerPCDummyFunction() { __asm__ volatile(""); }


int GetStackTrace(void** result, int max_depth, int skip_count) {
  void **sp;
  
  
  
  
#ifdef __APPLE__
  __asm__ volatile ("mr %0,r1" : "=r" (sp));
#else
  __asm__ volatile ("mr %0,1" : "=r" (sp));
#endif

  
  
  
  
  
  
  
  StacktracePowerPCDummyFunction();

  
  skip_count++;

  int n = 0;
  while (sp && n < max_depth) {
    if (skip_count > 0) {
      skip_count--;
    } else {
      
      
      
      
#if defined(_CALL_AIX) || defined(_CALL_DARWIN)
      result[n++] = *(sp+2);
#elif defined(_CALL_SYSV)
      result[n++] = *(sp+1);
#elif defined(__APPLE__) || (defined(__linux) && defined(__PPC64__))
      
      result[n++] = *(sp+2);
#elif defined(__linux)
      
      result[n++] = *(sp+1);
#else
#error Need to specify the PPC ABI for your archiecture.
#endif
    }
    
    sp = NextStackFrame<true>(sp);
  }
  return n;
}

_END_GOOGLE_NAMESPACE_
