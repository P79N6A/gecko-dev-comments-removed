





#ifndef TZRULE_H
#define TZRULE_H






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/dtrule.h"

U_NAMESPACE_BEGIN








class U_I18N_API TimeZoneRule : public UObject {
public:
    



    virtual ~TimeZoneRule();

    





    virtual TimeZoneRule* clone(void) const = 0;

    






    virtual UBool operator==(const TimeZoneRule& that) const;

    






    virtual UBool operator!=(const TimeZoneRule& that) const;

    





    UnicodeString& getName(UnicodeString& name) const;

    




    int32_t getRawOffset(void) const;

    





    int32_t getDSTSavings(void) const;

    







    virtual UBool isEquivalentTo(const TimeZoneRule& other) const;

    










    virtual UBool getFirstStart(int32_t prevRawOffset, int32_t prevDSTSavings, UDate& result) const = 0;

    










    virtual UBool getFinalStart(int32_t prevRawOffset, int32_t prevDSTSavings, UDate& result) const = 0;

    













    virtual UBool getNextStart(UDate base, int32_t prevRawOffset, int32_t prevDSTSavings,
        UBool inclusive, UDate& result) const = 0;

    













    virtual UBool getPreviousStart(UDate base, int32_t prevRawOffset, int32_t prevDSTSavings,
        UBool inclusive, UDate& result) const = 0;

protected:

    








    TimeZoneRule(const UnicodeString& name, int32_t rawOffset, int32_t dstSavings);

    




    TimeZoneRule(const TimeZoneRule& source);

    




    TimeZoneRule& operator=(const TimeZoneRule& right);

private:
    UnicodeString fName; 
    int32_t fRawOffset;  
    int32_t fDSTSavings; 
};







class U_I18N_API InitialTimeZoneRule : public TimeZoneRule {
public:
    








    InitialTimeZoneRule(const UnicodeString& name, int32_t rawOffset, int32_t dstSavings);

    




    InitialTimeZoneRule(const InitialTimeZoneRule& source);

    



    virtual ~InitialTimeZoneRule();

    





    virtual InitialTimeZoneRule* clone(void) const;

    




    InitialTimeZoneRule& operator=(const InitialTimeZoneRule& right);

    






    virtual UBool operator==(const TimeZoneRule& that) const;

    






    virtual UBool operator!=(const TimeZoneRule& that) const;

    











    UBool getStartInYear(int32_t year, int32_t prevRawOffset, int32_t prevDSTSavings, UDate& result) const;

    







    virtual UBool isEquivalentTo(const TimeZoneRule& that) const;

    










    virtual UBool getFirstStart(int32_t prevRawOffset, int32_t prevDSTSavings, UDate& result) const;

    










    virtual UBool getFinalStart(int32_t prevRawOffset, int32_t prevDSTSavings, UDate& result) const;

    













    virtual UBool getNextStart(UDate base, int32_t prevRawOffset, int32_t prevDSTSavings,
        UBool inclusive, UDate& result) const;

    













    virtual UBool getPreviousStart(UDate base, int32_t prevRawOffset, int32_t prevDSTSavings,
        UBool inclusive, UDate& result) const;

public:
    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;
};








class U_I18N_API AnnualTimeZoneRule : public TimeZoneRule {
public:
    



    static const int32_t MAX_YEAR;

    















    AnnualTimeZoneRule(const UnicodeString& name, int32_t rawOffset, int32_t dstSavings,
            const DateTimeRule& dateTimeRule, int32_t startYear, int32_t endYear);

    















    AnnualTimeZoneRule(const UnicodeString& name, int32_t rawOffset, int32_t dstSavings,
            DateTimeRule* dateTimeRule, int32_t startYear, int32_t endYear);

    




    AnnualTimeZoneRule(const AnnualTimeZoneRule& source);

    



    virtual ~AnnualTimeZoneRule();

    





    virtual AnnualTimeZoneRule* clone(void) const;

    




    AnnualTimeZoneRule& operator=(const AnnualTimeZoneRule& right);

    






    virtual UBool operator==(const TimeZoneRule& that) const;

    






    virtual UBool operator!=(const TimeZoneRule& that) const;

    





    const DateTimeRule* getRule(void) const;

    





    int32_t getStartYear(void) const;

    





    int32_t getEndYear(void) const;

    











    UBool getStartInYear(int32_t year, int32_t prevRawOffset, int32_t prevDSTSavings, UDate& result) const;

    







    virtual UBool isEquivalentTo(const TimeZoneRule& that) const;

    










    virtual UBool getFirstStart(int32_t prevRawOffset, int32_t prevDSTSavings, UDate& result) const;

    










    virtual UBool getFinalStart(int32_t prevRawOffset, int32_t prevDSTSavings, UDate& result) const;

    













    virtual UBool getNextStart(UDate base, int32_t prevRawOffset, int32_t prevDSTSavings,
        UBool inclusive, UDate& result) const;

    













    virtual UBool getPreviousStart(UDate base, int32_t prevRawOffset, int32_t prevDSTSavings,
        UBool inclusive, UDate& result) const;


private:
    DateTimeRule* fDateTimeRule;
    int32_t fStartYear;
    int32_t fEndYear;

public:
    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;
};







class U_I18N_API TimeArrayTimeZoneRule : public TimeZoneRule {
public:
    
















    TimeArrayTimeZoneRule(const UnicodeString& name, int32_t rawOffset, int32_t dstSavings,
        const UDate* startTimes, int32_t numStartTimes, DateTimeRule::TimeRuleType timeRuleType);

    




    TimeArrayTimeZoneRule(const TimeArrayTimeZoneRule& source);

    



    virtual ~TimeArrayTimeZoneRule();

    





    virtual TimeArrayTimeZoneRule* clone(void) const;

    




    TimeArrayTimeZoneRule& operator=(const TimeArrayTimeZoneRule& right);

    






    virtual UBool operator==(const TimeZoneRule& that) const;

    






    virtual UBool operator!=(const TimeZoneRule& that) const;

    







    DateTimeRule::TimeRuleType getTimeType(void) const;

    








    UBool getStartTimeAt(int32_t index, UDate& result) const;

    




    int32_t countStartTimes(void) const;

    







    virtual UBool isEquivalentTo(const TimeZoneRule& that) const;

    










    virtual UBool getFirstStart(int32_t prevRawOffset, int32_t prevDSTSavings, UDate& result) const;

    










    virtual UBool getFinalStart(int32_t prevRawOffset, int32_t prevDSTSavings, UDate& result) const;

    













    virtual UBool getNextStart(UDate base, int32_t prevRawOffset, int32_t prevDSTSavings,
        UBool inclusive, UDate& result) const;

    













    virtual UBool getPreviousStart(UDate base, int32_t prevRawOffset, int32_t prevDSTSavings,
        UBool inclusive, UDate& result) const;


private:
    enum { TIMEARRAY_STACK_BUFFER_SIZE = 32 };
    UBool initStartTimes(const UDate source[], int32_t size, UErrorCode& ec);
    UDate getUTC(UDate time, int32_t raw, int32_t dst) const;

    DateTimeRule::TimeRuleType  fTimeRuleType;
    int32_t fNumStartTimes;
    UDate*  fStartTimes;
    UDate   fLocalStartTimes[TIMEARRAY_STACK_BUFFER_SIZE];

public:
    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;
};


U_NAMESPACE_END

#endif

#endif


