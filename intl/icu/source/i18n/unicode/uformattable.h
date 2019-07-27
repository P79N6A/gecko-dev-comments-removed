


























#ifndef UFORMATTABLE_H
#define UFORMATTABLE_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/localpointer.h"








typedef enum UFormattableType {
  UFMT_DATE = 0, 
  UFMT_DOUBLE,   
  UFMT_LONG,     
  UFMT_STRING,   
  UFMT_ARRAY,    
  UFMT_INT64,    
  UFMT_OBJECT,   
  UFMT_COUNT     
} UFormattableType;








typedef void *UFormattable;











U_STABLE UFormattable* U_EXPORT2
ufmt_open(UErrorCode* status);







U_STABLE void U_EXPORT2
ufmt_close(UFormattable* fmt);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUFormattablePointer, UFormattable, ufmt_close);

U_NAMESPACE_END

#endif











U_STABLE UFormattableType U_EXPORT2
ufmt_getType(const UFormattable* fmt, UErrorCode *status);









U_STABLE UBool U_EXPORT2
ufmt_isNumeric(const UFormattable* fmt);











U_STABLE UDate U_EXPORT2
ufmt_getDate(const UFormattable* fmt, UErrorCode *status);
















U_STABLE double U_EXPORT2
ufmt_getDouble(UFormattable* fmt, UErrorCode *status);



















U_STABLE int32_t U_EXPORT2
ufmt_getLong(UFormattable* fmt, UErrorCode *status);



















U_STABLE int64_t U_EXPORT2
ufmt_getInt64(UFormattable* fmt, UErrorCode *status);











U_STABLE const void *U_EXPORT2
ufmt_getObject(const UFormattable* fmt, UErrorCode *status);













U_STABLE const UChar* U_EXPORT2
ufmt_getUChars(UFormattable* fmt, int32_t *len, UErrorCode *status);









U_STABLE int32_t U_EXPORT2
ufmt_getArrayLength(const UFormattable* fmt, UErrorCode *status);










U_STABLE UFormattable * U_EXPORT2
ufmt_getArrayItemByIndex(UFormattable* fmt, int32_t n, UErrorCode *status);























U_STABLE const char * U_EXPORT2
ufmt_getDecNumChars(UFormattable *fmt, int32_t *len, UErrorCode *status);

#endif

#endif
