















#ifndef __UDATPG_H__
#define __UDATPG_H__

#include "unicode/utypes.h"
#include "unicode/uenum.h"
#include "unicode/localpointer.h"






















typedef void *UDateTimePatternGenerator;







typedef enum UDateTimePatternField {
    
    UDATPG_ERA_FIELD,
    
    UDATPG_YEAR_FIELD,
    
    UDATPG_QUARTER_FIELD,
    
    UDATPG_MONTH_FIELD,
    
    UDATPG_WEEK_OF_YEAR_FIELD,
    
    UDATPG_WEEK_OF_MONTH_FIELD,
    
    UDATPG_WEEKDAY_FIELD,
    
    UDATPG_DAY_OF_YEAR_FIELD,
    
    UDATPG_DAY_OF_WEEK_IN_MONTH_FIELD,
    
    UDATPG_DAY_FIELD,
    
    UDATPG_DAYPERIOD_FIELD,
    
    UDATPG_HOUR_FIELD,
    
    UDATPG_MINUTE_FIELD,
    
    UDATPG_SECOND_FIELD,
    
    UDATPG_FRACTIONAL_SECOND_FIELD,
    
    UDATPG_ZONE_FIELD,
    
    UDATPG_FIELD_COUNT
} UDateTimePatternField;








typedef enum UDateTimePatternMatchOptions {
    
    UDATPG_MATCH_NO_OPTIONS = 0,
    
    UDATPG_MATCH_HOUR_FIELD_LENGTH = 1 << UDATPG_HOUR_FIELD,
#ifndef U_HIDE_INTERNAL_API
    
    UDATPG_MATCH_MINUTE_FIELD_LENGTH = 1 << UDATPG_MINUTE_FIELD,
    
    UDATPG_MATCH_SECOND_FIELD_LENGTH = 1 << UDATPG_SECOND_FIELD,
#endif  
    
    UDATPG_MATCH_ALL_FIELDS_LENGTH = (1 << UDATPG_FIELD_COUNT) - 1
} UDateTimePatternMatchOptions;





typedef enum UDateTimePatternConflict {
    
    UDATPG_NO_CONFLICT,
    
    UDATPG_BASE_CONFLICT,
    
    UDATPG_CONFLICT,
    
    UDATPG_CONFLICT_COUNT
} UDateTimePatternConflict;









U_STABLE UDateTimePatternGenerator * U_EXPORT2
udatpg_open(const char *locale, UErrorCode *pErrorCode);








U_STABLE UDateTimePatternGenerator * U_EXPORT2
udatpg_openEmpty(UErrorCode *pErrorCode);






U_STABLE void U_EXPORT2
udatpg_close(UDateTimePatternGenerator *dtpg);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUDateTimePatternGeneratorPointer, UDateTimePatternGenerator, udatpg_close);

U_NAMESPACE_END

#endif









U_STABLE UDateTimePatternGenerator * U_EXPORT2
udatpg_clone(const UDateTimePatternGenerator *dtpg, UErrorCode *pErrorCode);
























U_STABLE int32_t U_EXPORT2
udatpg_getBestPattern(UDateTimePatternGenerator *dtpg,
                      const UChar *skeleton, int32_t length,
                      UChar *bestPattern, int32_t capacity,
                      UErrorCode *pErrorCode);































U_STABLE int32_t U_EXPORT2
udatpg_getBestPatternWithOptions(UDateTimePatternGenerator *dtpg,
                                 const UChar *skeleton, int32_t length,
                                 UDateTimePatternMatchOptions options,
                                 UChar *bestPattern, int32_t capacity,
                                 UErrorCode *pErrorCode);





















U_STABLE int32_t U_EXPORT2
udatpg_getSkeleton(UDateTimePatternGenerator *dtpg,
                   const UChar *pattern, int32_t length,
                   UChar *skeleton, int32_t capacity,
                   UErrorCode *pErrorCode);
























U_STABLE int32_t U_EXPORT2
udatpg_getBaseSkeleton(UDateTimePatternGenerator *dtpg,
                       const UChar *pattern, int32_t length,
                       UChar *baseSkeleton, int32_t capacity,
                       UErrorCode *pErrorCode);

























U_STABLE UDateTimePatternConflict U_EXPORT2
udatpg_addPattern(UDateTimePatternGenerator *dtpg,
                  const UChar *pattern, int32_t patternLength,
                  UBool override,
                  UChar *conflictingPattern, int32_t capacity, int32_t *pLength,
                  UErrorCode *pErrorCode);





















U_STABLE void U_EXPORT2
udatpg_setAppendItemFormat(UDateTimePatternGenerator *dtpg,
                           UDateTimePatternField field,
                           const UChar *value, int32_t length);











U_STABLE const UChar * U_EXPORT2
udatpg_getAppendItemFormat(const UDateTimePatternGenerator *dtpg,
                           UDateTimePatternField field,
                           int32_t *pLength);














U_STABLE void U_EXPORT2
udatpg_setAppendItemName(UDateTimePatternGenerator *dtpg,
                         UDateTimePatternField field,
                         const UChar *value, int32_t length);











U_STABLE const UChar * U_EXPORT2
udatpg_getAppendItemName(const UDateTimePatternGenerator *dtpg,
                         UDateTimePatternField field,
                         int32_t *pLength);






















U_STABLE void U_EXPORT2
udatpg_setDateTimeFormat(const UDateTimePatternGenerator *dtpg,
                         const UChar *dtFormat, int32_t length);








U_STABLE const UChar * U_EXPORT2
udatpg_getDateTimeFormat(const UDateTimePatternGenerator *dtpg,
                         int32_t *pLength);














U_STABLE void U_EXPORT2
udatpg_setDecimal(UDateTimePatternGenerator *dtpg,
                  const UChar *decimal, int32_t length);









U_STABLE const UChar * U_EXPORT2
udatpg_getDecimal(const UDateTimePatternGenerator *dtpg,
                  int32_t *pLength);


























U_STABLE int32_t U_EXPORT2
udatpg_replaceFieldTypes(UDateTimePatternGenerator *dtpg,
                         const UChar *pattern, int32_t patternLength,
                         const UChar *skeleton, int32_t skeletonLength,
                         UChar *dest, int32_t destCapacity,
                         UErrorCode *pErrorCode);































U_STABLE int32_t U_EXPORT2
udatpg_replaceFieldTypesWithOptions(UDateTimePatternGenerator *dtpg,
                                    const UChar *pattern, int32_t patternLength,
                                    const UChar *skeleton, int32_t skeletonLength,
                                    UDateTimePatternMatchOptions options,
                                    UChar *dest, int32_t destCapacity,
                                    UErrorCode *pErrorCode);












U_STABLE UEnumeration * U_EXPORT2
udatpg_openSkeletons(const UDateTimePatternGenerator *dtpg, UErrorCode *pErrorCode);











U_STABLE UEnumeration * U_EXPORT2
udatpg_openBaseSkeletons(const UDateTimePatternGenerator *dtpg, UErrorCode *pErrorCode);











U_STABLE const UChar * U_EXPORT2
udatpg_getPatternForSkeleton(const UDateTimePatternGenerator *dtpg,
                             const UChar *skeleton, int32_t skeletonLength,
                             int32_t *pLength);

#endif
