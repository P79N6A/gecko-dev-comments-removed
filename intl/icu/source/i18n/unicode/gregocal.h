






















#ifndef GREGOCAL_H
#define GREGOCAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"






U_NAMESPACE_BEGIN














































































































class U_I18N_API GregorianCalendar: public Calendar {
public:

    



    enum EEras {
        BC,
        AD
    };

    







    GregorianCalendar(UErrorCode& success);

    









    GregorianCalendar(TimeZone* zoneToAdopt, UErrorCode& success);

    








    GregorianCalendar(const TimeZone& zone, UErrorCode& success);

    








    GregorianCalendar(const Locale& aLocale, UErrorCode& success);

    










    GregorianCalendar(TimeZone* zoneToAdopt, const Locale& aLocale, UErrorCode& success);

    









    GregorianCalendar(const TimeZone& zone, const Locale& aLocale, UErrorCode& success);

    











    GregorianCalendar(int32_t year, int32_t month, int32_t date, UErrorCode& success);

    













    GregorianCalendar(int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, UErrorCode& success);

    














    GregorianCalendar(int32_t year, int32_t month, int32_t date, int32_t hour, int32_t minute, int32_t second, UErrorCode& success);

    



    virtual ~GregorianCalendar();

    




    GregorianCalendar(const GregorianCalendar& source);

    




    GregorianCalendar& operator=(const GregorianCalendar& right);

    




    virtual Calendar* clone(void) const;

    








    void setGregorianChange(UDate date, UErrorCode& success);

    







    UDate getGregorianChange(void) const;

    














    UBool isLeapYear(int32_t year) const;

    






    virtual UBool isEquivalentTo(const Calendar& other) const;

    










    virtual void roll(EDateFields field, int32_t amount, UErrorCode& status);

    










    virtual void roll(UCalendarDateFields field, int32_t amount, UErrorCode& status);

#ifndef U_HIDE_DEPRECATED_API
    






    int32_t getActualMinimum(EDateFields field) const;

    







    int32_t getActualMinimum(EDateFields field, UErrorCode& status) const;
#endif  

    







    int32_t getActualMinimum(UCalendarDateFields field, UErrorCode &status) const;

#ifndef U_HIDE_DEPRECATED_API
    








    int32_t getActualMaximum(EDateFields field) const;
#endif  

    









    virtual int32_t getActualMaximum(UCalendarDateFields field, UErrorCode& status) const;

    








    virtual UBool inDaylightTime(UErrorCode& status) const;

public:

    









    virtual UClassID getDynamicClassID(void) const;

    










    static UClassID U_EXPORT2 getStaticClassID(void);


    









    virtual const char * getType() const;

 private:
    GregorianCalendar(); 

 protected:
    





    virtual int32_t internalGetEra() const;

    












    virtual int32_t handleComputeMonthStart(int32_t eyear, int32_t month,
                                                   UBool useMonth) const;

    







    virtual int32_t handleComputeJulianDay(UCalendarDateFields bestField)  ;

    






    virtual int32_t handleGetMonthLength(int32_t extendedYear, int32_t month) const;

    






    virtual int32_t handleGetYearLength(int32_t eyear) const;

    





    virtual int32_t monthLength(int32_t month) const;

    






    virtual int32_t monthLength(int32_t month, int32_t year) const;

#ifndef U_HIDE_INTERNAL_API
    





    int32_t yearLength(int32_t year) const;
    
    




    int32_t yearLength(void) const;

    






    void pinDayOfMonth(void);
#endif  

    






    virtual UDate getEpochDay(UErrorCode& status);

    




















    virtual int32_t handleGetLimit(UCalendarDateFields field, ELimitType limitType) const;

    







    virtual int32_t handleGetExtendedYear();

    







    virtual int32_t handleGetExtendedYearFromWeekFields(int32_t yearWoy, int32_t woy);


    














    virtual void handleComputeFields(int32_t julianDay, UErrorCode &status);

 private:
    






    static double computeJulianDayOfYear(UBool isGregorian, int32_t year,
                                         UBool& isLeap);
    
    



    UBool validateFields(void) const;

    


    UBool boundsCheck(int32_t value, UCalendarDateFields field) const;

    








    int32_t aggregateStamp(int32_t stamp_a, int32_t stamp_b);

    







    UDate                fGregorianCutover;

    


    int32_t             fCutoverJulianDay;

    




    UDate                 fNormalizedGregorianCutover;

    



    int32_t fGregorianCutoverYear;

    



    int32_t fGregorianCutoverJulianDay;

    






    static double millisToJulianDay(UDate millis);

    






    static UDate julianDayToMillis(double julian);

    



    UBool fIsGregorian;

    




    UBool fInvertGregorian;


 public: 

    



    virtual UBool haveDefaultCentury() const;

    



    virtual UDate defaultCenturyStart() const;

    



    virtual int32_t defaultCenturyStartYear() const;

 private:
    





    static UDate         fgSystemDefaultCenturyStart;

    


    static int32_t          fgSystemDefaultCenturyStartYear;

    


    static const int32_t    fgSystemDefaultCenturyYear;

    


    static const UDate        fgSystemDefaultCentury;

    





    UDate         internalGetDefaultCenturyStart(void) const;

    





    int32_t          internalGetDefaultCenturyStartYear(void) const;

    



    static void  initializeSystemDefaultCentury(void);

};

U_NAMESPACE_END

#endif

#endif


