














#ifndef CHNSECAL_H
#define CHNSECAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"

U_NAMESPACE_BEGIN






































































class ChineseCalendar : public Calendar {
 public:
  
  
  

  








  ChineseCalendar(const Locale& aLocale, UErrorCode &success);

  



  ChineseCalendar(const ChineseCalendar& other);

  



  virtual ~ChineseCalendar();

  
  virtual Calendar* clone() const;

 private:

  
  
  
    
  UBool isLeapYear;

  
  
  

 protected:
  virtual int32_t handleGetLimit(UCalendarDateFields field, ELimitType limitType) const;
  virtual int32_t handleGetMonthLength(int32_t extendedYear, int32_t month) const;
  virtual int32_t handleComputeMonthStart(int32_t eyear, int32_t month, UBool useMonth) const;
  virtual int32_t handleGetExtendedYear();
  virtual void handleComputeFields(int32_t julianDay, UErrorCode &status);
  virtual const UFieldResolutionTable* getFieldResolutionTable() const;



public:
  virtual void add(UCalendarDateFields field, int32_t amount, UErrorCode &status);
  virtual void add(EDateFields field, int32_t amount, UErrorCode &status);
  virtual void roll(UCalendarDateFields field, int32_t amount, UErrorCode &status);
  virtual void roll(EDateFields field, int32_t amount, UErrorCode &status);

  
  
  
  

private:

  static const UFieldResolutionTable CHINESE_DATE_PRECEDENCE[];

  static double daysToMillis(double days);
  static double millisToDays(double millis);
  virtual int32_t winterSolstice(int32_t gyear) const;
  virtual int32_t newMoonNear(double days, UBool after) const;
  virtual int32_t synodicMonthsBetween(int32_t day1, int32_t day2) const;
  virtual int32_t majorSolarTerm(int32_t days) const;
  virtual UBool hasNoMajorSolarTerm(int32_t newMoon) const;
  virtual UBool isLeapMonthBetween(int32_t newMoon1, int32_t newMoon2) const;
  virtual void computeChineseFields(int32_t days, int32_t gyear,
                 int32_t gmonth, UBool setAllFields);
  virtual int32_t newYear(int32_t gyear) const;
  virtual void offsetMonth(int32_t newMoon, int32_t dom, int32_t delta);


  
 public: 
  




  virtual UClassID getDynamicClassID(void) const;

  










  U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);

  





  virtual const char * getType() const;


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

  ChineseCalendar(); 
};

U_NAMESPACE_END

#endif
#endif



