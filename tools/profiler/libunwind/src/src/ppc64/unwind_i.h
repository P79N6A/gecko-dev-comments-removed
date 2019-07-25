


























#ifndef unwind_i_h
#define unwind_i_h

#include <memory.h>
#include <stdint.h>

#include <libunwind-ppc64.h>

#include <libunwind_i.h>
#include <sys/ucontext.h>

#define ppc64_lock			UNW_OBJ(lock)
#define ppc64_local_resume		UNW_OBJ(local_resume)
#define ppc64_local_addr_space_init	UNW_OBJ(local_addr_space_init)
#if 0
#define ppc64_scratch_loc		UNW_OBJ(scratch_loc)
#endif

extern void ppc64_local_addr_space_init (void);
extern int ppc64_local_resume (unw_addr_space_t as, unw_cursor_t *cursor,
			     void *arg);
#if 0
extern dwarf_loc_t ppc64_scratch_loc (struct cursor *c, unw_regnum_t reg);
#endif

#endif 
