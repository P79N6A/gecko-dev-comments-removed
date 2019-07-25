
























#include "_UPT_internal.h"

#if UNW_TARGET_IA64 && defined(__linux)
# include "elf64.h"
# include "os-linux.h"

static inline int
get_list_addr (unw_addr_space_t as, unw_word_t *dil_addr, void *arg,
	       int *countp)
{
  unsigned long lo, hi, off;
  struct UPT_info *ui = arg;
  struct map_iterator mi;
  char path[PATH_MAX];
  unw_word_t res;
  int count = 0;

  maps_init (&mi, ui->pid);
  while (maps_next (&mi, &lo, &hi, &off))
    {
      if (off)
	continue;

      if (ui->ei.image)
	{
	  munmap (ui->ei.image, ui->ei.size);
	  ui->ei.image = NULL;
	  ui->ei.size = 0;
	  
	  ui->di_cache.start_ip = ui->di_cache.end_ip = 0;
	}

      if (elf_map_image (&ui->ei, path) < 0)
	
	continue;

      Debug (16, "checking object %s\n", path);

      if (_UPTi_find_unwind_table (ui, as, path, lo, off, 0) > 0)
	{
	  res = _Uia64_find_dyn_list (as, &ui->di_cache, arg);
	  if (res && count++ == 0)
	    {
	      Debug (12, "dyn_info_list_addr = 0x%lx\n", (long) res);
	      *dil_addr = res;
	    }
	}
    }
  maps_close (&mi);
  *countp = count;
  return 0;
}

#else

static inline int
get_list_addr (unw_addr_space_t as, unw_word_t *dil_addr, void *arg,
	       int *countp)
{
# warning Implement get_list_addr(), please.
  *countp = 0;
  return 0;
}

#endif

int
_UPT_get_dyn_info_list_addr (unw_addr_space_t as, unw_word_t *dil_addr,
			     void *arg)
{
  int count, ret;

  Debug (12, "looking for dyn_info list\n");

  if ((ret = get_list_addr (as, dil_addr, arg, &count)) < 0)
    return ret;

  





  assert (count <= 1);

  return (count > 0) ? 0 : -UNW_ENOINFO;
}
