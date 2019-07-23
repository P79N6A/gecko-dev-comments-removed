


































#ifndef _HAVE_CAIRO_COMBSORT_NEWGAP
#define _HAVE_CAIRO_COMBSORT_NEWGAP
static inline unsigned int
_cairo_combsort_newgap (unsigned int gap)
{
  gap = 10 * gap / 13;
  if (gap == 9 || gap == 10)
    gap = 11;
  if (gap < 1)
    gap = 1;
  return gap;
}
#endif

#define CAIRO_COMBSORT_DECLARE(NAME, TYPE, CMP) \
static void \
NAME (TYPE *base, unsigned int nmemb) \
{ \
  unsigned int gap = nmemb; \
  unsigned int i, j; \
  int swapped; \
  do { \
      gap = _cairo_combsort_newgap (gap); \
      swapped = 0; \
      for (i = 0; i < nmemb-gap ; i++) { \
	  j = i + gap; \
	  if (CMP (base[i], base[j]) > 0 ) { \
	      TYPE tmp; \
	      tmp = base[i]; \
	      base[i] = base[j]; \
	      base[j] = tmp; \
	      swapped = 1; \
	  } \
      } \
  } while (gap > 1 || swapped); \
}
