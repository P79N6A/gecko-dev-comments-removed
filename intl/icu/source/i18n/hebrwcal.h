















#ifndef HEBRWCAL_H
#define HEBRWCAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/gregocal.h"

U_NAMESPACE_BEGIN























































class U_I18N_API HebrewCalendar : public Calendar {
public:
  



  enum EEras {
    


      TISHRI,
      


      HESHVAN,
      


      KISLEV,

    


      TEVET,

    


      SHEVAT,

    




      ADAR_1,

    


      ADAR,

    


      NISAN,

    


      IYAR,

    


      SIVAN,

    


      TAMUZ,

    


      AV,

    


      ELUL
    };

    








    HebrewCalendar(const Locale& aLocale, UErrorCode& success);


    



    virtual ~HebrewCalendar();

    




    HebrewCalendar(const HebrewCalendar& source);

    




    HebrewCalendar& operator=(const HebrewCalendar& right);

    




    virtual Calendar* clone(void) const;
    
public:
    









    virtual UClassID getDynamicClassID(void) const;

    










    static UClassID U_EXPORT2 getStaticClassID(void);

    





    virtual const char * getType() const;


    
 public:
    










    virtual void add(UCalendarDateFields field, int32_t amount, UErrorCode& status);
    


    virtual void add(EDateFields field, int32_t amount, UErrorCode& status);


    










    virtual void roll(UCalendarDateFields field, int32_t amount, UErrorCode& status);

    










    virtual void roll(EDateFields field, int32_t amount, UErrorCode& status);

    


    static UBool isLeapYear(int32_t year) ;

 protected:

    




















    virtual int32_t handleGetLimit(UCalendarDateFields field, ELimitType limitType) const;

    






    virtual int32_t handleGetMonthLength(int32_t extendedYear, int32_t month) const;

    






    virtual int32_t handleGetYearLength(int32_t eyear) const;
    














    virtual void handleComputeFields(int32_t julianDay, UErrorCode &status);
    







    virtual int32_t handleGetExtendedYear();
    












    virtual int32_t handleComputeMonthStart(int32_t eyear, int32_t month,
                                                   UBool useMonth) const;



 protected:

  








  virtual UBool inDaylightTime(UErrorCode& status) const;

    



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

 private: 
    






















    static int32_t startOfYear(int32_t year, UErrorCode& status);

    static int32_t absoluteDayToDayOfWeek(int32_t day) ;
    
    


    int32_t yearType(int32_t year) const;

    


    static int32_t monthsInYear(int32_t year) ;
};

U_NAMESPACE_END

#endif

#endif 


