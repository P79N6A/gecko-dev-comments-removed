































#ifndef __OGGZ_MACROS_H__
#define __OGGZ_MACROS_H__

#include <stdlib.h>
#include <ogg/ogg.h>


#define oggz_malloc _ogg_malloc
#define oggz_realloc _ogg_realloc
#define oggz_free _ogg_free

#undef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))

#undef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))




#ifdef UNUSED
#elif defined (__GNUC__)
#       define UNUSED(x) UNUSED_ ## x __attribute__ ((unused))
#elif defined (__LCLINT__)
#       define UNUSED(x) x
#else
#       define UNUSED(x) x
#endif

#ifdef __GNUC__
#       define WARN_UNUSED      __attribute__ ((warn_unused_result))
#else
#       define WARN_UNUSED
#endif

#endif 
