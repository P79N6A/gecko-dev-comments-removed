



































#ifndef CAIRO_MALLOC_PRIVATE_H
#define CAIRO_MALLOC_PRIVATE_H

#include "cairo-wideint-private.h"

#if HAVE_MEMFAULT
#include <memfault.h>
#define CAIRO_INJECT_FAULT() VALGRIND_INJECT_FAULT()
#else
#define CAIRO_INJECT_FAULT() 0
#endif













#define _cairo_malloc(size) \
   ((size) ? malloc((unsigned) (size)) : NULL)


















#define _cairo_malloc_ab(a, size) \
  ((size) && (unsigned) (a) >= INT32_MAX / (unsigned) (size) ? NULL : \
   _cairo_malloc((unsigned) (a) * (unsigned) (size)))



















#define _cairo_realloc_ab(ptr, a, size) \
  ((size) && (unsigned) (a) >= INT32_MAX / (unsigned) (size) ? NULL : \
   realloc(ptr, (unsigned) (a) * (unsigned) (size)))


















#define _cairo_malloc_abc(a, b, size) \
  ((b) && (unsigned) (a) >= INT32_MAX / (unsigned) (b) ? NULL : \
   (size) && (unsigned) ((a)*(b)) >= INT32_MAX / (unsigned) (size) ? NULL : \
   _cairo_malloc((unsigned) (a) * (unsigned) (b) * (unsigned) (size)))















#define _cairo_malloc_ab_plus_c(n, size, k) \
  ((size) && (unsigned) (n) >= INT32_MAX / (unsigned) (size) ? NULL : \
   (unsigned) (k) >= INT32_MAX - (unsigned) (n) * (unsigned) (size) ? NULL : \
   _cairo_malloc((unsigned) (n) * (unsigned) (size) + (unsigned) (k)))

#endif 
