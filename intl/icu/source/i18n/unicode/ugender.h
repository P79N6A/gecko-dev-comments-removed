






#ifndef UGENDER_H
#define UGENDER_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/localpointer.h"












enum UGender {
    



    UGENDER_MALE,
    



    UGENDER_FEMALE,
    



    UGENDER_OTHER
};



typedef enum UGender UGender;





struct UGenderInfo;
typedef struct UGenderInfo UGenderInfo;







U_STABLE const UGenderInfo* U_EXPORT2
ugender_getInstance(const char *locale, UErrorCode *status);











U_DRAFT UGender U_EXPORT2
ugender_getListGender(const UGenderInfo* genderinfo, const UGender *genders, int32_t size, UErrorCode *status);

#endif 

#endif
