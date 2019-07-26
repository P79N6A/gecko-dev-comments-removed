














#ifndef PERSNCAL_H
#define PERSNCAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"

U_NAMESPACE_BEGIN






















class PersianCalendar : public Calendar {
 public:
  
  
  
  



  enum EMonths {
    



    FARVARDIN = 0,

    



    ORDIBEHESHT = 1,

    



    KHORDAD = 2,

    



    TIR = 3,

    



    MORDAD = 4,

    



    SHAHRIVAR = 5,

    



    MEHR = 6,

    



    ABAN = 7,

    



    AZAR = 8,

    



    DEI = 9,

    



    BAHMAN = 10,

    



    ESFAND = 11,
    
    PERSIAN_MONTH_MAX
  }; 



  
  
  

  








  PersianCalendar(const Locale& aLocale, UErrorCode &success);

  



  PersianCalendar(const PersianCalendar& other);

  



  virtual ~PersianCalendar();

  

  
  virtual Calendar* clone() const;

 private:
  


  static UBool isLeapYear(int32_t year);
    
  



  int32_t yearStart(int32_t year);

  






  int32_t monthStart(int32_t year, int32_t month) const;
    
  
  
  
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
  PersianCalendar(); 

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



