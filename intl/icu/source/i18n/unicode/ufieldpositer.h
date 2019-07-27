






#ifndef UFIELDPOSITER_H
#define UFIELDPOSITER_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#ifndef U_HIDE_DRAFT_API

#include "unicode/localpointer.h"

























struct UFieldPositionIterator;
typedef struct UFieldPositionIterator UFieldPositionIterator;  










U_DRAFT UFieldPositionIterator* U_EXPORT2
ufieldpositer_open(UErrorCode* status);







U_DRAFT void U_EXPORT2
ufieldpositer_close(UFieldPositionIterator *fpositer);


#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUFieldPositionIteratorPointer, UFieldPositionIterator, ufieldpositer_close);

U_NAMESPACE_END

#endif





























U_DRAFT int32_t U_EXPORT2
ufieldpositer_next(UFieldPositionIterator *fpositer,
                   int32_t *beginIndex, int32_t *endIndex);

#endif 
#endif 

#endif
