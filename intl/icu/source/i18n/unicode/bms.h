










#ifndef _BMS_H
#define _BMS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/ucol.h"

#ifndef U_HIDE_INTERNAL_API

















typedef void UCD;















U_INTERNAL UCD * U_EXPORT2
ucd_open(UCollator *coll, UErrorCode *status);








U_INTERNAL void U_EXPORT2
ucd_close(UCD *ucd);














U_INTERNAL UCollator * U_EXPORT2
ucd_getCollator(UCD *ucd);













U_INTERNAL void U_EXPORT2
ucd_freeCache();








U_INTERNAL void U_EXPORT2
ucd_flushCache();














































































struct BMS;
typedef struct BMS BMS; 



















U_INTERNAL BMS * U_EXPORT2
bms_open(UCD *ucd,
         const UChar *pattern, int32_t patternLength,
         const UChar *target,  int32_t targetLength,
         UErrorCode  *status);








U_INTERNAL void U_EXPORT2
bms_close(BMS *bms);









U_INTERNAL UBool U_EXPORT2
bms_empty(BMS *bms);












U_INTERNAL UCD * U_EXPORT2
bms_getData(BMS *bms);













U_INTERNAL UBool U_EXPORT2
bms_search(BMS *bms, int32_t offset, int32_t *start, int32_t *end);











U_INTERNAL void U_EXPORT2
bms_setTargetString(BMS *bms, const UChar *target, int32_t targetLength, UErrorCode *status);

#endif  

#endif

#endif 
