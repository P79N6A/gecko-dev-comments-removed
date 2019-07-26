





















#include "stlport_prefix.h"

#include <memory>

#if defined (__GNUC__) && (defined (__CYGWIN__) || defined (__MINGW32__))
#  include <malloc.h>
#endif

#if defined (_STLP_PTHREADS) && !defined (_STLP_NO_THREADS)
#  include <pthread_alloc>
#  include <cerrno>
#endif

#include <stl/_threads.h>

#include "lock_free_slist.h"

#if defined (__WATCOMC__)
#  pragma warning 13 9
#  pragma warning 367 9
#  pragma warning 368 9
#endif

#if defined (_STLP_SGI_THREADS)
  
  
  
  
extern "C" {
  extern int __us_rsthread_malloc;
}
#endif



#if defined (_STLP_MSVC) && (_STLP_MSVC >= 1020 && defined (_STLP_DEBUG_ALLOC)) && !defined (_STLP_WCE)
#  include <crtdbg.h>
inline char* __stlp_new_chunk(size_t __bytes) {
  void *__chunk = _STLP_CHECK_NULL_ALLOC(::operator new(__bytes, __FILE__, __LINE__));
  return __STATIC_CAST(char*, __chunk);
}
inline void __stlp_delete_chunck(void* __p) { ::operator delete(__p, __FILE__, __LINE__); }
#else
#  ifdef _STLP_NODE_ALLOC_USE_MALLOC
#    include <cstdlib>
inline char* __stlp_new_chunk(size_t __bytes) {
  
  void *__chunk = _STLP_VENDOR_CSTD::malloc(__bytes);
  if (__chunk == 0) {
    _STLP_THROW_BAD_ALLOC;
  }
  return __STATIC_CAST(char*, __chunk);
}
inline void __stlp_delete_chunck(void* __p) { _STLP_VENDOR_CSTD::free(__p); }
#  else
inline char* __stlp_new_chunk(size_t __bytes)
{ return __STATIC_CAST(char*, _STLP_STD::__stl_new(__bytes)); }
inline void __stlp_delete_chunck(void* __p) { _STLP_STD::__stl_delete(__p); }
#  endif
#endif





typedef long __add_atomic_t;
typedef unsigned long __uadd_atomic_t;

#if defined (__GNUC__) && defined (__i386__)
inline long _STLP_atomic_add_gcc_x86(long volatile* p, long addend) {
  long result;
  __asm__ __volatile__
    ("lock; xaddl %1, %0;"
    :"=m" (*p), "=r" (result)
    :"m"  (*p), "1"  (addend)
    :"cc");
 return result + addend;
}
#  define _STLP_ATOMIC_ADD(__dst, __val)  _STLP_atomic_add_gcc_x86(__dst, __val)
#elif defined (_STLP_WIN32THREADS)

#  if !defined (_STLP_WIN95_LIKE)
#    if defined (_STLP_NEW_PLATFORM_SDK)
#      define _STLP_ATOMIC_ADD(__dst, __val) InterlockedExchangeAdd(__dst, __val)
#    else
#      define _STLP_ATOMIC_ADD(__dst, __val) InterlockedExchangeAdd(__CONST_CAST(__add_atomic_t*, __dst), __val)
#    endif
#  endif
#endif

#if defined (__OS400__)

enum { _ALIGN = 16, _ALIGN_SHIFT = 4 };
#else
enum { _ALIGN = 2 * sizeof(void*), _ALIGN_SHIFT = 2 + sizeof(void*) / 4 };
#endif

#define _S_FREELIST_INDEX(__bytes) ((__bytes - size_t(1)) >> (int)_ALIGN_SHIFT)

_STLP_BEGIN_NAMESPACE


static __oom_handler_type __oom_handler = __STATIC_CAST(__oom_handler_type, 0);

#ifdef _STLP_THREADS
_STLP_mutex __oom_handler_lock;
#endif

void* _STLP_CALL __malloc_alloc::allocate(size_t __n)
{
  void *__result = malloc(__n);
  if ( 0 == __result ) {
    __oom_handler_type __my_malloc_handler;

    for (;;) {
      {
#ifdef _STLP_THREADS
        _STLP_auto_lock _l( __oom_handler_lock );
#endif
        __my_malloc_handler = __oom_handler;
      }
      if ( 0 == __my_malloc_handler) {
        _STLP_THROW_BAD_ALLOC;
      }
      (*__my_malloc_handler)();
      __result = malloc(__n);
      if ( __result )
        return __result;
    }
  }
  return __result;
}

__oom_handler_type _STLP_CALL __malloc_alloc::set_malloc_handler(__oom_handler_type __f)
{
#ifdef _STLP_THREADS
  _STLP_auto_lock _l( __oom_handler_lock );
#endif
  __oom_handler_type __old = __oom_handler;
  __oom_handler = __f;
  return __old;
}















#define _STLP_NFREELISTS 16

#if defined (_STLP_LEAKS_PEDANTIC) && defined (_STLP_USE_DYNAMIC_LIB)







#  define _STLP_DO_CLEAN_NODE_ALLOC
#endif






#if defined (_STLP_THREADS) && \
    defined (_STLP_HAS_ATOMIC_FREELIST) && defined (_STLP_ATOMIC_ADD)




#  define _STLP_USE_LOCK_FREE_IMPLEMENTATION
#endif

#if !defined (_STLP_USE_LOCK_FREE_IMPLEMENTATION)
#  if defined (_STLP_THREADS)

class _Node_Alloc_Lock {
  static _STLP_STATIC_MUTEX& _S_Mutex() {
    static _STLP_STATIC_MUTEX mutex _STLP_MUTEX_INITIALIZER;
    return mutex;
  }
public:
  _Node_Alloc_Lock() {
#    if defined (_STLP_SGI_THREADS)
    if (__us_rsthread_malloc)
#    endif
      _S_Mutex()._M_acquire_lock();
  }

  ~_Node_Alloc_Lock() {
#    if defined (_STLP_SGI_THREADS)
    if (__us_rsthread_malloc)
#    endif
      _S_Mutex()._M_release_lock();
  }
};

#  else

class _Node_Alloc_Lock {
public:
  _Node_Alloc_Lock() { }
  ~_Node_Alloc_Lock() { }
};

#  endif

struct _Node_alloc_obj {
  _Node_alloc_obj * _M_next;
};
#endif

class __node_alloc_impl {
  static inline size_t _STLP_CALL _S_round_up(size_t __bytes)
  { return (((__bytes) + (size_t)_ALIGN-1) & ~((size_t)_ALIGN - 1)); }

#if defined (_STLP_USE_LOCK_FREE_IMPLEMENTATION)
  typedef _STLP_atomic_freelist::item   _Obj;
  typedef _STLP_atomic_freelist         _Freelist;
  typedef _STLP_atomic_freelist         _ChunkList;

  
  
  struct _FreeBlockHeader : public _STLP_atomic_freelist::item {
    char* _M_end;     
  };
#else
  typedef _Node_alloc_obj       _Obj;
  typedef _Obj* _STLP_VOLATILE  _Freelist;
  typedef _Obj*                 _ChunkList;
#endif

private:
  
  static _Obj* _S_refill(size_t __n);
  
  
  static char* _S_chunk_alloc(size_t __p_size, int& __nobjs);
  
  static _Freelist _S_free_list[_STLP_NFREELISTS];
  
#if defined (_STLP_USE_LOCK_FREE_IMPLEMENTATION)
  static _STLP_VOLATILE __add_atomic_t _S_heap_size;
#else
  static size_t _S_heap_size;
#endif

#if defined (_STLP_USE_LOCK_FREE_IMPLEMENTATION)
  
  static _STLP_atomic_freelist  _S_free_mem_blocks;
#else
  
  static char* _S_start_free;
  
  static char* _S_end_free;
#endif

#if defined (_STLP_DO_CLEAN_NODE_ALLOC)
public:
  
#  if defined (_STLP_USE_LOCK_FREE_IMPLEMENTATION)
  typedef _STLP_VOLATILE __stl_atomic_t _AllocCounter;
#  else
  typedef __stl_atomic_t _AllocCounter;
#  endif
  static _AllocCounter& _STLP_CALL _S_alloc_counter();
  static void _S_alloc_call();
  static void _S_dealloc_call();

private:
  
  static void _S_chunk_dealloc();
  
  static _ChunkList _S_chunks;
#endif 

public:
  
  static void* _M_allocate(size_t& __n);
  
  static void _M_deallocate(void *__p, size_t __n);
};

#if !defined (_STLP_USE_LOCK_FREE_IMPLEMENTATION)
void* __node_alloc_impl::_M_allocate(size_t& __n) {
  __n = _S_round_up(__n);
  _Obj * _STLP_VOLATILE * __my_free_list = _S_free_list + _S_FREELIST_INDEX(__n);
  _Obj *__r;

  
  
  
  _Node_Alloc_Lock __lock_instance;

  if ( (__r  = *__my_free_list) != 0 ) {
    *__my_free_list = __r->_M_next;
  } else {
    __r = _S_refill(__n);
  }
#  if defined (_STLP_DO_CLEAN_NODE_ALLOC)
  _S_alloc_call();
#  endif
  
  return __r;
}

void __node_alloc_impl::_M_deallocate(void *__p, size_t __n) {
  _Obj * _STLP_VOLATILE * __my_free_list = _S_free_list + _S_FREELIST_INDEX(__n);
  _Obj * __pobj = __STATIC_CAST(_Obj*, __p);

  
  _Node_Alloc_Lock __lock_instance;
  __pobj->_M_next = *__my_free_list;
  *__my_free_list = __pobj;

#  if defined (_STLP_DO_CLEAN_NODE_ALLOC)
  _S_dealloc_call();
#  endif
  
}

#  if defined (_STLP_DO_CLEAN_NODE_ALLOC)
#    define _STLP_OFFSET sizeof(_Obj)
#  else
#    define _STLP_OFFSET 0
#  endif





char* __node_alloc_impl::_S_chunk_alloc(size_t _p_size, int& __nobjs) {
  char* __result;
  size_t __total_bytes = _p_size * __nobjs;
  size_t __bytes_left = _S_end_free - _S_start_free;

  if (__bytes_left > 0) {
    if (__bytes_left >= __total_bytes) {
      __result = _S_start_free;
      _S_start_free += __total_bytes;
      return __result;
    }

    if (__bytes_left >= _p_size) {
      __nobjs = (int)(__bytes_left / _p_size);
      __total_bytes = _p_size * __nobjs;
      __result = _S_start_free;
      _S_start_free += __total_bytes;
      return __result;
    }

    
    _Obj* _STLP_VOLATILE* __my_free_list = _S_free_list + _S_FREELIST_INDEX(__bytes_left);
    __REINTERPRET_CAST(_Obj*, _S_start_free)->_M_next = *__my_free_list;
    *__my_free_list = __REINTERPRET_CAST(_Obj*, _S_start_free);
    _S_start_free = _S_end_free = 0;
  }

  size_t __bytes_to_get = 2 * __total_bytes + _S_round_up(_S_heap_size) + _STLP_OFFSET;

  _STLP_TRY {
    _S_start_free = __stlp_new_chunk(__bytes_to_get);
  }
#if defined (_STLP_USE_EXCEPTIONS)
  catch (const _STLP_STD::bad_alloc&) {
    _Obj* _STLP_VOLATILE* __my_free_list;
    _Obj* __p;
    
    
    
    for (size_t __i = _p_size; __i <= (size_t)_MAX_BYTES; __i += (size_t)_ALIGN) {
      __my_free_list = _S_free_list + _S_FREELIST_INDEX(__i);
      __p = *__my_free_list;
      if (0 != __p) {
        *__my_free_list = __p -> _M_next;
        _S_start_free = __REINTERPRET_CAST(char*, __p);
        _S_end_free = _S_start_free + __i;
        return _S_chunk_alloc(_p_size, __nobjs);
        
        
      }
    }
    __bytes_to_get = __total_bytes + _STLP_OFFSET;
    _S_start_free = __stlp_new_chunk(__bytes_to_get);
  }
#endif

  _S_heap_size += __bytes_to_get >> 4;
#  if defined (_STLP_DO_CLEAN_NODE_ALLOC)
  __REINTERPRET_CAST(_Obj*, _S_start_free)->_M_next = _S_chunks;
  _S_chunks = __REINTERPRET_CAST(_Obj*, _S_start_free);
#  endif
  _S_end_free = _S_start_free + __bytes_to_get;
  _S_start_free += _STLP_OFFSET;
  return _S_chunk_alloc(_p_size, __nobjs);
}




_Node_alloc_obj* __node_alloc_impl::_S_refill(size_t __n) {
  int __nobjs = 20;
  char* __chunk = _S_chunk_alloc(__n, __nobjs);

  if (1 == __nobjs) return __REINTERPRET_CAST(_Obj*, __chunk);

  _Obj* _STLP_VOLATILE* __my_free_list = _S_free_list + _S_FREELIST_INDEX(__n);
  _Obj* __result;
  _Obj* __current_obj;
  _Obj* __next_obj;

  
  __result = __REINTERPRET_CAST(_Obj*, __chunk);
  *__my_free_list = __next_obj = __REINTERPRET_CAST(_Obj*, __chunk + __n);
  for (--__nobjs; --__nobjs; ) {
    __current_obj = __next_obj;
    __next_obj = __REINTERPRET_CAST(_Obj*, __REINTERPRET_CAST(char*, __next_obj) + __n);
    __current_obj->_M_next = __next_obj;
  }
  __next_obj->_M_next = 0;
  return __result;
}

#  if defined (_STLP_DO_CLEAN_NODE_ALLOC)
void __node_alloc_impl::_S_alloc_call()
{ ++_S_alloc_counter(); }

void __node_alloc_impl::_S_dealloc_call() {
  __stl_atomic_t &counter = _S_alloc_counter();
  if (--counter == 0)
  { _S_chunk_dealloc(); }
}


void __node_alloc_impl::_S_chunk_dealloc() {
  _Obj *__pcur = _S_chunks, *__pnext;
  while (__pcur != 0) {
    __pnext = __pcur->_M_next;
    __stlp_delete_chunck(__pcur);
    __pcur = __pnext;
  }
  _S_chunks = 0;
  _S_start_free = _S_end_free = 0;
  _S_heap_size = 0;
  memset(__REINTERPRET_CAST(char*, __CONST_CAST(_Obj**, &_S_free_list[0])), 0, _STLP_NFREELISTS * sizeof(_Obj*));
}
#  endif

#else

void* __node_alloc_impl::_M_allocate(size_t& __n) {
  __n = _S_round_up(__n);
  _Obj* __r = _S_free_list[_S_FREELIST_INDEX(__n)].pop();
  if (__r  == 0)
  { __r = _S_refill(__n); }

#  if defined (_STLP_DO_CLEAN_NODE_ALLOC)
  _S_alloc_call();
#  endif
  return __r;
}

void __node_alloc_impl::_M_deallocate(void *__p, size_t __n) {
  _S_free_list[_S_FREELIST_INDEX(__n)].push(__STATIC_CAST(_Obj*, __p));

#  if defined (_STLP_DO_CLEAN_NODE_ALLOC)
  _S_dealloc_call();
#  endif
}




__node_alloc_impl::_Obj* __node_alloc_impl::_S_refill(size_t __n) {
  int __nobjs = 20;
  char* __chunk = _S_chunk_alloc(__n, __nobjs);

  if (__nobjs <= 1)
    return __REINTERPRET_CAST(_Obj*, __chunk);

  
  _Obj* __result   = __REINTERPRET_CAST(_Obj*, __chunk);
  _Obj* __cur_item = __result;
  _Freelist* __my_freelist = _S_free_list + _S_FREELIST_INDEX(__n);
  for (--__nobjs; __nobjs != 0; --__nobjs) {
    __cur_item  = __REINTERPRET_CAST(_Obj*, __REINTERPRET_CAST(char*, __cur_item) + __n);
    __my_freelist->push(__cur_item);
  }
  return __result;
}

#  if defined (_STLP_DO_CLEAN_NODE_ALLOC)
#    define _STLP_OFFSET _ALIGN
#  else
#    define _STLP_OFFSET 0
#  endif




char* __node_alloc_impl::_S_chunk_alloc(size_t _p_size, int& __nobjs) {
#  if defined (_STLP_DO_CLEAN_NODE_ALLOC)
  
  
  
  _STLP_STATIC_ASSERT(sizeof(_Obj) <= _ALIGN)
#  endif
  char*  __result       = 0;
  __add_atomic_t __total_bytes  = __STATIC_CAST(__add_atomic_t, _p_size) * __nobjs;

  _FreeBlockHeader* __block = __STATIC_CAST(_FreeBlockHeader*, _S_free_mem_blocks.pop());
  if (__block != 0) {
    
    
    char*  __buf_start  = __REINTERPRET_CAST(char*, __block);
    __add_atomic_t __bytes_left = __block->_M_end - __buf_start;

    if ((__bytes_left < __total_bytes) && (__bytes_left >= __STATIC_CAST(__add_atomic_t, _p_size))) {
      
      __result      = __buf_start;
      __nobjs       = (int)(__bytes_left/_p_size);
      __total_bytes = __STATIC_CAST(__add_atomic_t, _p_size) * __nobjs;
      __bytes_left -= __total_bytes;
      __buf_start  += __total_bytes;
    }
    else if (__bytes_left >= __total_bytes) {
      
      __result      = __buf_start;
      __bytes_left -= __total_bytes;
      __buf_start  += __total_bytes;
    }

    if (__bytes_left != 0) {
      
      if ((__result != 0) && (__bytes_left >= (__add_atomic_t)sizeof(_FreeBlockHeader))) {
        
        
        _FreeBlockHeader* __newblock = __REINTERPRET_CAST(_FreeBlockHeader*, __buf_start);
        __newblock->_M_end  = __block->_M_end;
        _S_free_mem_blocks.push(__newblock);
      }
      else {
        
        
        size_t __rounded_down = _S_round_up(__bytes_left + 1) - (size_t)_ALIGN;
        if (__rounded_down > 0)
          _S_free_list[_S_FREELIST_INDEX(__rounded_down)].push((_Obj*)__buf_start);
      }
    }
    if (__result != 0)
      return __result;
  }

  
  __add_atomic_t __bytes_to_get = 2 * __total_bytes +
                                  __STATIC_CAST(__add_atomic_t,
                                                _S_round_up(__STATIC_CAST(__uadd_atomic_t, _STLP_ATOMIC_ADD(&_S_heap_size, 0)))) +
                                  _STLP_OFFSET;
  _STLP_TRY {
    __result = __stlp_new_chunk(__bytes_to_get);
  }
#if defined (_STLP_USE_EXCEPTIONS)
  catch (const bad_alloc&) {
    
    for (size_t __i = _p_size; __i <= (size_t)_MAX_BYTES; __i += (size_t)_ALIGN) {
      _Obj* __p  = _S_free_list[_S_FREELIST_INDEX(__i)].pop();
      if (0 != __p) {
        if (__i < sizeof(_FreeBlockHeader)) {
          
          
          __nobjs = (int)(__i/_p_size);
          __total_bytes = __nobjs * __STATIC_CAST(__add_atomic_t, _p_size);
          size_t __bytes_left = __i - __total_bytes;
          size_t __rounded_down = _S_round_up(__bytes_left+1) - (size_t)_ALIGN;
          if (__rounded_down > 0) {
            _S_free_list[_S_FREELIST_INDEX(__rounded_down)].push(__REINTERPRET_CAST(_Obj*, __REINTERPRET_CAST(char*, __p) + __total_bytes));
          }
          return __REINTERPRET_CAST(char*, __p);
        }
        else {
          
          _FreeBlockHeader* __newblock = (_FreeBlockHeader*)__p;
          __newblock->_M_end  = __REINTERPRET_CAST(char*, __p) + __i;
          _S_free_mem_blocks.push(__newblock);
          return _S_chunk_alloc(_p_size, __nobjs);
        }
      }
    }

    
    __bytes_to_get  = __total_bytes + _STLP_OFFSET;
    __result = __stlp_new_chunk(__bytes_to_get);

    
    
  }
#endif
  
  _STLP_VERBOSE_ASSERT(((__REINTERPRET_CAST(size_t, __result) & __STATIC_CAST(size_t, _ALIGN - 1)) == 0),
                       _StlMsg_DBA_DELETED_TWICE)
  _STLP_ATOMIC_ADD(&_S_heap_size, __bytes_to_get >> 4);

#  if defined (_STLP_DO_CLEAN_NODE_ALLOC)
  
  _S_chunks.push(__REINTERPRET_CAST(_Obj*, __result));
  __result       += _ALIGN;
  __bytes_to_get -= _ALIGN;
#  endif

  if (__bytes_to_get > __total_bytes) {
    
    _FreeBlockHeader* __freeblock = __REINTERPRET_CAST(_FreeBlockHeader*, __result + __total_bytes);
    __freeblock->_M_end  = __result + __bytes_to_get;
    _S_free_mem_blocks.push(__freeblock);
  }
  return __result;
}

#  if defined (_STLP_DO_CLEAN_NODE_ALLOC)
void __node_alloc_impl::_S_alloc_call()
{ _STLP_ATOMIC_INCREMENT(&_S_alloc_counter()); }

void __node_alloc_impl::_S_dealloc_call() {
  _STLP_VOLATILE __stl_atomic_t *pcounter = &_S_alloc_counter();
  if (_STLP_ATOMIC_DECREMENT(pcounter) == 0)
    _S_chunk_dealloc();
}


void __node_alloc_impl::_S_chunk_dealloc() {
  
  
  
  
  

  
  
  
  _S_free_mem_blocks.clear();
  _S_heap_size      = 0;

  for (size_t __i = 0; __i < _STLP_NFREELISTS; ++__i) {
    _S_free_list[__i].clear();
  }

  
  _Obj* __chunk = _S_chunks.clear();
  while (__chunk != 0) {
    _Obj* __next = __chunk->_M_next;
    __stlp_delete_chunck(__chunk);
    __chunk  = __next;
  }
}
#  endif

#endif

#if defined (_STLP_DO_CLEAN_NODE_ALLOC)
struct __node_alloc_cleaner {
  ~__node_alloc_cleaner()
  { __node_alloc_impl::_S_dealloc_call(); }
};

#  if defined (_STLP_USE_LOCK_FREE_IMPLEMENTATION)
_STLP_VOLATILE __stl_atomic_t& _STLP_CALL
#  else
__stl_atomic_t& _STLP_CALL
#  endif
__node_alloc_impl::_S_alloc_counter() {
  static _AllocCounter _S_counter = 1;
  static __node_alloc_cleaner _S_node_alloc_cleaner;
  return _S_counter;
}
#endif

#if !defined (_STLP_USE_LOCK_FREE_IMPLEMENTATION)
_Node_alloc_obj * _STLP_VOLATILE
__node_alloc_impl::_S_free_list[_STLP_NFREELISTS]
= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};



#else
_STLP_atomic_freelist __node_alloc_impl::_S_free_list[_STLP_NFREELISTS];
_STLP_atomic_freelist __node_alloc_impl::_S_free_mem_blocks;
#endif

#if !defined (_STLP_USE_LOCK_FREE_IMPLEMENTATION)
char *__node_alloc_impl::_S_start_free = 0;
char *__node_alloc_impl::_S_end_free = 0;
#endif

#if defined (_STLP_USE_LOCK_FREE_IMPLEMENTATION)
_STLP_VOLATILE __add_atomic_t
#else
size_t
#endif
__node_alloc_impl::_S_heap_size = 0;

#if defined (_STLP_DO_CLEAN_NODE_ALLOC)
#  if defined (_STLP_USE_LOCK_FREE_IMPLEMENTATION)
_STLP_atomic_freelist __node_alloc_impl::_S_chunks;
#  else
_Node_alloc_obj* __node_alloc_impl::_S_chunks  = 0;
#  endif
#endif

void * _STLP_CALL __node_alloc::_M_allocate(size_t& __n)
{ return __node_alloc_impl::_M_allocate(__n); }

void _STLP_CALL __node_alloc::_M_deallocate(void *__p, size_t __n)
{ __node_alloc_impl::_M_deallocate(__p, __n); }

#if defined (_STLP_PTHREADS) && !defined (_STLP_NO_THREADS)

#  define _STLP_DATA_ALIGNMENT 8

_STLP_MOVE_TO_PRIV_NAMESPACE



union _Pthread_alloc_obj {
  union _Pthread_alloc_obj * __free_list_link;
  char __client_data[_STLP_DATA_ALIGNMENT];    
};





struct _Pthread_alloc_per_thread_state {
  typedef _Pthread_alloc_obj __obj;
  enum { _S_NFREELISTS = _MAX_BYTES / _STLP_DATA_ALIGNMENT };

  
  
  
  
  
  _Pthread_alloc_per_thread_state() : __next(0)
  { memset((void *)__CONST_CAST(_Pthread_alloc_obj**, __free_list), 0, (size_t)_S_NFREELISTS * sizeof(__obj *)); }
  
  void *_M_refill(size_t __n);

  _Pthread_alloc_obj* volatile __free_list[_S_NFREELISTS];
  _Pthread_alloc_per_thread_state *__next;
  
  _STLP_mutex _M_lock;
};


class _Pthread_alloc_impl {
public: 
  typedef _Pthread_alloc_per_thread_state __state_type;
  typedef char value_type;

  
  
  static char *_S_chunk_alloc(size_t __size, size_t &__nobjs, __state_type*);

  enum {_S_ALIGN = _STLP_DATA_ALIGNMENT};

  static size_t _S_round_up(size_t __bytes)
  { return (((__bytes) + (int)_S_ALIGN - 1) & ~((int)_S_ALIGN - 1)); }
  static size_t _S_freelist_index(size_t __bytes)
  { return (((__bytes) + (int)_S_ALIGN - 1) / (int)_S_ALIGN - 1); }

private:
  
  
  static _STLP_STATIC_MUTEX _S_chunk_allocator_lock;
  static char *_S_start_free;
  static char *_S_end_free;
  static size_t _S_heap_size;
  static __state_type *_S_free_per_thread_states;
  static pthread_key_t _S_key;
  static bool _S_key_initialized;
  
  
  static void _S_destructor(void *instance);
  
  
  static __state_type *_S_new_per_thread_state();
public:
  
  static __state_type *_S_get_per_thread_state();
private:
        
        
  class _M_lock;
  friend class _M_lock;
  class _M_lock {
  public:
    _M_lock () { _S_chunk_allocator_lock._M_acquire_lock(); }
    ~_M_lock () { _S_chunk_allocator_lock._M_release_lock(); }
  };

public:

  
  static void * allocate(size_t& __n);

  
  static void deallocate(void *__p, size_t __n);

  
  
  static void * allocate(size_t& __n, __state_type* __a);

  
  static void deallocate(void *__p, size_t __n, __state_type* __a);

  static void * reallocate(void *__p, size_t __old_sz, size_t& __new_sz);
};




void *_Pthread_alloc_per_thread_state::_M_refill(size_t __n) {
  typedef _Pthread_alloc_obj __obj;
  size_t __nobjs = 128;
  char * __chunk = _Pthread_alloc_impl::_S_chunk_alloc(__n, __nobjs, this);
  __obj * volatile * __my_free_list;
  __obj * __result;
  __obj * __current_obj, * __next_obj;
  size_t __i;

  if (1 == __nobjs)  {
    return __chunk;
  }

  __my_free_list = __free_list + _Pthread_alloc_impl::_S_freelist_index(__n);

  
  __result = (__obj *)__chunk;
  *__my_free_list = __next_obj = (__obj *)(__chunk + __n);
  for (__i = 1; ; ++__i) {
    __current_obj = __next_obj;
    __next_obj = (__obj *)((char *)__next_obj + __n);
    if (__nobjs - 1 == __i) {
      __current_obj -> __free_list_link = 0;
      break;
    } else {
      __current_obj -> __free_list_link = __next_obj;
    }
  }
  return __result;
}

void _Pthread_alloc_impl::_S_destructor(void *__instance) {
  _M_lock __lock_instance;  
  _Pthread_alloc_per_thread_state* __s = (_Pthread_alloc_per_thread_state*)__instance;
  __s -> __next = _S_free_per_thread_states;
  _S_free_per_thread_states = __s;
}

_Pthread_alloc_per_thread_state* _Pthread_alloc_impl::_S_new_per_thread_state() {
  
  if (0 != _S_free_per_thread_states) {
    _Pthread_alloc_per_thread_state *__result = _S_free_per_thread_states;
    _S_free_per_thread_states = _S_free_per_thread_states -> __next;
    return __result;
  }
  else {
    return new _Pthread_alloc_per_thread_state;
  }
}

_Pthread_alloc_per_thread_state* _Pthread_alloc_impl::_S_get_per_thread_state() {
  int __ret_code;
  __state_type* __result;

  if (_S_key_initialized && (__result = (__state_type*) pthread_getspecific(_S_key)))
    return __result;

  
  _M_lock __lock_instance;  
  if (!_S_key_initialized) {
    if (pthread_key_create(&_S_key, _S_destructor)) {
      _STLP_THROW_BAD_ALLOC;  
    }
    _S_key_initialized = true;
  }

  __result = _S_new_per_thread_state();
  __ret_code = pthread_setspecific(_S_key, __result);
  if (__ret_code) {
    if (__ret_code == ENOMEM) {
      _STLP_THROW_BAD_ALLOC;
    } else {
  
      _STLP_ABORT();
    }
  }
  return __result;
}




char *_Pthread_alloc_impl::_S_chunk_alloc(size_t __p_size, size_t &__nobjs, _Pthread_alloc_per_thread_state *__a) {
  typedef _Pthread_alloc_obj __obj;
  {
    char * __result;
    size_t __total_bytes;
    size_t __bytes_left;
    
    _M_lock __lock_instance;         

    __total_bytes = __p_size * __nobjs;
    __bytes_left = _S_end_free - _S_start_free;
    if (__bytes_left >= __total_bytes) {
      __result = _S_start_free;
      _S_start_free += __total_bytes;
      return __result;
    } else if (__bytes_left >= __p_size) {
      __nobjs = __bytes_left/__p_size;
      __total_bytes = __p_size * __nobjs;
      __result = _S_start_free;
      _S_start_free += __total_bytes;
      return __result;
    } else {
      size_t __bytes_to_get = 2 * __total_bytes + _S_round_up(_S_heap_size);
      
      if (__bytes_left > 0) {
        __obj * volatile * __my_free_list = __a->__free_list + _S_freelist_index(__bytes_left);
        ((__obj *)_S_start_free) -> __free_list_link = *__my_free_list;
        *__my_free_list = (__obj *)_S_start_free;
      }
#  ifdef _SGI_SOURCE
      
      
      
      
      {
        const int __cache_line_size = 128;  
        __bytes_to_get &= ~(__cache_line_size-1);
        _S_start_free = (char *)memalign(__cache_line_size, __bytes_to_get);
        if (0 == _S_start_free) {
          _S_start_free = (char *)__malloc_alloc::allocate(__bytes_to_get);
        }
      }
#  else  
      _S_start_free = (char *)__malloc_alloc::allocate(__bytes_to_get);
#  endif
      _S_heap_size += __bytes_to_get >> 4;
      _S_end_free = _S_start_free + __bytes_to_get;
    }
  }
  
  return _S_chunk_alloc(__p_size, __nobjs, __a);
}



void *_Pthread_alloc_impl::allocate(size_t& __n) {
  typedef _Pthread_alloc_obj __obj;
  __obj * volatile * __my_free_list;
  __obj * __result;
  __state_type* __a;

  if (__n > _MAX_BYTES) {
    return __malloc_alloc::allocate(__n);
  }

  __n = _S_round_up(__n);
  __a = _S_get_per_thread_state();

  __my_free_list = __a->__free_list + _S_freelist_index(__n);
  __result = *__my_free_list;
  if (__result == 0) {
    void *__r = __a->_M_refill(__n);
    return __r;
  }
  *__my_free_list = __result->__free_list_link;
  return __result;
};


void _Pthread_alloc_impl::deallocate(void *__p, size_t __n) {
  typedef _Pthread_alloc_obj __obj;
  __obj *__q = (__obj *)__p;
  __obj * volatile * __my_free_list;
  __state_type* __a;

  if (__n > _MAX_BYTES) {
      __malloc_alloc::deallocate(__p, __n);
      return;
  }

  __a = _S_get_per_thread_state();

  __my_free_list = __a->__free_list + _S_freelist_index(__n);
  __q -> __free_list_link = *__my_free_list;
  *__my_free_list = __q;
}



void *_Pthread_alloc_impl::allocate(size_t& __n, __state_type* __a) {
  typedef _Pthread_alloc_obj __obj;
  __obj * volatile * __my_free_list;
  __obj * __result;

  if (__n > _MAX_BYTES) {
    return __malloc_alloc::allocate(__n);
  }
  __n = _S_round_up(__n);

  
  
  _STLP_auto_lock __lock(__a->_M_lock);

  __my_free_list = __a->__free_list + _S_freelist_index(__n);
  __result = *__my_free_list;
  if (__result == 0) {
    void *__r = __a->_M_refill(__n);
    return __r;
  }
  *__my_free_list = __result->__free_list_link;
  return __result;
};


void _Pthread_alloc_impl::deallocate(void *__p, size_t __n, __state_type* __a) {
  typedef _Pthread_alloc_obj __obj;
  __obj *__q = (__obj *)__p;
  __obj * volatile * __my_free_list;

  if (__n > _MAX_BYTES) {
    __malloc_alloc::deallocate(__p, __n);
    return;
  }

  
  
  _STLP_auto_lock __lock(__a->_M_lock);

  __my_free_list = __a->__free_list + _S_freelist_index(__n);
  __q -> __free_list_link = *__my_free_list;
  *__my_free_list = __q;
}

void *_Pthread_alloc_impl::reallocate(void *__p, size_t __old_sz, size_t& __new_sz) {
  void * __result;
  size_t __copy_sz;

  if (__old_sz > _MAX_BYTES && __new_sz > _MAX_BYTES) {
    return realloc(__p, __new_sz);
  }

  if (_S_round_up(__old_sz) == _S_round_up(__new_sz)) return __p;
  __result = allocate(__new_sz);
  __copy_sz = __new_sz > __old_sz? __old_sz : __new_sz;
  memcpy(__result, __p, __copy_sz);
  deallocate(__p, __old_sz);
  return __result;
}

_Pthread_alloc_per_thread_state* _Pthread_alloc_impl::_S_free_per_thread_states = 0;
pthread_key_t _Pthread_alloc_impl::_S_key = 0;
_STLP_STATIC_MUTEX _Pthread_alloc_impl::_S_chunk_allocator_lock _STLP_MUTEX_INITIALIZER;
bool _Pthread_alloc_impl::_S_key_initialized = false;
char *_Pthread_alloc_impl::_S_start_free = 0;
char *_Pthread_alloc_impl::_S_end_free = 0;
size_t _Pthread_alloc_impl::_S_heap_size = 0;

void * _STLP_CALL _Pthread_alloc::allocate(size_t& __n)
{ return _Pthread_alloc_impl::allocate(__n); }
void _STLP_CALL _Pthread_alloc::deallocate(void *__p, size_t __n)
{ _Pthread_alloc_impl::deallocate(__p, __n); }
void * _STLP_CALL _Pthread_alloc::allocate(size_t& __n, __state_type* __a)
{ return _Pthread_alloc_impl::allocate(__n, __a); }
void _STLP_CALL _Pthread_alloc::deallocate(void *__p, size_t __n, __state_type* __a)
{ _Pthread_alloc_impl::deallocate(__p, __n, __a); }
void * _STLP_CALL _Pthread_alloc::reallocate(void *__p, size_t __old_sz, size_t& __new_sz)
{ return _Pthread_alloc_impl::reallocate(__p, __old_sz, __new_sz); }
_Pthread_alloc_per_thread_state* _STLP_CALL _Pthread_alloc::_S_get_per_thread_state()
{ return _Pthread_alloc_impl::_S_get_per_thread_state(); }

_STLP_MOVE_TO_STD_NAMESPACE

#endif

_STLP_END_NAMESPACE

#undef _S_FREELIST_INDEX
