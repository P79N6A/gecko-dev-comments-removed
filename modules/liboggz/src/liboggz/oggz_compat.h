































#ifdef WIN32
#include "config_win32.h"
#else
#include "config.h"
#endif

#ifndef WIN32
#  define oggz_stat_regular(mode) (S_ISREG((mode)) || S_ISLNK((mode)))
#else
#  define oggz_stat_regular(mode) ((mode) & S_IFREG)
#endif
