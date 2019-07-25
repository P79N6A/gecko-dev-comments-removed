





























#ifndef libunwind_i_h
#define libunwind_i_h

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE___THREAD
  


# undef HAVE___THREAD
#endif



#include <sys/types.h>	

#include <assert.h>
#include <libunwind.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <elf.h>

#if defined(HAVE_ENDIAN_H)
# include <endian.h>
#elif defined(HAVE_SYS_ENDIAN_H)
# include <sys/endian.h>
#else
# define __LITTLE_ENDIAN	1234
# define __BIG_ENDIAN		4321
# if defined(__hpux)
#   define __BYTE_ORDER __BIG_ENDIAN
# else
#   error Host has unknown byte-order.
# endif
#endif

#ifdef __GNUC__
# define UNUSED		__attribute__((unused))
# define NORETURN	__attribute__((noreturn))
# define ALIAS(name)	__attribute__((alias (#name)))
# if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ > 2)
#  define ALWAYS_INLINE	inline __attribute__((always_inline))
#  define HIDDEN	__attribute__((visibility ("hidden")))
#  define PROTECTED	__attribute__((visibility ("protected")))
# else
#  define ALWAYS_INLINE
#  define HIDDEN
#  define PROTECTED
# endif
# if (__GNUC__ >= 3)
#  define likely(x)	__builtin_expect ((x), 1)
#  define unlikely(x)	__builtin_expect ((x), 0)
# else
#  define likely(x)	(x)
#  define unlikely(x)	(x)
# endif
#else
# define ALWAYS_INLINE
# define UNUSED
# define NORETURN
# define ALIAS(name)
# define HIDDEN
# define PROTECTED
# define likely(x)	(x)
# define unlikely(x)	(x)
#endif

#ifdef DEBUG
# define UNW_DEBUG	1
#else
# define UNW_DEBUG	0
#endif

#define ARRAY_SIZE(a)	(sizeof (a) / sizeof ((a)[0]))






#pragma weak pthread_mutex_init
#pragma weak pthread_mutex_lock
#pragma weak pthread_mutex_unlock

#define mutex_init(l)							\
	(pthread_mutex_init != 0 ? pthread_mutex_init ((l), 0) : 0)
#define mutex_lock(l)							\
	(pthread_mutex_lock != 0 ? pthread_mutex_lock (l) : 0)
#define mutex_unlock(l)							\
	(pthread_mutex_unlock != 0 ? pthread_mutex_unlock (l) : 0)

#ifdef HAVE_ATOMIC_OPS_H
# include <atomic_ops.h>
static inline int
cmpxchg_ptr (void *addr, void *old, void *new)
{
  union
    {
      void *vp;
      AO_t *aop;
    }
  u;

  u.vp = addr;
  return AO_compare_and_swap(u.aop, (AO_t) old, (AO_t) new);
}
# define fetch_and_add1(_ptr)		AO_fetch_and_add1(_ptr)
   
#  if !(defined(__hpux) && __GNUC__ == 3 && __GNUC_MINOR__ == 2)
#   define HAVE_CMPXCHG
#  endif
# define HAVE_FETCH_AND_ADD1
#else
# ifdef HAVE_IA64INTRIN_H
#  include <ia64intrin.h>
static inline int
cmpxchg_ptr (void *addr, void *old, void *new)
{
  union
    {
      void *vp;
      long *vlp;
    }
  u;

  u.vp = addr;
  return __sync_bool_compare_and_swap(u.vlp, (long) old, (long) new);
}
#  define fetch_and_add1(_ptr)		__sync_fetch_and_add(_ptr, 1)
#  define HAVE_CMPXCHG
#  define HAVE_FETCH_AND_ADD1
# endif
#endif
#define atomic_read(ptr)	(*(ptr))

#define UNWI_OBJ(fn)	  UNW_PASTE(UNW_PREFIX,UNW_PASTE(I,fn))
#define UNWI_ARCH_OBJ(fn) UNW_PASTE(UNW_PASTE(UNW_PASTE(_UI,UNW_TARGET),_), fn)

#define unwi_full_mask    UNWI_ARCH_OBJ(full_mask)





typedef sigset_t intrmask_t;

extern intrmask_t unwi_full_mask;



static inline void mark_as_used(void *v) {
}

#if defined(CONFIG_BLOCK_SIGNALS)
# define SIGPROCMASK(how, new_mask, old_mask) \
  sigprocmask((how), (new_mask), (old_mask))
#else
# define SIGPROCMASK(how, new_mask, old_mask) mark_as_used(old_mask)
#endif

#define define_lock(name) \
  pthread_mutex_t name = PTHREAD_MUTEX_INITIALIZER
#define lock_init(l)		mutex_init (l)
#define lock_acquire(l,m)				\
do {							\
  SIGPROCMASK (SIG_SETMASK, &unwi_full_mask, &(m));	\
  mutex_lock (l);					\
} while (0)
#define lock_release(l,m)			\
do {						\
  mutex_unlock (l);				\
  SIGPROCMASK (SIG_SETMASK, &(m), NULL);	\
} while (0)

#define SOS_MEMORY_SIZE 16384	/* see src/mi/mempool.c */

#ifndef MAP_ANONYMOUS
# define MAP_ANONYMOUS MAP_ANON
#endif
#define GET_MEMORY(mem, size)				    		    \
do {									    \
  /* Hopefully, mmap() goes straight through to a system call stub...  */   \
  mem = mmap (0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, \
	      -1, 0);							    \
  if (mem == MAP_FAILED)						    \
    mem = NULL;								    \
} while (0)

#define unwi_find_dynamic_proc_info	UNWI_OBJ(find_dynamic_proc_info)
#define unwi_extract_dynamic_proc_info	UNWI_OBJ(extract_dynamic_proc_info)
#define unwi_put_dynamic_unwind_info	UNWI_OBJ(put_dynamic_unwind_info)
#define unwi_dyn_remote_find_proc_info	UNWI_OBJ(dyn_remote_find_proc_info)
#define unwi_dyn_remote_put_unwind_info	UNWI_OBJ(dyn_remote_put_unwind_info)
#define unwi_dyn_validate_cache		UNWI_OBJ(dyn_validate_cache)

extern int unwi_find_dynamic_proc_info (unw_addr_space_t as,
					unw_word_t ip,
					unw_proc_info_t *pi,
					int need_unwind_info, void *arg);
extern int unwi_extract_dynamic_proc_info (unw_addr_space_t as,
					   unw_word_t ip,
					   unw_proc_info_t *pi,
					   unw_dyn_info_t *di,
					   int need_unwind_info,
					   void *arg);
extern void unwi_put_dynamic_unwind_info (unw_addr_space_t as,
					  unw_proc_info_t *pi, void *arg);




extern int unwi_dyn_remote_find_proc_info (unw_addr_space_t as,
					   unw_word_t ip,
					   unw_proc_info_t *pi,
					   int need_unwind_info,
					   void *arg);
extern void unwi_dyn_remote_put_unwind_info (unw_addr_space_t as,
					     unw_proc_info_t *pi,
					     void *arg);
extern int unwi_dyn_validate_cache (unw_addr_space_t as, void *arg);

extern unw_dyn_info_list_t _U_dyn_info_list;
extern pthread_mutex_t _U_dyn_info_list_lock;

#if UNW_DEBUG
#define unwi_debug_level		UNWI_ARCH_OBJ(debug_level)
extern long unwi_debug_level;

# include <stdio.h>

#ifdef HAVE_ANDROID_LOG_H
#include <android/log.h>
# define Debug(level,format...)						\
do {									\
  if (unwi_debug_level >= level)					\
    {									\
      int _n = level;							\
      if (_n > 16)							\
	_n = 16;							\
      __android_log_print(ANDROID_LOG_INFO, "libunwind", format);	\
    }									\
} while (0)

#define Dprintf(format...)
#else
# define Debug(level,format...)						\
do {									\
  if (unwi_debug_level >= level)					\
    {									\
      int _n = level;							\
      if (_n > 16)							\
	_n = 16;							\
      fprintf (stderr, "%*c>%s: ", _n, ' ', __FUNCTION__);		\
      fprintf (stderr, format);						\
    }									\
} while (0)
# define Dprintf(format...) 	    fprintf (stderr, format)
#endif
# ifdef __GNUC__
#  undef inline
#  define inline	UNUSED
# endif
#else
# define Debug(level,format...)
# define Dprintf(format...)
#endif

static ALWAYS_INLINE int
print_error (const char *string)
{
  return write (2, string, strlen (string));
}

#define mi_init		UNWI_ARCH_OBJ(mi_init)

extern void mi_init (void);	
extern unw_word_t _U_dyn_info_list_addr (void);



struct elf_image
  {
    void *image;		
    size_t size;		
  };





#define ACCESS_MEM_FAST(ret,validate,cur,addr,to) \
  do { (ret) = dwarf_get ((cur), DWARF_MEM_LOC ((cur), (addr)), &(to)); } \
  while (0)



#ifndef PT_GNU_EH_FRAME
# define PT_GNU_EH_FRAME	0x6474e550
#endif 
#ifndef PT_ARM_EXIDX
# define PT_ARM_EXIDX		0x70000001	/* ARM unwind segment */
#endif 

#include "tdep/libunwind_i.h"

#ifndef tdep_get_func_addr
# define tdep_get_func_addr(as,addr,v)		(*(v) = addr, 0)
#endif

#endif 
