






#ifndef UREGION_H
#define UREGION_H

#include "unicode/utypes.h"
#include "unicode/uenum.h"


















































typedef enum URegionType {
    



    URGN_UNKNOWN,

    



    URGN_TERRITORY,

    



    URGN_WORLD,

    



    URGN_CONTINENT,

    



    URGN_SUBCONTINENT,

    




    URGN_GROUPING,

    




    URGN_DEPRECATED,

    



    URGN_LIMIT
} URegionType;

#if !UCONFIG_NO_FORMATTING





struct URegion;
typedef struct URegion URegion; 









U_STABLE const URegion* U_EXPORT2
uregion_getRegionFromCode(const char *regionCode, UErrorCode *status);






U_STABLE const URegion* U_EXPORT2
uregion_getRegionFromNumericCode (int32_t code, UErrorCode *status);






U_STABLE UEnumeration* U_EXPORT2
uregion_getAvailable(URegionType type, UErrorCode *status);





U_STABLE UBool U_EXPORT2
uregion_areEqual(const URegion* uregion, const URegion* otherRegion);







U_STABLE const URegion* U_EXPORT2
uregion_getContainingRegion(const URegion* uregion);










U_STABLE const URegion* U_EXPORT2
uregion_getContainingRegionOfType(const URegion* uregion, URegionType type);











U_STABLE UEnumeration* U_EXPORT2
uregion_getContainedRegions(const URegion* uregion, UErrorCode *status);










U_STABLE UEnumeration* U_EXPORT2
uregion_getContainedRegionsOfType(const URegion* uregion, URegionType type, UErrorCode *status);






U_STABLE UBool U_EXPORT2
uregion_contains(const URegion* uregion, const URegion* otherRegion);









U_STABLE UEnumeration* U_EXPORT2
uregion_getPreferredValues(const URegion* uregion, UErrorCode *status);





U_STABLE const char* U_EXPORT2
uregion_getRegionCode(const URegion* uregion);






U_STABLE int32_t U_EXPORT2
uregion_getNumericCode(const URegion* uregion);





U_STABLE URegionType U_EXPORT2
uregion_getType(const URegion* uregion);


#endif 

#endif
