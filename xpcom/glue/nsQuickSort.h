









































#ifndef nsQuickSort_h___
#define nsQuickSort_h___

#include "prtypes.h"
#include "nscore.h"

PR_BEGIN_EXTERN_C













NS_COM_GLUE void NS_QuickSort(void *, unsigned int, unsigned int,
                              int (*)(const void *, const void *, void *), 
                              void *);

PR_END_EXTERN_C

#endif 
