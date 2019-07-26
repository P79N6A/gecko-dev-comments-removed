






























#include <stdint.h>   

#include "utilities.h"   

#if !defined(OS_WINDOWS)
#include <unistd.h>
#include <sys/mman.h>
#endif

#include <stdio.h>  
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
#ifdef __i386__
  
  
  
  if ((uintptr_t)new_sp >= 0xffffe000) return NULL;
#endif
#if !defined(OS_WINDOWS)
  if (!STRICT_UNWINDING) {
    
    
    
    
    
    static int page_size = getpagesize();
    void *new_sp_aligned = (void *)((uintptr_t)new_sp & ~(page_size - 1));
    if (msync(new_sp_aligned, page_size, MS_ASYNC) == -1)
      return NULL;
  }
#endif
  return new_sp;
}


int GetStackTrace(void** result, int max_depth, int skip_count) {
  void **sp;
#ifdef __i386__
  
  
  
  
  
  sp = (void **)&result - 2;
#endif

#ifdef __x86_64__
  
  unsigned long rbp;
  
  
  
  
  
  
  
  __asm__ volatile ("mov %%rbp, %0" : "=r" (rbp));
  
  
  sp = (void **) rbp;
#endif

  int n = 0;
  while (sp && n < max_depth) {
    if (*(sp+1) == (void *)0) {
      
      
      break;
    }
    if (skip_count > 0) {
      skip_count--;
    } else {
      result[n++] = *(sp+1);
    }
    
    sp = NextStackFrame<true>(sp);
  }
  return n;
}

_END_GOOGLE_NAMESPACE_
