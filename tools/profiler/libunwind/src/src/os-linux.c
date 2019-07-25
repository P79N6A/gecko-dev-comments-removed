
























#include <limits.h>
#include <stdio.h>

#include "libunwind_i.h"
#include "os-linux.h"

PROTECTED int
tdep_get_elf_image (struct elf_image *ei, pid_t pid, unw_word_t ip,
		    unsigned long *segbase, unsigned long *mapoff,
		    char *path, size_t pathlen)
{
  struct map_iterator mi;
  int found = 0, rc;
  unsigned long hi;

  if (maps_init (&mi, pid) < 0)
    return -1;

  while (maps_next (&mi, segbase, &hi, mapoff))
    if (ip >= *segbase && ip < hi)
      {
	found = 1;
	break;
      }

  if (!found)
    {
      maps_close (&mi);
      return -1;
    }
  if (path)
    {
      strncpy(path, mi.path, pathlen);
    }
  rc = elf_map_image (ei, mi.path);
  maps_close (&mi);
  return rc;
}
