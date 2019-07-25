
























#include <dlfcn.h>
#include <string.h>
#include <unistd.h>

#include "libunwind_i.h"

#include "elf64.h"

HIDDEN int
tdep_get_elf_image (struct elf_image *ei, pid_t pid, unw_word_t ip,
		    unsigned long *segbase, unsigned long *mapoff,
		    char *path, size_t pathlen)
{
  struct load_module_desc lmd;
  const char *path2;

  if (pid != getpid ())
    {
      printf ("%s: remote case not implemented yet\n", __FUNCTION__);
      return -UNW_ENOINFO;
    }

  if (!dlmodinfo (ip, &lmd, sizeof (lmd), NULL, 0, 0))
    return -UNW_ENOINFO;

  *segbase = lmd.text_base;
  *mapoff = 0;			

  path2 = dlgetname (&lmd, sizeof (lmd), NULL, 0, 0);
  if (!path2)
    return -UNW_ENOINFO;
  if (path)
    {
      strncpy(path, path2, pathlen);
      path[pathlen - 1] = '\0';
      if (strcmp(path, path2) != 0)
        Debug(1, "buffer size (%d) not big enough to hold path\n", pathlen);
    }
  Debug(1, "segbase=%lx, mapoff=%lx, path=%s\n", *segbase, *mapoff, path);

  return elf_map_image (ei, path);
}
