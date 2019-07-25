

























#include "_UPT_internal.h"

int
_UPT_get_proc_name (unw_addr_space_t as, unw_word_t ip,
		    char *buf, size_t buf_len, unw_word_t *offp, void *arg)
{
  struct UPT_info *ui = arg;

#if ELF_CLASS == ELFCLASS64
  return _Uelf64_get_proc_name (as, ui->pid, ip, buf, buf_len, offp);
#elif ELF_CLASS == ELFCLASS32
  return _Uelf32_get_proc_name (as, ui->pid, ip, buf, buf_len, offp);
#else
  return -UNW_ENOINFO;
#endif
}
