









#ifndef DANGICAL_H
#define DANGICAL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/timezone.h"
#include "chnsecal.h"

U_NAMESPACE_BEGIN











class DangiCalendar : public ChineseCalendar {
 public:
  
  
  

  








  DangiCalendar(const Locale& aLocale, UErrorCode &success);

  



  DangiCalendar(const DangiCalendar& other);

  



  virtual ~DangiCalendar();

  



  virtual Calendar* clone() const;

  
  
  

 private:

  const TimeZone* getDangiCalZoneAstroCalc(void) const;

  
 public: 
  




  virtual UClassID getDynamicClassID(void) const;

  










  U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);

  





  const char * getType() const;


 private:
 
  DangiCalendar(); 
};

U_NAMESPACE_END

#endif
#endif



