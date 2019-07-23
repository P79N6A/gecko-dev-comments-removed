































#include "config.h"


#ifdef _WIN32
#define inline __inline
#define alloca _alloca
#define strncasecmp _strnicmp
#define snprintf _snprintf
#ifndef __SYMBIAN32__
#define strcasecmp _stricmp
#endif 
#endif


#ifndef fs_malloc
#define fs_malloc malloc
#endif

#ifndef fs_realloc
#define fs_realloc realloc
#endif

#ifndef fs_free
#define fs_free free
#endif
