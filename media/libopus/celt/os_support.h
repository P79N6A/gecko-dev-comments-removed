






































#ifndef OS_SUPPORT_H
#define OS_SUPPORT_H

#ifdef CUSTOM_SUPPORT
#  include "custom_support.h"
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#ifndef OVERRIDE_OPUS_ALLOC
static inline void *opus_alloc (size_t size)
{
   return malloc(size);
}
#endif


#ifndef OVERRIDE_OPUS_ALLOC_SCRATCH
static inline void *opus_alloc_scratch (size_t size)
{
   
   return opus_alloc(size);
}
#endif


#ifndef OVERRIDE_OPUS_FREE
static inline void opus_free (void *ptr)
{
   free(ptr);
}
#endif


#ifndef OVERRIDE_OPUS_COPY
#define OPUS_COPY(dst, src, n) (memcpy((dst), (src), (n)*sizeof(*(dst)) + 0*((dst)-(src)) ))
#endif



#ifndef OVERRIDE_OPUS_MOVE
#define OPUS_MOVE(dst, src, n) (memmove((dst), (src), (n)*sizeof(*(dst)) + 0*((dst)-(src)) ))
#endif


#ifndef OVERRIDE_OPUS_CLEAR
#define OPUS_CLEAR(dst, n) (memset((dst), 0, (n)*sizeof(*(dst))))
#endif






#endif 

