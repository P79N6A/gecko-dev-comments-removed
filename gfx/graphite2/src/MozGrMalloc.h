





































#ifndef MOZ_GR_MALLOC_H
#define MOZ_GR_MALLOC_H





#include "mozilla/mozalloc.h"

#define malloc moz_xmalloc
#define calloc moz_xcalloc
#define realloc moz_xrealloc
#define free moz_free

#endif 
