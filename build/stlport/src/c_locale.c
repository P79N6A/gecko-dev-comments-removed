

















#include "stlport_prefix.h"

#include "c_locale.h"

#if defined (_STLP_WIN32) && !defined (_STLP_WCE)
#  include "c_locale_win32/c_locale_win32.c"
#elif defined (_STLP_USE_GLIBC2_LOCALIZATION)
#  include "c_locale_glibc/c_locale_glibc2.c" 
#else
#  include "c_locale_dummy/c_locale_dummy.c"
#endif
