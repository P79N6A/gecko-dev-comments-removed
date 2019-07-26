





#ifndef BASICTZ_H
#define BASICTZ_H






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/timezone.h"
#include "unicode/tzrule.h"
#include "unicode/tztrans.h"

U_NAMESPACE_BEGIN


class UVector;







class U_I18N_API BasicTimeZone: public TimeZone {
public:
    



    virtual ~BasicTimeZone();

    







    virtual UBool getNextTransition(UDate base, UBool inclusive, TimeZoneTransition& result)  = 0;

    







    virtual UBool getPreviousTransition(UDate base, UBool inclusive, TimeZoneTransition& result)  = 0;

    



















    virtual UBool hasEquivalentTransitions( BasicTimeZone& tz, UDate start, UDate end,
        UBool ignoreDstAmount, UErrorCode& ec) ;

    







    virtual int32_t countTransitionRules(UErrorCode& status)  = 0;

    















    virtual void getTimeZoneRules(const InitialTimeZoneRule*& initial,
        const TimeZoneRule* trsrules[], int32_t& trscount, UErrorCode& status)  = 0;

    

























    virtual void getSimpleRulesNear(UDate date, InitialTimeZoneRule*& initial,
        AnnualTimeZoneRule*& std, AnnualTimeZoneRule*& dst, UErrorCode& status) ;


#ifndef U_HIDE_INTERNAL_API
    



    enum {
        kStandard = 0x01,
        kDaylight = 0x03,
        kFormer = 0x04,
        kLatter = 0x0C
    };
#endif  

    



    virtual void getOffsetFromLocal(UDate date, int32_t nonExistingTimeOpt, int32_t duplicatedTimeOpt,
        int32_t& rawOffset, int32_t& dstOffset, UErrorCode& status) ;

protected:

#ifndef U_HIDE_INTERNAL_API
    



    enum {
        kStdDstMask = kDaylight,
        kFormerLatterMask = kLatter
    };
#endif  

    



    BasicTimeZone();

    




    BasicTimeZone(const UnicodeString &id);

    




    BasicTimeZone(const BasicTimeZone& source);

    






    void getTimeZoneRulesAfter(UDate start, InitialTimeZoneRule*& initial, UVector*& transitionRules,
        UErrorCode& status) ;
};

U_NAMESPACE_END

#endif 

#endif 


