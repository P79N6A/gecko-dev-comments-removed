
























#include <string.h>

#include "_UPT_internal.h"

void *
_UPT_create (pid_t pid)
{
  struct UPT_info *ui = malloc (sizeof (struct UPT_info));

  if (!ui)
    return NULL;

  memset (ui, 0, sizeof (*ui));
  ui->pid = pid;
  ui->di_cache.format = -1;
  ui->di_debug.format = -1;
#if UNW_TARGET_IA64
  ui->ktab.format = -1;;
#endif
  return ui;
}
