









#ifndef __CalendarCaseTest__
#define __CalendarCaseTest__
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/smpdtfmt.h"
#include "caltest.h"

class CalendarCaseTest: public CalendarTest {
 public:
  virtual void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );

  
  struct TestCase {
    double julian; 
    int32_t era;
    int32_t year;
    int32_t month;
    int32_t day;
    int32_t dayOfWeek;
    int32_t hour;
    int32_t min;
    int32_t sec;
  };
  
  


  void doTestCases(const TestCase *cases, Calendar *cal);

 private:
  







  UBool checkField(Calendar *cal, UCalendarDateFields field, int32_t value, UErrorCode &status);

 private:
  
  void IslamicCivil();
  void Hebrew();
  void Indian();
  void Coptic();
  void Ethiopic();
};

#endif
#endif
