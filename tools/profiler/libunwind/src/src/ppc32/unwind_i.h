


























#ifndef unwind_i_h
#define unwind_i_h

#include <memory.h>
#include <stdint.h>

#include <libunwind-ppc32.h>

#include <libunwind_i.h>
#include <sys/ucontext.h>

#define ppc32_lock			UNW_OBJ(lock)
#define ppc32_local_resume		UNW_OBJ(local_resume)
#define ppc32_local_addr_space_init	UNW_OBJ(local_addr_space_init)

extern void ppc32_local_addr_space_init (void);
extern int ppc32_local_resume (unw_addr_space_t as, unw_cursor_t *cursor,
			     void *arg);

#endif 
