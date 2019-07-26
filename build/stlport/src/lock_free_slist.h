

















#ifndef _STLP_LOCK_FREE_SLIST_H
#define _STLP_LOCK_FREE_SLIST_H

#if defined(_STLP_PTHREADS)
#  include <pthread.h>

#  if defined (__GNUC__) && defined (__i386__)

#    define _STLP_HAS_ATOMIC_FREELIST






class _STLP_atomic_freelist {
public:
  


  struct item {
    item* _M_next;
  };

  _STLP_atomic_freelist() {
    
    _STLP_STATIC_ASSERT(sizeof(_M) == 8)
    _M._M_data._M_top       = 0;
    _M._M_data._M_sequence  = 0;
  }

  




  void push(item* __item) {
    
    
    
    
    
    
    
    
    
    
    int __tmp1;     
    int __tmp2;     
    int __tmp3;     
    __asm__ __volatile__
      ("       movl       %%ebx, %%edi\n\t"
       "       movl       %%ecx, %%ebx\n\t"
       "L1_%=: movl       %%eax, (%%ebx)\n\t"     
       "       leal       1(%%edx),%%ecx\n\t"     
       "lock;  cmpxchg8b  (%%esi)\n\t"
       "       jne        L1_%=\n\t"              
       "       movl       %%edi, %%ebx"
      :"=a" (__tmp1), "=d" (__tmp2), "=c" (__tmp3)
      :"a" (_M._M_data._M_top), "d" (_M._M_data._M_sequence), "c" (__item), "S" (&_M._M_data)
      :"edi", "memory", "cc");
  }

  





  item* pop() {
    item*   __result;
    int     __tmp;
    __asm__ __volatile__
      ("       movl       %%ebx, %%edi\n\t"
       "L1_%=: testl      %%eax, %%eax\n\t"       
       "       je         L2_%=\n\t"              
       "       movl       (%%eax), %%ebx\n\t"     
       "       leal       1(%%edx),%%ecx\n\t"     
       "lock;  cmpxchg8b  (%%esi)\n\t"
       "       jne        L1_%=\n\t"              
       "L2_%=: movl       %%edi, %%ebx"
      :"=a" (__result), "=d" (__tmp)
      :"a" (_M._M_data._M_top), "d" (_M._M_data._M_sequence), "S" (&_M._M_data)
      :"edi", "ecx", "memory", "cc");
    return __result;
  }

  






  item* clear() {
    item*   __result;
    int     __tmp;
    __asm__ __volatile__
      ("       movl       %%ebx, %%edi\n\t"
       "L1_%=: testl      %%eax, %%eax\n\t"       
       "       je         L2_%=\n\t"              
       "       xorl       %%ebx, %%ebx\n\t"       
       "       leal       1(%%edx),%%ecx\n\t"     
       "lock;  cmpxchg8b  (%%esi)\n\t"
       "       jne        L1_%=\n\t"              
       "L2_%=: movl       %%edi, %%ebx"
      :"=a" (__result), "=d" (__tmp)
      :"a" (_M._M_data._M_top), "d" (_M._M_data._M_sequence), "S" (&_M._M_data)
      :"edi", "ecx", "memory", "cc");
    return __result;
  }

private:
    union {
      long long   _M_align;
      struct {
        item*           _M_top;         
        unsigned int    _M_sequence;    
      } _M_data;
    } _M;

  _STLP_atomic_freelist(const _STLP_atomic_freelist&);
  _STLP_atomic_freelist& operator=(const _STLP_atomic_freelist&);
};

#  endif 

#elif defined (_STLP_WIN32THREADS)

#  if !defined (_WIN64)
#    define _STLP_USE_ASM_IMPLEMENTATION
#  endif



#  if defined (_STLP_USE_ASM_IMPLEMENTATION)

#    if defined (_STLP_MSVC) && defined (_M_IX86) && (_M_IX86 >= 500)
#      define _STLP_HAS_ATOMIC_FREELIST
#    endif
#  else

#    if defined (_STLP_NEW_PLATFORM_SDK) && (!defined (WINVER) || (WINVER >= 0x0501)) && \
                                            (!defined (_WIN32_WINNT) || (_WIN32_WINNT >= 0x0501))
#      define _STLP_HAS_ATOMIC_FREELIST
#    endif
#  endif

#  if defined (_STLP_HAS_ATOMIC_FREELIST)
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
#      if defined (_STLP_MSVC) && (_STLP_MSVC < 1300) || defined (__ICL)
#        pragma warning (push)
#        pragma warning (disable : 4035) //function has no return value
#      endif
#    endif






class _STLP_atomic_freelist {
public:
  


#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
  struct item {
      item*   _M_next;
  };
#    else
  typedef SLIST_ENTRY item;
#    endif

  _STLP_atomic_freelist() {
    
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
    _STLP_STATIC_ASSERT((sizeof(item) == sizeof(item*)) && (sizeof(_M) == 8))
    _M._M_data._M_top       = 0;
    _M._M_data._M_sequence  = 0;
#    else
    InitializeSListHead(&_M_head);
#    endif
  }

  




  void push(item* __item) {
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
    __asm
    {
        mov             esi, this
        mov             ebx, __item
        mov             eax, [esi]              
        mov             edx, [esi+4]            
    L1: mov             [ebx], eax              
        lea             ecx, [edx+1]            
        lock cmpxchg8b  qword ptr [esi]
        jne             L1                      
    }
#    else
    InterlockedPushEntrySList(&_M_head, __item);
#    endif
  }

  





  item* pop() {
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
    __asm
    {
        mov             esi, this
        mov             eax, [esi]              
        mov             edx, [esi+4]            
    L1: test            eax, eax                
        je              L2                      
        mov             ebx, [eax]              
        lea             ecx, [edx+1]            
        lock cmpxchg8b  qword ptr [esi]
        jne             L1                      
    L2:
    }
#    else
    return InterlockedPopEntrySList(&_M_head);
#    endif
  }

  






  item* clear() {
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
    __asm
    {
        mov             esi, this
        mov             eax, [esi]              
        mov             edx, [esi+4]            
    L1: test            eax, eax                
        je              L2                      
        xor             ebx,ebx                 
        lea             ecx, [edx+1]            
        lock cmpxchg8b  qword ptr [esi]
        jne             L1                      
    L2:
    }
#    else
    return InterlockedFlushSList(&_M_head);
#    endif
  }

private:
#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
  union {
    __int64     _M_align;
    struct {
      item*           _M_top;         
      unsigned int    _M_sequence;    
    } _M_data;
  } _M;
#    else
  SLIST_HEADER _M_head;
#    endif

  _STLP_atomic_freelist(const _STLP_atomic_freelist&);
  _STLP_atomic_freelist& operator = (const _STLP_atomic_freelist&);
};

#    if defined (_STLP_USE_ASM_IMPLEMENTATION)
#      if defined (_STLP_MSVC) && (_STLP_MSVC < 1300) || defined (__ICL)
#        pragma warning (pop)
#      endif
#    endif

#  endif 

#endif

#endif 
