























#include <stdlib.h>
#include <string.h>

#include "unwind_i.h"

#ifdef UNW_REMOTE_ONLY


PROTECTED unw_addr_space_t unw_local_addr_space;

#else 

static struct unw_addr_space local_addr_space;

PROTECTED unw_addr_space_t unw_local_addr_space = &local_addr_space;

static inline void *
uc_addr (unw_tdep_context_t *uc, int reg)
{
  if (reg >= UNW_ARM_R0 && reg < UNW_ARM_R0 + 16)
    return &uc->regs[reg - UNW_ARM_R0];
  else
    return NULL;
}

# ifdef UNW_LOCAL_ONLY

HIDDEN void *
tdep_uc_addr (unw_tdep_context_t *uc, int reg)
{
  return uc_addr (uc, reg);
}

# endif 

HIDDEN unw_dyn_info_list_t _U_dyn_info_list;






static int
get_dyn_info_list_addr (unw_addr_space_t as, unw_word_t *dyn_info_list_addr,
			void *arg)
{
  *dyn_info_list_addr = (unw_word_t) &_U_dyn_info_list;
  return 0;
}

static struct sigaction old_sigsegv_handler;
static volatile int sigsegv_protection = 0;
#define SIGSEGV_PROTECT 0x80000000
#define SIGSEGV_RAISED  0x00000001

#define PSR_J_BIT 0x01000000
#define PSR_T_BIT 0x00000020
#define PROCESSOR_MODE(x) (((x) & PSR_J_BIT) >> 23) | \
                          (((x) & PSR_T_BIT) >> 5)

static void
sigsegv_protect ()
{
  sigsegv_protection = SIGSEGV_PROTECT;
}

static int
sigsegv_raised ()
{
  int raised = (sigsegv_protection & SIGSEGV_RAISED) != 0;
  sigsegv_protection = 0;
  return raised;
}

static void
sigsegv_handler (int sig, siginfo_t* si, void* arg)
{
  if (sigsegv_protection & SIGSEGV_PROTECT) {
    sigsegv_protection |= SIGSEGV_RAISED;
    ucontext_t* uc = (ucontext_t*) arg;
    unw_word_t pc = uc->uc_mcontext.arm_pc;
    switch (PROCESSOR_MODE(uc->uc_mcontext.arm_cpsr)) {
      case 0: 
        pc += 4; 
        break;
      case 1: 
        pc += 2; 
        break;
      case 2: 
      case 3: 
        
        break;
    }
    
    uc->uc_mcontext.arm_pc = pc;
  } else {
    if ((old_sigsegv_handler.sa_flags & SA_SIGINFO) &&
        old_sigsegv_handler.sa_sigaction) {
      old_sigsegv_handler.sa_sigaction (sig, si, arg);
    } else if (old_sigsegv_handler.sa_handler) {
      old_sigsegv_handler.sa_handler (sig);
    }
  }
}

static void
install_sigsegv_handler ()
{
  struct sigaction sa;
  sa.sa_flags = SA_RESTART | SA_SIGINFO;
  sigemptyset (&sa.sa_mask);
  sa.sa_sigaction = sigsegv_handler;
  sigaction (SIGSEGV, &sa, &old_sigsegv_handler);
}

static int
access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *val, int write,
	    void *arg)
{
  int valid_access;

  if (write)
    {
      Debug (16, "mem[%x] <- %x\n", addr, *val);

      sigsegv_protect();

      *(unw_word_t *) addr = *val;

      valid_access = !sigsegv_raised();
    }
  else
    {
      sigsegv_protect();

      *val = *(unw_word_t *) addr;

      valid_access = !sigsegv_raised();

      Debug (16, "mem[%x] -> %x\n", addr, *val);
    }
  return valid_access ? 0 : -UNW_EINVAL;
}

static int
access_reg (unw_addr_space_t as, unw_regnum_t reg, unw_word_t *val, int write,
	    void *arg)
{
  unw_word_t *addr;
  unw_tdep_context_t *uc = arg;

  if (unw_is_fpreg (reg))
    goto badreg;

Debug (16, "reg = %s\n", unw_regname (reg));
  if (!(addr = uc_addr (uc, reg)))
    goto badreg;

  if (write)
    {
      *(unw_word_t *) addr = *val;
      Debug (12, "%s <- %x\n", unw_regname (reg), *val);
    }
  else
    {
      *val = *(unw_word_t *) addr;
      Debug (12, "%s -> %x\n", unw_regname (reg), *val);
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
  unw_tdep_context_t *uc = arg;
  unw_fpreg_t *addr;

  if (!unw_is_fpreg (reg))
    goto badreg;

  if (!(addr = uc_addr (uc, reg)))
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
  return _Uelf32_get_proc_name (as, getpid (), ip, buf, buf_len, offp);
}

HIDDEN void
arm_local_addr_space_init (void)
{
  memset (&local_addr_space, 0, sizeof (local_addr_space));
  local_addr_space.caching_policy = UNW_CACHE_GLOBAL;
  local_addr_space.acc.find_proc_info = arm_find_proc_info;
  local_addr_space.acc.put_unwind_info = arm_put_unwind_info;
  local_addr_space.acc.get_dyn_info_list_addr = get_dyn_info_list_addr;
  local_addr_space.acc.access_mem = access_mem;
  local_addr_space.acc.access_reg = access_reg;
  local_addr_space.acc.access_fpreg = access_fpreg;
  local_addr_space.acc.resume = arm_local_resume;
  local_addr_space.acc.get_proc_name = get_static_proc_name;
  unw_flush_cache (&local_addr_space, 0, 0);

  install_sigsegv_handler ();
}

#endif 
