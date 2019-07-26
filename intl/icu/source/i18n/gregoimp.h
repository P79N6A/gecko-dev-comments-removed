










#ifndef GREGOIMP_H
#define GREGOIMP_H
#include "unicode/utypes.h"
#if !UCONFIG_NO_FORMATTING

#include "unicode/ures.h"
#include "unicode/locid.h"
#include "putilimp.h"

U_NAMESPACE_BEGIN






class ClockMath {
 public:
    








    static int32_t floorDivide(int32_t numerator, int32_t denominator);

    








    static inline double floorDivide(double numerator, double denominator);

    














    static int32_t floorDivide(double numerator, int32_t denominator,
                               int32_t& remainder);

    









    static double floorDivide(double dividend, double divisor,
                              double& remainder);
};


#define kOneDay    (1.0 * U_MILLIS_PER_DAY)       //  86,400,000
#define kOneHour   (60*60*1000)
#define kOneMinute 60000
#define kOneSecond 1000
#define kOneMillisecond  1
#define kOneWeek   (7.0 * kOneDay) // 604,800,000


#define kJan1_1JulianDay  1721426 // January 1, year 1 (Gregorian)

#define kEpochStartAsJulianDay  2440588 // January 1, 1970 (Gregorian)

#define kEpochYear              1970


#define kEarliestViableMillis  -185331720384000000.0  // minimum representable by julian day  -1e17

#define kLatestViableMillis     185753453990400000.0  // max representable by julian day      +1e17





#define MIN_JULIAN (-0x7F000000)





#define MIN_MILLIS ((MIN_JULIAN - kEpochStartAsJulianDay) * kOneDay)





#define MAX_JULIAN (+0x7F000000)





#define MAX_MILLIS ((MAX_JULIAN - kEpochStartAsJulianDay) * kOneDay)








class Grego {
 public:
    




    static inline UBool isLeapYear(int32_t year);

    





    static inline int8_t monthLength(int32_t year, int32_t month);

    





    static inline int8_t previousMonthLength(int y, int m);

    







    static double fieldsToDay(int32_t year, int32_t month, int32_t dom);
    
    









    static void dayToFields(double day, int32_t& year, int32_t& month,
                            int32_t& dom, int32_t& dow, int32_t& doy);

    








    static inline void dayToFields(double day, int32_t& year, int32_t& month,
                                   int32_t& dom, int32_t& dow);

    










    static void timeToFields(UDate time, int32_t& year, int32_t& month,
                            int32_t& dom, int32_t& dow, int32_t& doy, int32_t& mid);

    




    static int32_t dayOfWeek(double day);

    







    static int32_t dayOfWeekInMonth(int32_t year, int32_t month, int32_t dom);

    





    static inline double julianDayToMillis(int32_t julian);

    





    static inline int32_t millisToJulianDay(double millis);

    




    static inline int32_t gregorianShift(int32_t eyear);

 private:
    static const int16_t DAYS_BEFORE[24];
    static const int8_t MONTH_LENGTH[24];
};

inline double ClockMath::floorDivide(double numerator, double denominator) {
    return uprv_floor(numerator / denominator);
}

inline UBool Grego::isLeapYear(int32_t year) {
    
    return ((year&0x3) == 0) && ((year%100 != 0) || (year%400 == 0));
}

inline int8_t
Grego::monthLength(int32_t year, int32_t month) {
    return MONTH_LENGTH[month + (isLeapYear(year) ? 12 : 0)];
}

inline int8_t
Grego::previousMonthLength(int y, int m) {
  return (m > 0) ? monthLength(y, m-1) : 31;
}

inline void Grego::dayToFields(double day, int32_t& year, int32_t& month,
                               int32_t& dom, int32_t& dow) {
  int32_t doy_unused;
  dayToFields(day,year,month,dom,dow,doy_unused);
}

inline double Grego::julianDayToMillis(int32_t julian)
{
  return (julian - kEpochStartAsJulianDay) * kOneDay;
}

inline int32_t Grego::millisToJulianDay(double millis) {
  return (int32_t) (kEpochStartAsJulianDay + ClockMath::floorDivide(millis, (double)kOneDay));
}

inline int32_t Grego::gregorianShift(int32_t eyear) {
  int32_t y = eyear-1;
  int32_t gregShift = ClockMath::floorDivide(y, 400) - ClockMath::floorDivide(y, 100) + 2;
  return gregShift;
}





class CalendarData : public UMemory {
public: 
    






    CalendarData(const Locale& loc, const char *type, UErrorCode& status);

    







    UResourceBundle* getByKey(const char *key, UErrorCode& status);

    











    UResourceBundle* getByKey2(const char *key, const char *subKey, UErrorCode& status);

    











    UResourceBundle* getByKey3(const char *key, const char *contextKey, const char *subKey, UErrorCode& status);

    ~CalendarData();

private:
    void initData(const char *locale, const char *type, UErrorCode& status);

    UResourceBundle *fFillin;
    UResourceBundle *fOtherFillin;
    UResourceBundle *fBundle;
    UResourceBundle *fFallback;
    CalendarData(); 
};

U_NAMESPACE_END

#endif 
#endif 


