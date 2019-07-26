















#ifndef __ULOCDATA_H__
#define __ULOCDATA_H__

#include "unicode/ures.h"
#include "unicode/uloc.h"
#include "unicode/uset.h"
#include "unicode/localpointer.h"







struct ULocaleData;


typedef struct ULocaleData ULocaleData;






typedef enum ULocaleDataExemplarSetType  {
    
    ULOCDATA_ES_STANDARD=0,
    
    ULOCDATA_ES_AUXILIARY=1,
    
    ULOCDATA_ES_INDEX=2,
    
    ULOCDATA_ES_COUNT=3
} ULocaleDataExemplarSetType;




typedef enum ULocaleDataDelimiterType {
    
    ULOCDATA_QUOTATION_START = 0,
    
    ULOCDATA_QUOTATION_END = 1,
    
    ULOCDATA_ALT_QUOTATION_START = 2,
    
    ULOCDATA_ALT_QUOTATION_END = 3,
    
    ULOCDATA_DELIMITER_COUNT = 4
} ULocaleDataDelimiterType;









U_STABLE ULocaleData* U_EXPORT2
ulocdata_open(const char *localeID, UErrorCode *status);







U_STABLE void U_EXPORT2
ulocdata_close(ULocaleData *uld);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalULocaleDataPointer, ULocaleData, ulocdata_close);

U_NAMESPACE_END

#endif












U_STABLE void U_EXPORT2
ulocdata_setNoSubstitute(ULocaleData *uld, UBool setting);












U_STABLE UBool U_EXPORT2
ulocdata_getNoSubstitute(ULocaleData *uld);




























U_STABLE USet* U_EXPORT2
ulocdata_getExemplarSet(ULocaleData *uld, USet *fillIn,
                        uint32_t options, ULocaleDataExemplarSetType extype, UErrorCode *status);














U_STABLE int32_t U_EXPORT2
ulocdata_getDelimiter(ULocaleData *uld, ULocaleDataDelimiterType type, UChar *result, int32_t resultLength, UErrorCode *status);





typedef enum UMeasurementSystem {
    UMS_SI,     
    UMS_US,     
    UMS_LIMIT
} UMeasurementSystem;











U_STABLE UMeasurementSystem U_EXPORT2
ulocdata_getMeasurementSystem(const char *localeID, UErrorCode *status);

















U_STABLE void U_EXPORT2
ulocdata_getPaperSize(const char *localeID, int32_t *height, int32_t *width, UErrorCode *status);







U_STABLE void U_EXPORT2
ulocdata_getCLDRVersion(UVersionInfo versionArray, UErrorCode *status);
















U_STABLE int32_t U_EXPORT2
ulocdata_getLocaleDisplayPattern(ULocaleData *uld,
                                 UChar *pattern,
                                 int32_t patternCapacity,
                                 UErrorCode *status);

















U_STABLE int32_t U_EXPORT2
ulocdata_getLocaleSeparator(ULocaleData *uld,
                            UChar *separator,
                            int32_t separatorCapacity,
                            UErrorCode *status);
#endif
