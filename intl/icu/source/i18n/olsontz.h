









#ifndef OLSONTZ_H
#define OLSONTZ_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/basictz.h"

struct UResourceBundle;

U_NAMESPACE_BEGIN

class SimpleTimeZone;
























































































class U_I18N_API OlsonTimeZone: public BasicTimeZone {
 public:
    







    OlsonTimeZone(const UResourceBundle* top,
                  const UResourceBundle* res,
                  const UnicodeString& tzid,
                  UErrorCode& ec);

    


    OlsonTimeZone(const OlsonTimeZone& other);

    


    virtual ~OlsonTimeZone();

    


    OlsonTimeZone& operator=(const OlsonTimeZone& other);

    


    virtual UBool operator==(const TimeZone& other) const;

    


    virtual TimeZone* clone() const;

    


    static UClassID U_EXPORT2 getStaticClassID();

    


    virtual UClassID getDynamicClassID() const;
    
    


    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month,
                              int32_t day, uint8_t dayOfWeek,
                              int32_t millis, UErrorCode& ec) const;

    


    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month,
                              int32_t day, uint8_t dayOfWeek,
                              int32_t millis, int32_t monthLength,
                              UErrorCode& ec) const;

    


    virtual void getOffset(UDate date, UBool local, int32_t& rawOffset,
                   int32_t& dstOffset, UErrorCode& ec) const;

    


    virtual void getOffsetFromLocal(UDate date, int32_t nonExistingTimeOpt, int32_t duplicatedTimeOpt,
        int32_t& rawoff, int32_t& dstoff, UErrorCode& ec) ;

    




    virtual void setRawOffset(int32_t offsetMillis);

    





    virtual int32_t getRawOffset() const;

    





    virtual UBool useDaylightTime() const;

    


    virtual UBool inDaylightTime(UDate date, UErrorCode& ec) const;

    


    virtual int32_t getDSTSavings() const;

    


    virtual UBool hasSameRules(const TimeZone& other) const;

    







    virtual UBool getNextTransition(UDate base, UBool inclusive, TimeZoneTransition& result) ;

    







    virtual UBool getPreviousTransition(UDate base, UBool inclusive, TimeZoneTransition& result) ;

    







    virtual int32_t countTransitionRules(UErrorCode& status) ;

    














    virtual void getTimeZoneRules(const InitialTimeZoneRule*& initial,
        const TimeZoneRule* trsrules[], int32_t& trscount, UErrorCode& status) ;

    



    const UChar *getCanonicalID() const;

private:
    



    OlsonTimeZone();

private:

    void constructEmpty();

    void getHistoricalOffset(UDate date, UBool local,
        int32_t NonExistingTimeOpt, int32_t DuplicatedTimeOpt,
        int32_t& rawoff, int32_t& dstoff) const;

    int16_t transitionCount() const;

    int64_t transitionTimeInSeconds(int16_t transIdx) const;
    double transitionTime(int16_t transIdx) const;

    



    int32_t zoneOffsetAt(int16_t transIdx) const;
    int32_t rawOffsetAt(int16_t transIdx) const;
    int32_t dstOffsetAt(int16_t transIdx) const;

    


    int32_t initialRawOffset() const;
    int32_t initialDstOffset() const;

    


    int16_t transitionCountPre32;
    int16_t transitionCount32;
    int16_t transitionCountPost32;

    




    const int32_t *transitionTimesPre32; 

    



    const int32_t *transitionTimes32; 

    




    const int32_t *transitionTimesPost32; 

    


    int16_t typeCount;

    




    const int32_t *typeOffsets; 

    




    const uint8_t *typeMapData; 

    


    SimpleTimeZone *finalZone; 

    


    double finalStartMillis;

    


    int32_t finalStartYear;

    


    const UChar *canonicalID;

    
    void clearTransitionRules(void);
    void deleteTransitionRules(void);
    void initTransitionRules(UErrorCode& status);

    InitialTimeZoneRule *initialRule;
    TimeZoneTransition  *firstTZTransition;
    int16_t             firstTZTransitionIdx;
    TimeZoneTransition  *firstFinalTZTransition;
    TimeArrayTimeZoneRule   **historicRules;
    int16_t             historicRuleCount;
    SimpleTimeZone      *finalZoneWithStartYear; 
    UBool               transitionRulesInitialized;
};

inline int16_t
OlsonTimeZone::transitionCount() const {
    return transitionCountPre32 + transitionCount32 + transitionCountPost32;
}

inline double
OlsonTimeZone::transitionTime(int16_t transIdx) const {
    return (double)transitionTimeInSeconds(transIdx) * U_MILLIS_PER_SECOND;
}

inline int32_t
OlsonTimeZone::zoneOffsetAt(int16_t transIdx) const {
    int16_t typeIdx = (transIdx >= 0 ? typeMapData[transIdx] : 0) << 1;
    return typeOffsets[typeIdx] + typeOffsets[typeIdx + 1];
}

inline int32_t
OlsonTimeZone::rawOffsetAt(int16_t transIdx) const {
    int16_t typeIdx = (transIdx >= 0 ? typeMapData[transIdx] : 0) << 1;
    return typeOffsets[typeIdx];
}

inline int32_t
OlsonTimeZone::dstOffsetAt(int16_t transIdx) const {
    int16_t typeIdx = (transIdx >= 0 ? typeMapData[transIdx] : 0) << 1;
    return typeOffsets[typeIdx + 1];
}

inline int32_t
OlsonTimeZone::initialRawOffset() const {
    return typeOffsets[0];
}

inline int32_t
OlsonTimeZone::initialDstOffset() const {
    return typeOffsets[1];
}

inline const UChar*
OlsonTimeZone::getCanonicalID() const {
    return canonicalID;
}


U_NAMESPACE_END

#endif 
#endif 


