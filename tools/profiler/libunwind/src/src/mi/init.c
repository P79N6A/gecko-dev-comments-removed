
























#include "libunwind_i.h"

HIDDEN intrmask_t unwi_full_mask;

static const char rcsid[] UNUSED =
  "$Id: " PACKAGE_STRING " --- report bugs to " PACKAGE_BUGREPORT " $";

#if UNW_DEBUG





long unwi_debug_level;

#endif 

HIDDEN void
mi_init (void)
{
#if UNW_DEBUG
  const char *str = getenv ("UNW_DEBUG_LEVEL");

  if (str)
    unwi_debug_level = atoi (str);

  if (unwi_debug_level > 0)
    {
      setbuf (stdout, NULL);
      setbuf (stderr, NULL);
    }
#endif

  assert (sizeof (struct cursor) <= sizeof (unw_cursor_t));
}
