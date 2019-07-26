





#ifndef RBTZ_H
#define RBTZ_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/basictz.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN


class UVector;
struct Transition;







class U_I18N_API RuleBasedTimeZone : public BasicTimeZone {
public:
    








    RuleBasedTimeZone(const UnicodeString& id, InitialTimeZoneRule* initialRule);

    




    RuleBasedTimeZone(const RuleBasedTimeZone& source);

    



    virtual ~RuleBasedTimeZone();

    




    RuleBasedTimeZone& operator=(const RuleBasedTimeZone& right);

    







    virtual UBool operator==(const TimeZone& that) const;

    







    virtual UBool operator!=(const TimeZone& that) const;

    














    void addTransitionRule(TimeZoneRule* rule, UErrorCode& status);

    








    void complete(UErrorCode& status);

    






    virtual TimeZone* clone(void) const;

    























    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                              uint8_t dayOfWeek, int32_t millis, UErrorCode& status) const;

    


















    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                           uint8_t dayOfWeek, int32_t millis,
                           int32_t monthLength, UErrorCode& status) const;

    





















    virtual void getOffset(UDate date, UBool local, int32_t& rawOffset,
                           int32_t& dstOffset, UErrorCode& ec) const;

    






    virtual void setRawOffset(int32_t offsetMillis);

    






    virtual int32_t getRawOffset(void) const;

    





    virtual UBool useDaylightTime(void) const;

    












    virtual UBool inDaylightTime(UDate date, UErrorCode& status) const;

    







    virtual UBool hasSameRules(const TimeZone& other) const;

    







    virtual UBool getNextTransition(UDate base, UBool inclusive, TimeZoneTransition& result) ;

    







    virtual UBool getPreviousTransition(UDate base, UBool inclusive, TimeZoneTransition& result) ;

    







    virtual int32_t countTransitionRules(UErrorCode& status) ;

    















    virtual void getTimeZoneRules(const InitialTimeZoneRule*& initial,
        const TimeZoneRule* trsrules[], int32_t& trscount, UErrorCode& status) ;

    



    virtual void getOffsetFromLocal(UDate date, int32_t nonExistingTimeOpt, int32_t duplicatedTimeOpt,
        int32_t& rawOffset, int32_t& dstOffset, UErrorCode& status) ;

private:
    void deleteRules(void);
    void deleteTransitions(void);
    UVector* copyRules(UVector* source);
    TimeZoneRule* findRuleInFinal(UDate date, UBool local,
        int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt) const;
    UBool findNext(UDate base, UBool inclusive, UDate& time, TimeZoneRule*& from, TimeZoneRule*& to) const;
    UBool findPrev(UDate base, UBool inclusive, UDate& time, TimeZoneRule*& from, TimeZoneRule*& to) const;
    int32_t getLocalDelta(int32_t rawBefore, int32_t dstBefore, int32_t rawAfter, int32_t dstAfter,
        int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt) const;
    UDate getTransitionTime(Transition* transition, UBool local,
        int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt) const;
    void getOffsetInternal(UDate date, UBool local, int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt,
        int32_t& rawOffset, int32_t& dstOffset, UErrorCode& ec) const;

    InitialTimeZoneRule *fInitialRule;
    UVector             *fHistoricRules;
    UVector             *fFinalRules;
    UVector             *fHistoricTransitions;
    UBool               fUpToDate;

public:
    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;
};

U_NAMESPACE_END

#endif 

#endif 


