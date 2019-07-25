


























#ifndef X86_64_LIBUNWIND_I_H
#define X86_64_LIBUNWIND_I_H




#include <stdlib.h>
#include <libunwind.h>

#include "elf64.h"
#include "mempool.h"
#include "dwarf.h"

typedef enum
  {
    UNW_X86_64_FRAME_STANDARD = -2,     
    UNW_X86_64_FRAME_SIGRETURN = -1,    
    UNW_X86_64_FRAME_OTHER = 0,         
    UNW_X86_64_FRAME_GUESSED = 1        
  }
unw_tdep_frame_type_t;

typedef struct
  {
    uint64_t virtual_address;
    int64_t frame_type     : 2;  
    int64_t last_frame     : 1;  
    int64_t cfa_reg_rsp    : 1;  
    int64_t cfa_reg_offset : 30; 
    int64_t rbp_cfa_offset : 15; 
    int64_t rsp_cfa_offset : 15; 
  }
unw_tdep_frame_t;

struct unw_addr_space
  {
    struct unw_accessors acc;
    unw_caching_policy_t caching_policy;
#ifdef HAVE_ATOMIC_OPS_H
    AO_t cache_generation;
#else
    uint32_t cache_generation;
#endif
    unw_word_t dyn_generation;		
    unw_word_t dyn_info_list_addr;	
    struct dwarf_rs_cache global_cache;
    struct unw_debug_frame_list *debug_frames;
   };

struct cursor
  {
    struct dwarf_cursor dwarf;		

    unw_tdep_frame_t frame_info;	

    

    enum
      {
	X86_64_SCF_NONE,		
	X86_64_SCF_LINUX_RT_SIGFRAME,	
	X86_64_SCF_FREEBSD_SIGFRAME,	
	X86_64_SCF_FREEBSD_SYSCALL,	
      }
    sigcontext_format;
    unw_word_t sigcontext_addr;
    int validate;
    ucontext_t *uc;
  };

static inline ucontext_t *
dwarf_get_uc(const struct dwarf_cursor *cursor)
{
  const struct cursor *c = (struct cursor *) cursor->as_arg;
  return c->uc;
}

#define DWARF_GET_LOC(l)	((l).val)

#ifdef UNW_LOCAL_ONLY
# define DWARF_NULL_LOC		DWARF_LOC (0, 0)
# define DWARF_IS_NULL_LOC(l)	(DWARF_GET_LOC (l) == 0)
# define DWARF_LOC(r, t)	((dwarf_loc_t) { .val = (r) })
# define DWARF_IS_REG_LOC(l)	0
# define DWARF_REG_LOC(c,r)	(DWARF_LOC((unw_word_t)			     \
				 x86_64_r_uc_addr(dwarf_get_uc(c), (r)), 0))
# define DWARF_MEM_LOC(c,m)	DWARF_LOC ((m), 0)
# define DWARF_FPREG_LOC(c,r)	(DWARF_LOC((unw_word_t)			     \
				 x86_64_r_uc_addr(dwarf_get_uc(c), (r)), 0))
#else 

# define DWARF_LOC_TYPE_FP	(1 << 0)
# define DWARF_LOC_TYPE_REG	(1 << 1)
# define DWARF_NULL_LOC		DWARF_LOC (0, 0)
# define DWARF_IS_NULL_LOC(l)						\
		({ dwarf_loc_t _l = (l); _l.val == 0 && _l.type == 0; })
# define DWARF_LOC(r, t)	((dwarf_loc_t) { .val = (r), .type = (t) })
# define DWARF_IS_REG_LOC(l)	(((l).type & DWARF_LOC_TYPE_REG) != 0)
# define DWARF_IS_FP_LOC(l)	(((l).type & DWARF_LOC_TYPE_FP) != 0)
# define DWARF_REG_LOC(c,r)	DWARF_LOC((r), DWARF_LOC_TYPE_REG)
# define DWARF_MEM_LOC(c,m)	DWARF_LOC ((m), 0)
# define DWARF_FPREG_LOC(c,r)	DWARF_LOC((r), (DWARF_LOC_TYPE_REG	\
						| DWARF_LOC_TYPE_FP))

#endif 

static inline int
dwarf_getfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t *val)
{
  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  abort ();
}

static inline int
dwarf_putfp (struct dwarf_cursor *c, dwarf_loc_t loc, unw_fpreg_t val)
{
  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  abort ();
}

static inline int
dwarf_get (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t *val)
{
  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg) (c->as, DWARF_GET_LOC (loc), val,
				     0, c->as_arg);
  else
    return (*c->as->acc.access_mem) (c->as, DWARF_GET_LOC (loc), val,
				     0, c->as_arg);
}

static inline int
dwarf_put (struct dwarf_cursor *c, dwarf_loc_t loc, unw_word_t val)
{
  if (DWARF_IS_NULL_LOC (loc))
    return -UNW_EBADREG;

  if (DWARF_IS_REG_LOC (loc))
    return (*c->as->acc.access_reg) (c->as, DWARF_GET_LOC (loc), &val,
				     1, c->as_arg);
  else
    return (*c->as->acc.access_mem) (c->as, DWARF_GET_LOC (loc), &val,
				     1, c->as_arg);
}

#define tdep_getcontext_trace	        UNW_ARCH_OBJ(getcontext_trace)
#define tdep_needs_initialization	UNW_OBJ(needs_initialization)
#define tdep_init_mem_validate		UNW_OBJ(init_mem_validate)
#define tdep_init			UNW_OBJ(init)


#define tdep_search_unwind_table	dwarf_search_unwind_table
#define tdep_get_elf_image		UNW_ARCH_OBJ(get_elf_image)
#define tdep_access_reg			UNW_OBJ(access_reg)
#define tdep_access_fpreg		UNW_OBJ(access_fpreg)
#if __linux__
# define tdep_fetch_frame		UNW_OBJ(fetch_frame)
# define tdep_cache_frame		UNW_OBJ(cache_frame)
# define tdep_reuse_frame		UNW_OBJ(reuse_frame)
#else
# define tdep_fetch_frame(c,ip,n)	do {} while(0)
# define tdep_cache_frame(c,rs)		do {} while(0)
# define tdep_reuse_frame(c,rs)		do {} while(0)
#endif
#define tdep_stash_frame		UNW_OBJ(stash_frame)
#define tdep_trace			UNW_OBJ(tdep_trace)
#define x86_64_r_uc_addr                UNW_OBJ(r_uc_addr)

#ifdef UNW_LOCAL_ONLY
# define tdep_find_proc_info(c,ip,n)				\
	dwarf_find_proc_info((c)->as, (ip), &(c)->pi, (n),	\
				       (c)->as_arg)
# define tdep_put_unwind_info(as,pi,arg)		\
	dwarf_put_unwind_info((as), (pi), (arg))
#else
# define tdep_find_proc_info(c,ip,n)					\
	(*(c)->as->acc.find_proc_info)((c)->as, (ip), &(c)->pi, (n),	\
				       (c)->as_arg)
# define tdep_put_unwind_info(as,pi,arg)			\
	(*(as)->acc.put_unwind_info)((as), (pi), (arg))
#endif

#define tdep_get_as(c)			((c)->dwarf.as)
#define tdep_get_as_arg(c)		((c)->dwarf.as_arg)
#define tdep_get_ip(c)			((c)->dwarf.ip)
#define tdep_big_endian(as)		0

extern int tdep_needs_initialization;

extern void tdep_init (void);
extern void tdep_init_mem_validate (void);
extern int tdep_search_unwind_table (unw_addr_space_t as, unw_word_t ip,
				     unw_dyn_info_t *di, unw_proc_info_t *pi,
				     int need_unwind_info, void *arg);
extern void *x86_64_r_uc_addr (ucontext_t *uc, int reg);
extern int tdep_get_elf_image (struct elf_image *ei, pid_t pid, unw_word_t ip,
			       unsigned long *segbase, unsigned long *mapoff,
			       char *path, size_t pathlen);
extern int tdep_access_reg (struct cursor *c, unw_regnum_t reg,
			    unw_word_t *valp, int write);
extern int tdep_access_fpreg (struct cursor *c, unw_regnum_t reg,
			      unw_fpreg_t *valp, int write);
#if __linux__
extern void tdep_fetch_frame (struct dwarf_cursor *c, unw_word_t ip,
			      int need_unwind_info);
extern void tdep_cache_frame (struct dwarf_cursor *c,
			      struct dwarf_reg_state *rs);
extern void tdep_reuse_frame (struct dwarf_cursor *c,
			      struct dwarf_reg_state *rs);
extern void tdep_stash_frame (struct dwarf_cursor *c,
			      struct dwarf_reg_state *rs);
#endif

extern int tdep_getcontext_trace (unw_tdep_context_t *);
extern int tdep_trace (unw_cursor_t *cursor, void **addresses, int *n);

#endif 
