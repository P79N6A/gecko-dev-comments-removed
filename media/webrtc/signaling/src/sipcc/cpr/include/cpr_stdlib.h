






































#ifndef _CPR_STDLIB_H_
#define _CPR_STDLIB_H_

#include "cpr_types.h"

__BEGIN_DECLS

#if defined SIP_OS_LINUX
#include "../linux/cpr_linux_stdlib.h"
#elif defined SIP_OS_WINDOWS
#include "../win32/cpr_win_stdlib.h"
#elif defined SIP_OS_OSX
#include "../darwin/cpr_darwin_stdlib.h"
#endif

#ifdef CPR_USE_DIRECT_OS_CALL





#ifdef CPR_USE_CALLOC_FOR_MALLOC
#define cpr_malloc(x)  calloc(1, x)
#else
#define cpr_malloc  malloc
#endif
#define cpr_calloc  calloc
#define cpr_realloc realloc
#define cpr_strdup  _strdup
#define cpr_free    free

#define CPR_REACH_MEMORY_HIGH_WATER_MARK FALSE

#else












void *cpr_malloc(size_t size);















void *cpr_calloc(size_t nelem, size_t size);












void *cpr_realloc(void *object, size_t size);















char *cpr_strdup(const char *string);









void  cpr_free(void *mem);
#endif

__END_DECLS

#endif


