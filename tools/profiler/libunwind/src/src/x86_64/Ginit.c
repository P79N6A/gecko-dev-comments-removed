



























#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "unwind_i.h"

#ifdef UNW_REMOTE_ONLY


PROTECTED unw_addr_space_t unw_local_addr_space;

#else 

static struct unw_addr_space local_addr_space;

PROTECTED unw_addr_space_t unw_local_addr_space = &local_addr_space;

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

#define PAGE_SIZE 4096
#define PAGE_START(a)	((a) & ~(PAGE_SIZE-1))

static int (*mem_validate_func) (void *addr, size_t len);
static int msync_validate (void *addr, size_t len)
{
  return msync (addr, len, MS_ASYNC);
}

#ifdef HAVE_MINCORE
static int mincore_validate (void *addr, size_t len)
{
  unsigned char mvec[2]; 
  return mincore (addr, len, mvec);
}
#endif





HIDDEN void
tdep_init_mem_validate (void)
{
#ifdef HAVE_MINCORE
  unsigned char present = 1;
  if (mincore (&present, 1, &present) == 0)
    {
      Debug(1, "using mincore to validate memory\n");
      mem_validate_func = mincore_validate;
    }
  else
#endif
    {
      Debug(1, "using msync to validate memory\n");
      mem_validate_func = msync_validate;
    }
}


#define NLGA 4
static unw_word_t last_good_addr[NLGA];
static int lga_victim;

static int
validate_mem (unw_word_t addr)
{
  int i, victim;
  size_t len;

  if (PAGE_START(addr + sizeof (unw_word_t) - 1) == PAGE_START(addr))
    len = PAGE_SIZE;
  else
    len = PAGE_SIZE * 2;

  addr = PAGE_START(addr);

  if (addr == 0)
    return -1;

  for (i = 0; i < NLGA; i++)
    {
      if (last_good_addr[i] && (addr == last_good_addr[i]))
	return 0;
    }

  if (mem_validate_func ((void *) addr, len) == -1)
    return -1;

  victim = lga_victim;
  for (i = 0; i < NLGA; i++) {
    if (!last_good_addr[victim]) {
      last_good_addr[victim++] = addr;
      return 0;
    }
    victim = (victim + 1) % NLGA;
  }

  
  last_good_addr[victim] = addr;
  victim = (victim + 1) % NLGA;
  lga_victim = victim;

  return 0;
}

static int
access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *val, int write,
	    void *arg)
{
  if (unlikely (write))
    {
      Debug (16, "mem[%016lx] <- %lx\n", addr, *val);
      *(unw_word_t *) addr = *val;
    }
  else
    {
      
      const struct cursor *c = (const struct cursor *)arg;
      if (likely (c != 0) && unlikely (c->validate)
          && unlikely (validate_mem (addr)))
        return -1;
      *val = *(unw_word_t *) addr;
      Debug (16, "mem[%016lx] -> %lx\n", addr, *val);
    }
  return 0;
}

static int
access_reg (unw_addr_space_t as, unw_regnum_t reg, unw_word_t *val, int write,
	    void *arg)
{
  unw_word_t *addr;
  ucontext_t *uc = ((struct cursor *)arg)->uc;

  if (unw_is_fpreg (reg))
    goto badreg;

  if (!(addr = x86_64_r_uc_addr (uc, reg)))
    goto badreg;

  if (write)
    {
      *(unw_word_t *) addr = *val;
      Debug (12, "%s <- 0x%016lx\n", unw_regname (reg), *val);
    }
  else
    {
      *val = *(unw_word_t *) addr;
      Debug (12, "%s -> 0x%016lx\n", unw_regname (reg), *val);
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
  ucontext_t *uc = ((struct cursor *)arg)->uc;
  unw_fpreg_t *addr;

  if (!unw_is_fpreg (reg))
    goto badreg;

  if (!(addr = x86_64_r_uc_addr (uc, reg)))
    goto badreg;

  if (write)
    {
      Debug (12, "%s <- %08lx.%08lx.%08lx\n", unw_regname (reg),
	     ((long *)val)[0], ((long *)val)[1], ((long *)val)[2]);
      *(unw_fpreg_t *) addr = *val;
    }
  else
    {
      *val = *(unw_fpreg_t *) addr;
      Debug (12, "%s -> %08lx.%08lx.%08lx\n", unw_regname (reg),
	     ((long *)val)[0], ((long *)val)[1], ((long *)val)[2]);
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
  return _Uelf64_get_proc_name (as, getpid (), ip, buf, buf_len, offp);
}

HIDDEN void
x86_64_local_addr_space_init (void)
{
  memset (&local_addr_space, 0, sizeof (local_addr_space));
  local_addr_space.caching_policy = UNW_CACHE_GLOBAL;
  local_addr_space.acc.find_proc_info = dwarf_find_proc_info;
  local_addr_space.acc.put_unwind_info = put_unwind_info;
  local_addr_space.acc.get_dyn_info_list_addr = get_dyn_info_list_addr;
  local_addr_space.acc.access_mem = access_mem;
  local_addr_space.acc.access_reg = access_reg;
  local_addr_space.acc.access_fpreg = access_fpreg;
  local_addr_space.acc.resume = x86_64_local_resume;
  local_addr_space.acc.get_proc_name = get_static_proc_name;
  unw_flush_cache (&local_addr_space, 0, 0);

  memset (last_good_addr, 0, sizeof (unw_word_t) * NLGA);
  lga_victim = 0;
}

#endif 
