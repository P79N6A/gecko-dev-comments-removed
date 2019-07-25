
























#include "_UPT_internal.h"

void
_UPT_destroy (void *ptr)
{
  struct UPT_info *ui = (struct UPT_info *) ptr;
  if (ui->ei.image)
    {
      munmap(ui->ei.image, ui->ei.size);
    }
 
  free (ptr);
}
