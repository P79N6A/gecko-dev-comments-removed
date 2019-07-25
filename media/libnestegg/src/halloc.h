













#ifndef _LIBP_HALLOC_H_
#define _LIBP_HALLOC_H_

#include <stddef.h>  




void * halloc (void * block, size_t len);
void   hattach(void * block, void * parent);




void * h_malloc (size_t len);
void * h_calloc (size_t n, size_t len);
void * h_realloc(void * p, size_t len);
void   h_free   (void * p);
char * h_strdup (const char * str);




typedef void * (* realloc_t)(void * ptr, size_t len);

extern realloc_t halloc_allocator;

#endif

