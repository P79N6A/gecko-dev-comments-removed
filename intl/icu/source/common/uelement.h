



















#ifndef __UELEMENT_H__
#define __UELEMENT_H__

#include "unicode/utypes.h"

U_CDECL_BEGIN










union UElement {
    void*   pointer;
    int32_t integer;
};
typedef union UElement UElement;







typedef UBool U_CALLCONV UElementsAreEqual(const UElement e1, const UElement e2);







typedef int8_t U_CALLCONV UElementComparator(UElement e1, UElement e2);







typedef void U_CALLCONV UElementAssigner(UElement *dst, UElement *src);

U_CDECL_END







U_CAPI UBool U_EXPORT2 
uhash_compareUnicodeString(const UElement key1, const UElement key2);









U_CAPI UBool U_EXPORT2 
uhash_compareCaselessUnicodeString(const UElement key1, const UElement key2);

#endif  
