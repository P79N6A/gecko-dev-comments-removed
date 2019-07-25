


























#include <stdlib.h>
#include <string.h>

#include "ucontext_i.h"
#include "unwind_i.h"

#ifdef UNW_REMOTE_ONLY


PROTECTED unw_addr_space_t unw_local_addr_space;

#else 

static struct unw_addr_space local_addr_space;

PROTECTED unw_addr_space_t unw_local_addr_space = &local_addr_space;


#define PAGE_SIZE 4096
#define PAGE_START(a)	((a) & ~(PAGE_SIZE-1))


static void *
uc_addr (ucontext_t *uc, int reg)
{
  void *addr;

  if ((unsigned) (reg - UNW_PPC32_R0) < 32)
    addr = &uc->uc_mcontext.uc_regs->gregs[reg - UNW_PPC32_R0];

  else
  if ( ((unsigned) (reg - UNW_PPC32_F0) < 32) &&
       ((unsigned) (reg - UNW_PPC32_F0) >= 0) )
    addr = &uc->uc_mcontext.uc_regs->fpregs.fpregs[reg - UNW_PPC32_F0];

  else
    {
      unsigned gregs_idx;

      switch (reg)
	{
	case UNW_PPC32_CTR:
	  gregs_idx = CTR_IDX;
	  break;
	case UNW_PPC32_LR:
	  gregs_idx = LINK_IDX;
	  break;
	case UNW_PPC32_XER:
	  gregs_idx = XER_IDX;
	  break;
	case UNW_PPC32_CCR:
	  gregs_idx = CCR_IDX;
	  break;
	default:
	  return NULL;
	}
      addr = &uc->uc_mcontext.uc_regs->gregs[gregs_idx];
    }
  return addr;
}

# ifdef UNW_LOCAL_ONLY

HIDDEN void *
tdep_uc_addr (ucontext_t *uc, int reg)
{
  return uc_addr (uc, reg);
}

# endif	

HIDDEN unw_dyn_info_list_t _U_dyn_info_list;


static void
put_unwind_info (unw_addr_space_t as, unw_proc_info_t *proc_info, void *arg)
{
  
}

static int
get_dyn_info_list_addr (unw_addr_space_t as, unw_word_t *dyn_info_list_addr,
			void *arg)
{
  *dyn_info_list_addr = (unw_word_t) &_U_dyn_info_list;
  return 0;
}

static int
access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *val, int write,
	    void *arg)
{
  if (write)
    {
      Debug (12, "mem[%lx] <- %lx\n", addr, *val);
      *(unw_word_t *) addr = *val;
    }
  else
    {
      *val = *(unw_word_t *) addr;
      Debug (12, "mem[%lx] -> %lx\n", addr, *val);
    }
  return 0;
}

static int
access_reg (unw_addr_space_t as, unw_regnum_t reg, unw_word_t *val,
	    int write, void *arg)
{
  unw_word_t *addr;
  ucontext_t *uc = arg;

  if ( ((unsigned int) (reg - UNW_PPC32_F0) < 32) &&
       ((unsigned int) (reg - UNW_PPC32_F0) >= 0))
    goto badreg;

  addr = uc_addr (uc, reg);
  if (!addr)
    goto badreg;

  if (write)
    {
      *(unw_word_t *) addr = *val;
      Debug (12, "%s <- %lx\n", unw_regname (reg), *val);
    }
  else
    {
      *val = *(unw_word_t *) addr;
      Debug (12, "%s -> %lx\n", unw_regname (reg), *val);
    }
  return 0;

badreg:
  Debug (1, "bad register number %u\n", reg);
  return -UNW_EBADREG;
}

static int
access_fpreg (unw_addr_space_t as, unw_regnum_t reg, unw_fpreg_t *val,
	      int write, void *arg)
{
  ucontext_t *uc = arg;
  unw_fpreg_t *addr;

  if ((unsigned) (reg - UNW_PPC32_F0) < 0)
    goto badreg;

  addr = uc_addr (uc, reg);
  if (!addr)
    goto badreg;

  if (write)
    {
      Debug (12, "%s <- %016Lf\n", unw_regname (reg), *val);
      *(unw_fpreg_t *) addr = *val;
    }
  else
    {
      *val = *(unw_fpreg_t *) addr;
      Debug (12, "%s -> %016Lf\n", unw_regname (reg), *val);
    }
  return 0;

badreg:
  Debug (1, "bad register number %u\n", reg);
  
  return -UNW_EBADREG;
}

static int
get_static_proc_name (unw_addr_space_t as, unw_word_t ip,
		      char *buf, size_t buf_len, unw_word_t *offp,
		      void *arg)
{
  return _Uelf32_get_proc_name (as, getpid (), ip, buf, buf_len, offp);
}

HIDDEN void
ppc32_local_addr_space_init (void)
{
  memset (&local_addr_space, 0, sizeof (local_addr_space));
  local_addr_space.caching_policy = UNW_CACHE_GLOBAL;
  local_addr_space.acc.find_proc_info = dwarf_find_proc_info;
  local_addr_space.acc.put_unwind_info = put_unwind_info;
  local_addr_space.acc.get_dyn_info_list_addr = get_dyn_info_list_addr;
  local_addr_space.acc.access_mem = access_mem;
  local_addr_space.acc.access_reg = access_reg;
  local_addr_space.acc.access_fpreg = access_fpreg;
  local_addr_space.acc.resume = ppc32_local_resume;
  local_addr_space.acc.get_proc_name = get_static_proc_name;
  unw_flush_cache (&local_addr_space, 0, 0);
}

#endif 
