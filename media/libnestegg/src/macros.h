













#ifndef _LIBP_MACROS_H_
#define _LIBP_MACROS_H_

#include <stddef.h>  




#define structof(p,t,f) ((t*)(- offsetof(t,f) + (char*)(p)))




#ifdef _WIN32
#define static_inline static __inline
#else
#define static_inline static __inline__
#endif


#endif

