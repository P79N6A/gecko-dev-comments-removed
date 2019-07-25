

























#include "_UPT_internal.h"

#if HAVE_DECL_PTRACE_POKEDATA || HAVE_TTRACE
int
_UPT_access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *val,
		 int write, void *arg)
{
  struct UPT_info *ui = arg;
  pid_t pid = ui->pid;

  errno = 0;
  if (write)
    {
      Debug (16, "mem[%lx] <- %lx\n", (long) addr, (long) *val);
#ifdef HAVE_TTRACE
#	warning No support for ttrace() yet.
#else
      ptrace (PTRACE_POKEDATA, pid, addr, *val);
      if (errno)
	return -UNW_EINVAL;
#endif
    }
  else
    {
#ifdef HAVE_TTRACE
#	warning No support for ttrace() yet.
#else
      *val = ptrace (PTRACE_PEEKDATA, pid, addr, 0);
      if (errno)
	return -UNW_EINVAL;
#endif
      Debug (16, "mem[%lx] -> %lx\n", (long) addr, (long) *val);
    }
  return 0;
}
#elif HAVE_DECL_PT_IO
int
_UPT_access_mem (unw_addr_space_t as, unw_word_t addr, unw_word_t *val,
		 int write, void *arg)
{
  struct UPT_info *ui = arg;
  pid_t pid = ui->pid;
  struct ptrace_io_desc iod;

  iod.piod_offs = (void *)addr;
  iod.piod_addr = val;
  iod.piod_len = sizeof(*val);
  iod.piod_op = write ? PIOD_WRITE_D : PIOD_READ_D;
  if (write)
    Debug (16, "mem[%lx] <- %lx\n", (long) addr, (long) *val);
  if (ptrace(PT_IO, pid, (caddr_t)&iod, 0) == -1)
    return -UNW_EINVAL;
  if (!write)
     Debug (16, "mem[%lx] -> %lx\n", (long) addr, (long) *val);
  return 0;
}
#else
#error Fix me
#endif
