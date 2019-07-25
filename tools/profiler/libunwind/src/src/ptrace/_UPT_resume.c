
























#include "_UPT_internal.h"

int
_UPT_resume (unw_addr_space_t as, unw_cursor_t *c, void *arg)
{
  struct UPT_info *ui = arg;

#ifdef HAVE_TTRACE
# warning No support for ttrace() yet.
#elif HAVE_DECL_PTRACE_CONT
  return ptrace (PTRACE_CONT, ui->pid, 0, 0);
#elif HAVE_DECL_PT_CONTINUE
  return ptrace(PT_CONTINUE, ui->pid, (caddr_t)1, 0);
#endif
}
