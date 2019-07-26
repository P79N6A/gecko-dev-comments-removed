






















#ifndef SIMPLETZ_H
#define SIMPLETZ_H

#include "unicode/utypes.h"





 
#if !UCONFIG_NO_FORMATTING

#include "unicode/basictz.h"

U_NAMESPACE_BEGIN


class InitialTimeZoneRule;
class TimeZoneTransition;
class AnnualTimeZoneRule;

















class U_I18N_API SimpleTimeZone: public BasicTimeZone {
public:

    










    enum TimeMode {
        WALL_TIME = 0,
        STANDARD_TIME,
        UTC_TIME
    };

    




    SimpleTimeZone(const SimpleTimeZone& source);

    




    SimpleTimeZone& operator=(const SimpleTimeZone& right);

    



    virtual ~SimpleTimeZone();

    








    virtual UBool operator==(const TimeZone& that) const;

    










    SimpleTimeZone(int32_t rawOffsetGMT, const UnicodeString& ID);

    


































    SimpleTimeZone(int32_t rawOffsetGMT, const UnicodeString& ID,
        int8_t savingsStartMonth, int8_t savingsStartDayOfWeekInMonth,
        int8_t savingsStartDayOfWeek, int32_t savingsStartTime,
        int8_t savingsEndMonth, int8_t savingsEndDayOfWeekInMonth,
        int8_t savingsEndDayOfWeek, int32_t savingsEndTime,
        UErrorCode& status);
    




































    SimpleTimeZone(int32_t rawOffsetGMT, const UnicodeString& ID,
        int8_t savingsStartMonth, int8_t savingsStartDayOfWeekInMonth,
        int8_t savingsStartDayOfWeek, int32_t savingsStartTime,
        int8_t savingsEndMonth, int8_t savingsEndDayOfWeekInMonth,
        int8_t savingsEndDayOfWeek, int32_t savingsEndTime,
        int32_t savingsDST, UErrorCode& status);

    








































    SimpleTimeZone(int32_t rawOffsetGMT, const UnicodeString& ID,
        int8_t savingsStartMonth, int8_t savingsStartDayOfWeekInMonth,
        int8_t savingsStartDayOfWeek, int32_t savingsStartTime,
        TimeMode savingsStartTimeMode,
        int8_t savingsEndMonth, int8_t savingsEndDayOfWeekInMonth,
        int8_t savingsEndDayOfWeek, int32_t savingsEndTime, TimeMode savingsEndTimeMode,
        int32_t savingsDST, UErrorCode& status);

    







    void setStartYear(int32_t year);

    









































    void setStartRule(int32_t month, int32_t dayOfWeekInMonth, int32_t dayOfWeek,
                      int32_t time, UErrorCode& status);
    











































    void setStartRule(int32_t month, int32_t dayOfWeekInMonth, int32_t dayOfWeek,
                      int32_t time, TimeMode mode, UErrorCode& status);

    










    void setStartRule(int32_t month, int32_t dayOfMonth, int32_t time,
                      UErrorCode& status);
    












    void setStartRule(int32_t month, int32_t dayOfMonth, int32_t time,
                      TimeMode mode, UErrorCode& status);

    















    void setStartRule(int32_t month, int32_t dayOfMonth, int32_t dayOfWeek,
                      int32_t time, UBool after, UErrorCode& status);
    

















    void setStartRule(int32_t month, int32_t dayOfMonth, int32_t dayOfWeek,
                      int32_t time, TimeMode mode, UBool after, UErrorCode& status);

    





















    void setEndRule(int32_t month, int32_t dayOfWeekInMonth, int32_t dayOfWeek,
                    int32_t time, UErrorCode& status);

    























    void setEndRule(int32_t month, int32_t dayOfWeekInMonth, int32_t dayOfWeek,
                    int32_t time, TimeMode mode, UErrorCode& status);

    










    void setEndRule(int32_t month, int32_t dayOfMonth, int32_t time, UErrorCode& status);

    












    void setEndRule(int32_t month, int32_t dayOfMonth, int32_t time,
                    TimeMode mode, UErrorCode& status);

    















    void setEndRule(int32_t month, int32_t dayOfMonth, int32_t dayOfWeek,
                    int32_t time, UBool after, UErrorCode& status);

    

















    void setEndRule(int32_t month, int32_t dayOfMonth, int32_t dayOfWeek,
                    int32_t time, TimeMode mode, UBool after, UErrorCode& status);

    



















    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                              uint8_t dayOfWeek, int32_t millis, UErrorCode& status) const;

    














    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                           uint8_t dayOfWeek, int32_t milliseconds,
                           int32_t monthLength, UErrorCode& status) const;
    















    virtual int32_t getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                              uint8_t dayOfWeek, int32_t milliseconds,
                              int32_t monthLength, int32_t prevMonthLength,
                              UErrorCode& status) const;

    




    virtual void getOffset(UDate date, UBool local, int32_t& rawOffset,
                           int32_t& dstOffset, UErrorCode& ec) const;

    



    virtual void getOffsetFromLocal(UDate date, int32_t nonExistingTimeOpt, int32_t duplicatedTimeOpt,
        int32_t& rawOffset, int32_t& dstOffset, UErrorCode& status) ;

    






    virtual int32_t getRawOffset(void) const;

    






    virtual void setRawOffset(int32_t offsetMillis);

    







    void setDSTSavings(int32_t millisSavedDuringDST, UErrorCode& status);

    






    virtual int32_t getDSTSavings(void) const;

    





    virtual UBool useDaylightTime(void) const;

    













    virtual UBool inDaylightTime(UDate date, UErrorCode& status) const;

    





    UBool hasSameRules(const TimeZone& other) const;

    






    virtual TimeZone* clone(void) const;

    







    virtual UBool getNextTransition(UDate base, UBool inclusive, TimeZoneTransition& result) ;

    







    virtual UBool getPreviousTransition(UDate base, UBool inclusive, TimeZoneTransition& result) ;

    







    virtual int32_t countTransitionRules(UErrorCode& status) ;

    















    virtual void getTimeZoneRules(const InitialTimeZoneRule*& initial,
        const TimeZoneRule* trsrules[], int32_t& trscount, UErrorCode& status) ;


public:

    









    virtual UClassID getDynamicClassID(void) const;

    










    static UClassID U_EXPORT2 getStaticClassID(void);

private:
    


    enum EMode
    {
        DOM_MODE = 1,
        DOW_IN_MONTH_MODE,
        DOW_GE_DOM_MODE,
        DOW_LE_DOM_MODE
    };

    SimpleTimeZone(); 

    


















    void construct(int32_t rawOffsetGMT,
                   int8_t startMonth, int8_t startDay, int8_t startDayOfWeek,
                   int32_t startTime, TimeMode startTimeMode,
                   int8_t endMonth, int8_t endDay, int8_t endDayOfWeek,
                   int32_t endTime, TimeMode endTimeMode,
                   int32_t dstSavings, UErrorCode& status);

    








    static int32_t compareToRule(int8_t month, int8_t monthLen, int8_t prevMonthLen,
                                 int8_t dayOfMonth,
                                 int8_t dayOfWeek, int32_t millis, int32_t millisDelta,
                                 EMode ruleMode, int8_t ruleMonth, int8_t ruleDayOfWeek,
                                 int8_t ruleDay, int32_t ruleMillis);

    














    void decodeRules(UErrorCode& status);
    void decodeStartRule(UErrorCode& status);
    void decodeEndRule(UErrorCode& status);

    int8_t startMonth, startDay, startDayOfWeek;   
    int32_t startTime;
    TimeMode startTimeMode, endTimeMode; 
    int8_t endMonth, endDay, endDayOfWeek; 
    int32_t endTime;
    int32_t startYear;  
    int32_t rawOffset;  
    UBool useDaylight; 
    static const int8_t STATICMONTHLENGTH[12]; 
    EMode startMode, endMode;   

    



    int32_t dstSavings;

    
    void initTransitionRules(UErrorCode& status);
    void clearTransitionRules(void);
    void deleteTransitionRules(void);
    UBool   transitionRulesInitialized;
    InitialTimeZoneRule*    initialRule;
    TimeZoneTransition*     firstTransition;
    AnnualTimeZoneRule*     stdRule;
    AnnualTimeZoneRule*     dstRule;
};

inline void SimpleTimeZone::setStartRule(int32_t month, int32_t dayOfWeekInMonth,
                                         int32_t dayOfWeek,
                                         int32_t time, UErrorCode& status) {
    setStartRule(month, dayOfWeekInMonth, dayOfWeek, time, WALL_TIME, status);
}

inline void SimpleTimeZone::setStartRule(int32_t month, int32_t dayOfMonth,
                                         int32_t time,
                                         UErrorCode& status) {
    setStartRule(month, dayOfMonth, time, WALL_TIME, status);
}

inline void SimpleTimeZone::setStartRule(int32_t month, int32_t dayOfMonth,
                                         int32_t dayOfWeek,
                                         int32_t time, UBool after, UErrorCode& status) {
    setStartRule(month, dayOfMonth, dayOfWeek, time, WALL_TIME, after, status);
}

inline void SimpleTimeZone::setEndRule(int32_t month, int32_t dayOfWeekInMonth,
                                       int32_t dayOfWeek,
                                       int32_t time, UErrorCode& status) {
    setEndRule(month, dayOfWeekInMonth, dayOfWeek, time, WALL_TIME, status);
}

inline void SimpleTimeZone::setEndRule(int32_t month, int32_t dayOfMonth,
                                       int32_t time, UErrorCode& status) {
    setEndRule(month, dayOfMonth, time, WALL_TIME, status);
}

inline void SimpleTimeZone::setEndRule(int32_t month, int32_t dayOfMonth, int32_t dayOfWeek,
                                       int32_t time, UBool after, UErrorCode& status) {
    setEndRule(month, dayOfMonth, dayOfWeek, time, WALL_TIME, after, status);
}

inline void
SimpleTimeZone::getOffset(UDate date, UBool local, int32_t& rawOffsetRef,
                          int32_t& dstOffsetRef, UErrorCode& ec) const {
    TimeZone::getOffset(date, local, rawOffsetRef, dstOffsetRef, ec);
}

U_NAMESPACE_END

#endif

#endif
