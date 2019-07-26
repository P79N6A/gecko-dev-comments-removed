













#ifndef __VZONE_H
#define __VZONE_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "ztrans.h"

#ifndef UCNV_H
struct VZone;




typedef struct VZone VZone;
#endif












U_CAPI VZone* U_EXPORT2
vzone_openID(const UChar* ID, int32_t idLength);









U_CAPI VZone* U_EXPORT2
vzone_openData(const UChar* vtzdata, int32_t vtzdataLength, UErrorCode& status);






U_CAPI void U_EXPORT2
vzone_close(VZone* zone);






U_CAPI VZone* U_EXPORT2
vzone_clone(const VZone *zone);








U_CAPI UBool U_EXPORT2
vzone_equals(const VZone* zone1, const VZone* zone2);










U_CAPI UBool U_EXPORT2
vzone_getTZURL(VZone* zone, UChar* & url, int32_t & urlLength);







U_CAPI void U_EXPORT2
vzone_setTZURL(VZone* zone, UChar* url, int32_t urlLength);










U_CAPI UBool U_EXPORT2
vzone_getLastModified(VZone* zone, UDate& lastModified);






U_CAPI void U_EXPORT2
vzone_setLastModified(VZone* zone, UDate lastModified);








U_CAPI void U_EXPORT2
vzone_write(VZone* zone, UChar* & result, int32_t & resultLength, UErrorCode& status);










U_CAPI void U_EXPORT2
vzone_writeFromStart(VZone* zone, UDate start, UChar* & result, int32_t & resultLength, UErrorCode& status);














U_CAPI void U_EXPORT2
vzone_writeSimple(VZone* zone, UDate time, UChar* & result, int32_t & resultLength, UErrorCode& status);

























U_CAPI int32_t U_EXPORT2
vzone_getOffset(VZone* zone, uint8_t era, int32_t year, int32_t month, int32_t day,
                uint8_t dayOfWeek, int32_t millis, UErrorCode& status);




















U_CAPI int32_t U_EXPORT2
vzone_getOffset2(VZone* zone, uint8_t era, int32_t year, int32_t month, int32_t day,
                uint8_t dayOfWeek, int32_t millis,
                int32_t monthLength, UErrorCode& status);























U_CAPI void U_EXPORT2
vzone_getOffset3(VZone* zone, UDate date, UBool local, int32_t& rawOffset,
                int32_t& dstOffset, UErrorCode& ec);








U_CAPI void U_EXPORT2
vzone_setRawOffset(VZone* zone, int32_t offsetMillis);








U_CAPI int32_t U_EXPORT2
vzone_getRawOffset(VZone* zone);







U_CAPI UBool U_EXPORT2
vzone_useDaylightTime(VZone* zone);














U_INTERNAL UBool U_EXPORT2
vzone_inDaylightTime(VZone* zone, UDate date, UErrorCode& status);









U_CAPI UBool U_EXPORT2
vzone_hasSameRules(VZone* zone, const VZone* other);









U_CAPI UBool U_EXPORT2
vzone_getNextTransition(VZone* zone, UDate base, UBool inclusive, ZTrans* result);









U_CAPI UBool U_EXPORT2
vzone_getPreviousTransition(VZone* zone, UDate base, UBool inclusive, ZTrans* result);









U_CAPI int32_t U_EXPORT2
vzone_countTransitionRules(VZone* zone, UErrorCode& status);












U_CAPI UClassID U_EXPORT2
vzone_getStaticClassID(VZone* zone);












U_CAPI UClassID U_EXPORT2
vzone_getDynamicClassID(VZone* zone);

#endif 

#endif
