








































#ifndef __memalloc_simple_h_
#define __memalloc_simple_h_

#include <sk_glu.h>
#include <stdlib.h>

#define memRealloc	realloc
#define memFree		free

#define memInit		__gl_memInit

extern int		__gl_memInit( size_t );

#ifndef MEMORY_DEBUG
#define memAlloc	malloc
#else
#define memAlloc	__gl_memAlloc
extern void *		__gl_memAlloc( size_t );
#endif

#endif
