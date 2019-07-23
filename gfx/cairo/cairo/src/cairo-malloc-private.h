



































#ifndef CAIRO_MALLOC_PRIVATE_H
#define CAIRO_MALLOC_PRIVATE_H

#include "cairo-wideint-private.h"


















#define _cairo_malloc_ab(a, size) \
  ((unsigned) (a) >= INT32_MAX / (unsigned) (size) ? NULL : \
   malloc((unsigned) (a) * (unsigned) (size)))


















#define _cairo_malloc_abc(a, b, size) \
  ((unsigned) (a) >= INT32_MAX / (unsigned) (b) ? NULL : \
   (unsigned) ((a)*(b)) >= INT32_MAX / (unsigned) (size) ? NULL : \
   malloc((unsigned) (a) * (unsigned) (b) * (unsigned) size))















#define _cairo_malloc_ab_plus_c(n, size, k) \
  ((unsigned) (n) >= INT32_MAX / (unsigned) (size) ? NULL : \
   (unsigned) (k) >= INT32_MAX - (unsigned) (n) * (unsigned) (size) ? NULL : \
   malloc((unsigned) (n) * (unsigned) (size) + (unsigned) (k)))

#endif 
