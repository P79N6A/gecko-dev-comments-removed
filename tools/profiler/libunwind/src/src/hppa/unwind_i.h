
























#ifndef unwind_i_h
#define unwind_i_h

#include <memory.h>
#include <stdint.h>

#include <libunwind-hppa.h>

#include "libunwind_i.h"

#define hppa_lock			UNW_OBJ(lock)
#define hppa_local_resume		UNW_OBJ(local_resume)
#define hppa_local_addr_space_init	UNW_OBJ(local_addr_space_init)
#define hppa_scratch_loc		UNW_OBJ(scratch_loc)
#define setcontext			UNW_ARCH_OBJ (setcontext)

extern void hppa_local_addr_space_init (void);
extern int hppa_local_resume (unw_addr_space_t as, unw_cursor_t *cursor,
			      void *arg);
extern dwarf_loc_t hppa_scratch_loc (struct cursor *c, unw_regnum_t reg);
extern int setcontext (const ucontext_t *ucp);

#endif 
