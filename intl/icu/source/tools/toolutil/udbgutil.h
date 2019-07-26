








#ifndef _UDBGUTIL_H
#define _UDBGUTIL_H

#include "unicode/utypes.h"
#include <stdio.h>

enum UDebugEnumType {
    UDBG_UDebugEnumType = 0, 
#if !UCONFIG_NO_FORMATTING
    UDBG_UCalendarDateFields, 
    UDBG_UCalendarMonths, 
    UDBG_UDateFormatStyle, 
#endif
    UDBG_UPlugReason,   
    UDBG_UPlugLevel,    
    UDBG_UAcceptResult, 
    
     
    
#if !UCONFIG_NO_COLLATION
    UDBG_UColAttributeValue,  
#endif
    UDBG_ENUM_COUNT,
    UDBG_HIGHEST_CONTIGUOUS_ENUM = UDBG_UAcceptResult,  
    UDBG_INVALID_ENUM = -1 
};

typedef enum UDebugEnumType UDebugEnumType;






U_CAPI int32_t U_EXPORT2 udbg_enumCount(UDebugEnumType type);







U_CAPI const char * U_EXPORT2 udbg_enumName(UDebugEnumType type, int32_t field);







U_CAPI int32_t U_EXPORT2 udbg_enumExpectedCount(UDebugEnumType type);







U_CAPI int32_t U_EXPORT2 udbg_enumArrayValue(UDebugEnumType type, int32_t field);







U_CAPI int32_t U_EXPORT2 udbg_enumByName(UDebugEnumType type, const char *name);





U_CAPI const char *udbg_getPlatform(void);







U_CAPI const char *udbg_getSystemParameterNameByIndex(int32_t i);








U_CAPI int32_t udbg_getSystemParameterValueByIndex(int32_t i, char *buffer, int32_t bufferCapacity, UErrorCode *status);




U_CAPI void udbg_writeIcuInfo(FILE *f);

#endif
