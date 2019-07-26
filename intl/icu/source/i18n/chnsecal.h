














#ifndef CHNSECAL_H
#define CHNSECAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/timezone.h"

U_NAMESPACE_BEGIN






































































class U_I18N_API ChineseCalendar : public Calendar {
 public:
  
  
  

  








  ChineseCalendar(const Locale& aLocale, UErrorCode &success);

 protected:
 
   












  ChineseCalendar(const Locale& aLocale, int32_t epochYear, const TimeZone* zoneAstroCalc, UErrorCode &success);

 public:
  



  ChineseCalendar(const ChineseCalendar& other);

  



  virtual ~ChineseCalendar();

  
  virtual Calendar* clone() const;

 private:

  
  
  
    
  UBool isLeapYear;
  int32_t fEpochYear;   
  const TimeZone* fZoneAstroCalc;   
                                    

  
  
  

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

  double daysToMillis(double days) const;
  double millisToDays(double millis) const;
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
  const TimeZone* getChineseCalZoneAstroCalc(void) const;

  
 public: 
  




  virtual UClassID getDynamicClassID(void) const;

  










  static UClassID U_EXPORT2 getStaticClassID(void);

  





  virtual const char * getType() const;


 protected:
  








  virtual UBool inDaylightTime(UErrorCode& status) const;


  



  virtual UBool haveDefaultCentury() const;

  




  virtual UDate defaultCenturyStart() const;

  



  virtual int32_t defaultCenturyStartYear() const;

 private: 

  



  UDate         internalGetDefaultCenturyStart(void) const;

  



  int32_t          internalGetDefaultCenturyStartYear(void) const;

  ChineseCalendar(); 
};

U_NAMESPACE_END

#endif
#endif
