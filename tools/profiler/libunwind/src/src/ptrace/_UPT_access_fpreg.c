

























#include "_UPT_internal.h"

#if HAVE_DECL_PTRACE_POKEUSER || HAVE_TTRACE
int
_UPT_access_fpreg (unw_addr_space_t as, unw_regnum_t reg, unw_fpreg_t *val,
		   int write, void *arg)
{
  unw_word_t *wp = (unw_word_t *) val;
  struct UPT_info *ui = arg;
  pid_t pid = ui->pid;
  int i;

  if ((unsigned) reg >= sizeof (_UPT_reg_offset) / sizeof (_UPT_reg_offset[0]))
    return -UNW_EBADREG;

  errno = 0;
  if (write)
    for (i = 0; i < (int) (sizeof (*val) / sizeof (wp[i])); ++i)
      {
#ifdef HAVE_TTRACE
#	warning No support for ttrace() yet.
#else
	ptrace (PTRACE_POKEUSER, pid, _UPT_reg_offset[reg] + i * sizeof(wp[i]),
		wp[i]);
#endif
	if (errno)
	  return -UNW_EBADREG;
      }
  else
    for (i = 0; i < (int) (sizeof (*val) / sizeof (wp[i])); ++i)
      {
#ifdef HAVE_TTRACE
#	warning No support for ttrace() yet.
#else
	wp[i] = ptrace (PTRACE_PEEKUSER, pid,
			_UPT_reg_offset[reg] + i * sizeof(wp[i]), 0);
#endif
	if (errno)
	  return -UNW_EBADREG;
      }
  return 0;
}
#elif HAVE_DECL_PT_GETFPREGS
int
_UPT_access_fpreg (unw_addr_space_t as, unw_regnum_t reg, unw_fpreg_t *val,
		   int write, void *arg)
{
  struct UPT_info *ui = arg;
  pid_t pid = ui->pid;
  fpregset_t fpreg;

  if ((unsigned) reg >= sizeof (_UPT_reg_offset) / sizeof (_UPT_reg_offset[0]))
    return -UNW_EBADREG;

  if (ptrace(PT_GETFPREGS, pid, (caddr_t)&fpreg, 0) == -1)
	  return -UNW_EBADREG;
  if (write) {
#if defined(__amd64__)
	  memcpy(&fpreg.fpr_xacc[reg], val, sizeof(unw_fpreg_t));
#elif defined(__i386__)
	  memcpy(&fpreg.fpr_acc[reg], val, sizeof(unw_fpreg_t));
#else
#error Fix me
#endif
	  if (ptrace(PT_SETFPREGS, pid, (caddr_t)&fpreg, 0) == -1)
		  return -UNW_EBADREG;
  } else
#if defined(__amd64__)
	  memcpy(val, &fpreg.fpr_xacc[reg], sizeof(unw_fpreg_t));
#elif defined(__i386__)
	  memcpy(val, &fpreg.fpr_acc[reg], sizeof(unw_fpreg_t));
#else
#error Fix me
#endif
  return 0;
}
#else
#error Fix me
#endif
