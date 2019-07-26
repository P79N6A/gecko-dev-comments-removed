














#ifndef _GENDER
#define _GENDER

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/locid.h"
#include "unicode/ugender.h"
#include "unicode/uobject.h"

class GenderInfoTest;

U_NAMESPACE_BEGIN






class U_I18N_API GenderInfo : public UObject {
public:

    













    static const GenderInfo* U_EXPORT2 getInstance(const Locale& locale, UErrorCode& status);

    










    UGender getListGender(const UGender* genders, int32_t length, UErrorCode& status) const;

    





    virtual ~GenderInfo();

private:
    int32_t _style;


    


    virtual UClassID getDynamicClassID() const;

    



    GenderInfo(const GenderInfo& other);

    


    GenderInfo& operator=(const GenderInfo&);

    GenderInfo();

    static const GenderInfo* getNeutralInstance();

    static const GenderInfo* getMixedNeutralInstance();

    static const GenderInfo* getMaleTaintsInstance();

    static const GenderInfo* loadInstance(const Locale& locale, UErrorCode& status);
    friend class ::GenderInfoTest;
};

U_NAMESPACE_END

#endif 

#endif 

