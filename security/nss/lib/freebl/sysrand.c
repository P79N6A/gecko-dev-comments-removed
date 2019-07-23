



































#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "seccomon.h"
#if defined(XP_UNIX) || defined(XP_BEOS)
#include "unix_rand.c"
#endif
#ifdef XP_WIN
#include "win_rand.c"
#endif
#ifdef XP_OS2
#include "os2_rand.c"
#endif
