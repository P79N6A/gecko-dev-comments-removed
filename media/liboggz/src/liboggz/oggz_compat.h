































#include "config.h"

#ifndef WIN32
#  define oggz_stat_regular(mode) (S_ISREG((mode)) || S_ISLNK((mode)))
#else
#  define oggz_stat_regular(mode) ((mode) & S_IFREG)
#endif
