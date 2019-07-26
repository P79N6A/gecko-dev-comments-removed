
























#ifndef _EVENT_MM_INTERNAL_H
#define _EVENT_MM_INTERNAL_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _EVENT_DISABLE_MM_REPLACEMENT



void *event_mm_malloc_(size_t sz);
void *event_mm_calloc_(size_t count, size_t size);
char *event_mm_strdup_(const char *s);
void *event_mm_realloc_(void *p, size_t sz);
void event_mm_free_(void *p);
#define mm_malloc(sz) event_mm_malloc_(sz)
#define mm_calloc(count, size) event_mm_calloc_((count), (size))
#define mm_strdup(s) event_mm_strdup_(s)
#define mm_realloc(p, sz) event_mm_realloc_((p), (sz))
#define mm_free(p) event_mm_free_(p)
#else
#define mm_malloc(sz) malloc(sz)
#define mm_calloc(n, sz) calloc((n), (sz))
#define mm_strdup(s) strdup(s)
#define mm_realloc(p, sz) realloc((p), (sz))
#define mm_free(p) free(p)
#endif

#ifdef __cplusplus
}
#endif

#endif
