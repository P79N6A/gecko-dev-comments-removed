























#ifndef CALENDAR_H
#define CALENDAR_H

#include "unicode/utypes.h"





#if !UCONFIG_NO_FORMATTING

#include "unicode/uobject.h"
#include "unicode/locid.h"
#include "unicode/timezone.h"
#include "unicode/ucal.h"
#include "unicode/umisc.h"

U_NAMESPACE_BEGIN

class ICUServiceFactory;




typedef int32_t UFieldResolutionTable[12][8];

class BasicTimeZone;




















































































































































class U_I18N_API Calendar : public UObject {
public:

    





    enum EDateFields {
#ifndef U_HIDE_DEPRECATED_API



#ifdef ERA
#undef ERA
#endif
        ERA,                  
        YEAR,                 
        MONTH,                
        WEEK_OF_YEAR,         
        WEEK_OF_MONTH,        
        DATE,                 
        DAY_OF_YEAR,          
        DAY_OF_WEEK,          
        DAY_OF_WEEK_IN_MONTH, 
        AM_PM,                
        HOUR,                 
        HOUR_OF_DAY,          
        MINUTE,               
        SECOND,               
        MILLISECOND,          
        ZONE_OFFSET,          
        DST_OFFSET,           
        YEAR_WOY,             
        DOW_LOCAL,            
        
        EXTENDED_YEAR,
        JULIAN_DAY,
        MILLISECONDS_IN_DAY,
        IS_LEAP_MONTH,

        FIELD_COUNT = UCAL_FIELD_COUNT 
#endif 
    };

#ifndef U_HIDE_DEPRECATED_API
    





    enum EDaysOfWeek {
        SUNDAY = 1,
        MONDAY,
        TUESDAY,
        WEDNESDAY,
        THURSDAY,
        FRIDAY,
        SATURDAY
    };

    



    enum EMonths {
        JANUARY,
        FEBRUARY,
        MARCH,
        APRIL,
        MAY,
        JUNE,
        JULY,
        AUGUST,
        SEPTEMBER,
        OCTOBER,
        NOVEMBER,
        DECEMBER,
        UNDECIMBER
    };

    



    enum EAmpm {
        AM,
        PM
    };
#endif  

    



    virtual ~Calendar();

    





    virtual Calendar* clone(void) const = 0;

    










    static Calendar* U_EXPORT2 createInstance(UErrorCode& success);

    











    static Calendar* U_EXPORT2 createInstance(TimeZone* zoneToAdopt, UErrorCode& success);

    










    static Calendar* U_EXPORT2 createInstance(const TimeZone& zone, UErrorCode& success);

    









    static Calendar* U_EXPORT2 createInstance(const Locale& aLocale, UErrorCode& success);

    












    static Calendar* U_EXPORT2 createInstance(TimeZone* zoneToAdopt, const Locale& aLocale, UErrorCode& success);

    











    static Calendar* U_EXPORT2 createInstance(const TimeZone& zone, const Locale& aLocale, UErrorCode& success);

    








    static const Locale* U_EXPORT2 getAvailableLocales(int32_t& count);


    















    static StringEnumeration* U_EXPORT2 getKeywordValuesForLocale(const char* key,
                    const Locale& locale, UBool commonlyUsed, UErrorCode& status);

    






    static UDate U_EXPORT2 getNow(void);

    












    inline UDate getTime(UErrorCode& status) const { return getTimeInMillis(status); }

    









    inline void setTime(UDate date, UErrorCode& status) { setTimeInMillis(date, status); }

    










    virtual UBool operator==(const Calendar& that) const;

    







    UBool operator!=(const Calendar& that) const {return !operator==(that);}

    









    virtual UBool isEquivalentTo(const Calendar& other) const;

    













    UBool equals(const Calendar& when, UErrorCode& status) const;

    












    UBool before(const Calendar& when, UErrorCode& status) const;

    












    UBool after(const Calendar& when, UErrorCode& status) const;

    



















    virtual void add(EDateFields field, int32_t amount, UErrorCode& status);

    



















    virtual void add(UCalendarDateFields field, int32_t amount, UErrorCode& status);

#ifndef U_HIDE_DEPRECATED_API
    































    inline void roll(EDateFields field, UBool up, UErrorCode& status);
#endif  

    































    inline void roll(UCalendarDateFields field, UBool up, UErrorCode& status);

    






























    virtual void roll(EDateFields field, int32_t amount, UErrorCode& status);

    






























    virtual void roll(UCalendarDateFields field, int32_t amount, UErrorCode& status);

    






















































    virtual int32_t fieldDifference(UDate when, EDateFields field, UErrorCode& status);

    






















































    virtual int32_t fieldDifference(UDate when, UCalendarDateFields field, UErrorCode& status);

    







    void adoptTimeZone(TimeZone* value);

    






    void setTimeZone(const TimeZone& zone);

    







    const TimeZone& getTimeZone(void) const;

    







    TimeZone* orphanTimeZone(void);

    







    virtual UBool inDaylightTime(UErrorCode& status) const = 0;

    











    void setLenient(UBool lenient);

    





    UBool isLenient(void) const;

#ifndef U_HIDE_DRAFT_API
    



















    void setRepeatedWallTimeOption(UCalendarWallTimeOption option);

    








    UCalendarWallTimeOption getRepeatedWallTimeOption(void) const;

    




















    void setSkippedWallTimeOption(UCalendarWallTimeOption option);

    









    UCalendarWallTimeOption getSkippedWallTimeOption(void) const;
#endif  

#ifndef U_HIDE_DEPRECATED_API
    





    void setFirstDayOfWeek(EDaysOfWeek value);
#endif  

    





    void setFirstDayOfWeek(UCalendarDaysOfWeek value);

#ifndef U_HIDE_DEPRECATED_API
    





    EDaysOfWeek getFirstDayOfWeek(void) const;
#endif  

    






    UCalendarDaysOfWeek getFirstDayOfWeek(UErrorCode &status) const;

    








    void setMinimalDaysInFirstWeek(uint8_t value);

    








    uint8_t getMinimalDaysInFirstWeek(void) const;

    







    virtual int32_t getMinimum(EDateFields field) const;

    







    virtual int32_t getMinimum(UCalendarDateFields field) const;

    







    virtual int32_t getMaximum(EDateFields field) const;

    







    virtual int32_t getMaximum(UCalendarDateFields field) const;

    







    virtual int32_t getGreatestMinimum(EDateFields field) const;

    







    virtual int32_t getGreatestMinimum(UCalendarDateFields field) const;

    







    virtual int32_t getLeastMaximum(EDateFields field) const;

    







    virtual int32_t getLeastMaximum(UCalendarDateFields field) const;

#ifndef U_HIDE_DEPRECATED_API
    













    int32_t getActualMinimum(EDateFields field, UErrorCode& status) const;
#endif  

    













    virtual int32_t getActualMinimum(UCalendarDateFields field, UErrorCode& status) const;

#ifndef U_HIDE_DEPRECATED_API
    















    int32_t getActualMaximum(EDateFields field, UErrorCode& status) const;
#endif  

    















    virtual int32_t getActualMaximum(UCalendarDateFields field, UErrorCode& status) const;

#ifndef U_HIDE_DEPRECATED_API
    












    int32_t get(EDateFields field, UErrorCode& status) const;
#endif  

    












    int32_t get(UCalendarDateFields field, UErrorCode& status) const;

#ifndef U_HIDE_DEPRECATED_API
    







    UBool isSet(EDateFields field) const;
#endif  

    







    UBool isSet(UCalendarDateFields field) const;

#ifndef U_HIDE_DEPRECATED_API
    






    void set(EDateFields field, int32_t value);
#endif  

    






    void set(UCalendarDateFields field, int32_t value);

    









    void set(int32_t year, int32_t month, int32_t date);

    











    void set(int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute);

    












    void set(int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, int32_t second);

    





    void clear(void);

#ifndef U_HIDE_DEPRECATED_API
    







    void clear(EDateFields field);
#endif  

    







    void clear(UCalendarDateFields field);

    














    virtual UClassID getDynamicClassID(void) const = 0;

    































    virtual const char * getType() const = 0;

    














    virtual UCalendarWeekdayType getDayOfWeekType(UCalendarDaysOfWeek dayOfWeek, UErrorCode &status) const;

    













    virtual int32_t getWeekendTransition(UCalendarDaysOfWeek dayOfWeek, UErrorCode &status) const;

    








    virtual UBool isWeekend(UDate date, UErrorCode &status) const;

    






    virtual UBool isWeekend(void) const;

protected:

     







    Calendar(UErrorCode& success);

    





    Calendar(const Calendar& source);

    





    Calendar& operator=(const Calendar& right);

    









    Calendar(TimeZone* zone, const Locale& aLocale, UErrorCode& success);

    








    Calendar(const TimeZone& zone, const Locale& aLocale, UErrorCode& success);

    







    virtual void computeTime(UErrorCode& status);

    










    virtual void computeFields(UErrorCode& status);

    








    double getTimeInMillis(UErrorCode& status) const;

    







    void setTimeInMillis( double millis, UErrorCode& status );

    








    void complete(UErrorCode& status);

#ifndef U_HIDE_DEPRECATED_API
    







    inline int32_t internalGet(EDateFields field) const {return fFields[field];}
#endif  

#ifndef U_HIDE_INTERNAL_API
    









    inline int32_t internalGet(UCalendarDateFields field, int32_t defaultValue) const {return fStamp[field]>kUnset ? fFields[field] : defaultValue;}

    







    inline int32_t internalGet(UCalendarDateFields field) const {return fFields[field];}
#endif  

#ifndef U_HIDE_DEPRECATED_API
    








    void internalSet(EDateFields field, int32_t value);
#endif  

    








    inline void internalSet(UCalendarDateFields field, int32_t value);

    





    virtual void prepareGetActual(UCalendarDateFields field, UBool isMinimum, UErrorCode &status);

    



    enum ELimitType {
      UCAL_LIMIT_MINIMUM = 0,
      UCAL_LIMIT_GREATEST_MINIMUM,
      UCAL_LIMIT_LEAST_MAXIMUM,
      UCAL_LIMIT_MAXIMUM,
      UCAL_LIMIT_COUNT
    };

    




















    virtual int32_t handleGetLimit(UCalendarDateFields field, ELimitType limitType) const = 0;

    






    virtual int32_t getLimit(UCalendarDateFields field, ELimitType limitType) const;


    












    virtual int32_t handleComputeMonthStart(int32_t eyear, int32_t month,
                                                   UBool useMonth) const  = 0;

    






    virtual int32_t handleGetMonthLength(int32_t extendedYear, int32_t month) const ;

    






    virtual int32_t handleGetYearLength(int32_t eyear) const;


    







    virtual int32_t handleGetExtendedYear() = 0;

    







    virtual int32_t handleComputeJulianDay(UCalendarDateFields bestField);

    







    virtual int32_t handleGetExtendedYearFromWeekFields(int32_t yearWoy, int32_t woy);

#ifndef U_HIDE_INTERNAL_API
    





    int32_t computeJulianDay();

    






    int32_t computeMillisInDay();

    








    int32_t computeZoneOffset(double millis, int32_t millisInDay, UErrorCode &ec);


    







    int32_t newestStamp(UCalendarDateFields start, UCalendarDateFields end, int32_t bestSoFar) const;

    




    enum {
      
      kResolveSTOP = -1,
      
      kResolveRemap = 32
    };

    




    static const UFieldResolutionTable kDatePrecedence[];

    




    static const UFieldResolutionTable kYearPrecedence[];

    




    static const UFieldResolutionTable kDOWPrecedence[];

    


























    UCalendarDateFields resolveFields(const UFieldResolutionTable *precedenceTable);
#endif  


    


    virtual const UFieldResolutionTable* getFieldResolutionTable() const;

#ifndef U_HIDE_INTERNAL_API
    




    UCalendarDateFields newerField(UCalendarDateFields defaultField, UCalendarDateFields alternateField) const;
#endif  


private:
    







    int32_t getActualHelper(UCalendarDateFields field, int32_t startValue, int32_t endValue, UErrorCode &status) const;


protected:
    



    UBool      fIsTimeSet;

    









    UBool      fAreFieldsSet;

    




    UBool      fAreAllFieldsSet;

    






    UBool fAreFieldsVirtuallySet;

    





    UDate        internalGetTime(void) const     { return fTime; }

    






    void        internalSetTime(UDate time)     { fTime = time; }

    



    int32_t     fFields[UCAL_FIELD_COUNT];

    



    UBool      fIsSet[UCAL_FIELD_COUNT];

    


    enum {
        kUnset                 = 0,
        kInternallySet,
        kMinimumUserStamp
    };

    





    int32_t        fStamp[UCAL_FIELD_COUNT];

    























    virtual void handleComputeFields(int32_t julianDay, UErrorCode &status);

#ifndef U_HIDE_INTERNAL_API
    




    int32_t getGregorianYear() const {
        return fGregorianYear;
    }

    




    int32_t getGregorianMonth() const {
        return fGregorianMonth;
    }

    




    int32_t getGregorianDayOfYear() const {
        return fGregorianDayOfYear;
    }

    




    int32_t getGregorianDayOfMonth() const {
      return fGregorianDayOfMonth;
    }
#endif  

    





    virtual int32_t getDefaultMonthInYear(int32_t eyear) ;


    






    virtual int32_t getDefaultDayInMonth(int32_t eyear, int32_t month);

    
    
    
    

    




























    virtual void pinField(UCalendarDateFields field, UErrorCode& status);

    










































    int32_t weekNumber(int32_t desiredDay, int32_t dayOfPeriod, int32_t dayOfWeek);


#ifndef U_HIDE_INTERNAL_API
    





























    inline int32_t weekNumber(int32_t dayOfPeriod, int32_t dayOfWeek);

    



    int32_t getLocalDOW();
#endif  

private:

    


    int32_t fNextStamp;

    



    void recalculateStamp();

    


    UDate        fTime;

    


    UBool      fLenient;

    



    TimeZone*   fZone;

    



    UCalendarWallTimeOption fRepeatedWallTime;

    



    UCalendarWallTimeOption fSkippedWallTime;

    







    UCalendarDaysOfWeek fFirstDayOfWeek;
    uint8_t     fMinimalDaysInFirstWeek;
    UCalendarDaysOfWeek fWeekendOnset;
    int32_t fWeekendOnsetMillis;
    UCalendarDaysOfWeek fWeekendCease;
    int32_t fWeekendCeaseMillis;

    









    void        setWeekData(const Locale& desiredLocale, const char *type, UErrorCode& success);

    








    void updateTime(UErrorCode& status);

    




    int32_t fGregorianYear;

    




    int32_t fGregorianMonth;

    




    int32_t fGregorianDayOfYear;

    




    int32_t fGregorianDayOfMonth;

    

    





    void computeGregorianAndDOWFields(int32_t julianDay, UErrorCode &ec);

protected:

    






    void computeGregorianFields(int32_t julianDay, UErrorCode &ec);

private:

    



















    void computeWeekFields(UErrorCode &ec);


    







    void validateFields(UErrorCode &status);

    







    virtual void validateField(UCalendarDateFields field, UErrorCode &status);

    







    void validateField(UCalendarDateFields field, int32_t min, int32_t max, UErrorCode& status);

 protected:
#ifndef U_HIDE_INTERNAL_API
    








    static uint8_t julianDayToDayOfWeek(double julian);
#endif  

 private:
    char validLocale[ULOC_FULLNAME_CAPACITY];
    char actualLocale[ULOC_FULLNAME_CAPACITY];

 public:
#if !UCONFIG_NO_SERVICE
    



#ifndef U_HIDE_INTERNAL_API
    





    static StringEnumeration* getAvailableLocales(void);

    







    static URegistryKey registerFactory(ICUServiceFactory* toAdopt, UErrorCode& status);

    









    static UBool unregister(URegistryKey key, UErrorCode& status);
#endif  

    



    friend class CalendarFactory;

    



    friend class CalendarService;

    



    friend class DefaultCalendarFactory;
#endif 

    



    virtual UBool haveDefaultCentury() const = 0;

    



    virtual UDate defaultCenturyStart() const = 0;
    



    virtual int32_t defaultCenturyStartYear() const = 0;

    





    Locale getLocale(ULocDataLocaleType type, UErrorCode &status) const;

#ifndef U_HIDE_INTERNAL_API
    





    const char* getLocaleID(ULocDataLocaleType type, UErrorCode &status) const;
#endif  

private:
    



    BasicTimeZone* getBasicTimeZone() const;
};



inline Calendar*
Calendar::createInstance(TimeZone* zone, UErrorCode& errorCode)
{
    
    return createInstance(zone, Locale::getDefault(), errorCode);
}



inline void
Calendar::roll(UCalendarDateFields field, UBool up, UErrorCode& status)
{
    roll(field, (int32_t)(up ? +1 : -1), status);
}

#ifndef U_HIDE_DEPRECATED_API
inline void
Calendar::roll(EDateFields field, UBool up, UErrorCode& status)
{
    roll((UCalendarDateFields) field, up, status);
}
#endif









inline void
Calendar::internalSet(UCalendarDateFields field, int32_t value)
{
    fFields[field] = value;
    fStamp[field] = kInternallySet;
    fIsSet[field]     = TRUE; 
}


#ifndef U_HIDE_INTERNAL_API
inline int32_t  Calendar::weekNumber(int32_t dayOfPeriod, int32_t dayOfWeek)
{
  return weekNumber(dayOfPeriod, dayOfPeriod, dayOfWeek);
}
#endif

U_NAMESPACE_END

#endif 

#endif
