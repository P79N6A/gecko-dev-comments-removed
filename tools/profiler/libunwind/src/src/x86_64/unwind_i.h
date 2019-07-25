


























#ifndef unwind_i_h
#define unwind_i_h

#include <memory.h>
#include <stdint.h>

#include <libunwind-x86_64.h>

#include "libunwind_i.h"
#include <sys/ucontext.h>


#define RAX	0
#define RDX	1
#define RCX	2
#define RBX	3
#define RSI	4
#define RDI	5
#define RBP	6
#define RSP	7
#define R8	8
#define R9	9
#define R10	10
#define R11	11
#define R12	12
#define R13	13
#define R14	14
#define R15	15
#define RIP	16

#define x86_64_lock			UNW_OBJ(lock)
#define x86_64_local_resume		UNW_OBJ(local_resume)
#define x86_64_local_addr_space_init	UNW_OBJ(local_addr_space_init)
#define setcontext			UNW_ARCH_OBJ (setcontext)
#if 0
#define x86_64_scratch_loc		UNW_OBJ(scratch_loc)
#endif
#define x86_64_r_uc_addr		UNW_OBJ(r_uc_addr)
#define x86_64_sigreturn		UNW_OBJ(sigreturn)


#ifdef UNW_LOCAL_ONLY
# undef ACCESS_MEM_FAST
# define ACCESS_MEM_FAST(ret,validate,cur,addr,to)                     \
  do {                                                                 \
    if (unlikely(validate))                                            \
      (ret) = dwarf_get ((cur), DWARF_MEM_LOC ((cur), (addr)), &(to)); \
    else                                                               \
      (ret) = 0, (to) = *(unw_word_t *)(addr);                         \
  } while (0)
#endif

extern void x86_64_local_addr_space_init (void);
extern int x86_64_local_resume (unw_addr_space_t as, unw_cursor_t *cursor,
			     void *arg);
extern int setcontext (const ucontext_t *ucp);

#if 0
extern dwarf_loc_t x86_64_scratch_loc (struct cursor *c, unw_regnum_t reg);
#endif

extern void *x86_64_r_uc_addr (ucontext_t *uc, int reg);
extern NORETURN void x86_64_sigreturn (unw_cursor_t *cursor);

#endif 
