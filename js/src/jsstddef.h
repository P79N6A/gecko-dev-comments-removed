











































#ifdef _WINDOWS
# ifndef XP_WIN
# define XP_WIN
# endif
#if defined(_WIN32) || defined(WIN32)
# ifndef XP_WIN32
# define XP_WIN32
# endif
#else
# ifndef XP_WIN16
# define XP_WIN16
# endif
#endif
#endif

#ifdef XP_WIN16
#ifndef _PTRDIFF_T_DEFINED
typedef long ptrdiff_t;






#define PTRDIFF(p1, p2, type)                                 \
    ((((unsigned long)(p1)) - ((unsigned long)(p2))) / sizeof(type))

#define _PTRDIFF_T_DEFINED
#endif 
#else 

#define PTRDIFF(p1, p2, type)                                 \
        ((p1) - (p2))

#endif

#include <stddef.h>


