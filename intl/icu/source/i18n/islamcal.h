














#ifndef ISLAMCAL_H
#define ISLAMCAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"

U_NAMESPACE_BEGIN


























































class IslamicCalendar : public Calendar {
 public:
  
  
  
  



  enum ECivil {
    ASTRONOMICAL,
    CIVIL
  };
  
  



  enum EMonths {
    



    MUHARRAM = 0,

    



    SAFAR = 1,

    



    RABI_1 = 2,

    



    RABI_2 = 3,

    



    JUMADA_1 = 4,

    



    JUMADA_2 = 5,

    



    RAJAB = 6,

    



    SHABAN = 7,

    



    RAMADAN = 8,

    



    SHAWWAL = 9,

    



    DHU_AL_QIDAH = 10,

    



    DHU_AL_HIJJAH = 11,
    
    ISLAMIC_MONTH_MAX
  }; 



  
  
  

  









  IslamicCalendar(const Locale& aLocale, UErrorCode &success, ECivil beCivil = CIVIL);

  



  IslamicCalendar(const IslamicCalendar& other);

  



  virtual ~IslamicCalendar();

  







  void setCivil(ECivil beCivil, UErrorCode &status);
    
  





  UBool isCivil();


  

  
  virtual Calendar* clone() const;

 private:
  


  static UBool civilLeapYear(int32_t year);
    
  



  int32_t yearStart(int32_t year);

  






  int32_t monthStart(int32_t year, int32_t month) const;
    
  







  int32_t trueMonthStart(int32_t month) const;

  








  static double moonAge(UDate time, UErrorCode &status);

  
  
  
    
  




  ECivil civil;

  
  
  
 protected:
  


  virtual int32_t handleGetLimit(UCalendarDateFields field, ELimitType limitType) const;
  
  






  virtual int32_t handleGetMonthLength(int32_t extendedYear, int32_t month) const;
  
  



  virtual int32_t handleGetYearLength(int32_t extendedYear) const;
    
  
  
  

  
  


  virtual int32_t handleComputeMonthStart(int32_t eyear, int32_t month, UBool useMonth) const;

  
  
  

  


  virtual int32_t handleGetExtendedYear();

  















  virtual void handleComputeFields(int32_t julianDay, UErrorCode &status);

  
 public: 
  




  virtual UClassID getDynamicClassID(void) const;

  










  U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);

  





  virtual const char * getType() const;

 private:
  IslamicCalendar(); 

  
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
};

U_NAMESPACE_END

#endif
#endif



