









#ifndef nsQuickSort_h___
#define nsQuickSort_h___

#include "prtypes.h"
#include "nscore.h"

#ifdef __cplusplus
extern "C" {
#endif













NS_COM_GLUE void NS_QuickSort(void *, unsigned int, unsigned int,
                              int (*)(const void *, const void *, void *), 
                              void *);

#ifdef __cplusplus
}
#endif

#endif
