






#ifndef UCAL_H
#define UCAL_H

#include "unicode/utypes.h"
#include "unicode/uenum.h"
#include "unicode/uloc.h"
#include "unicode/localpointer.h"

#if !UCONFIG_NO_FORMATTING


































































































































#define UCAL_UNKNOWN_ZONE_ID "Etc/Unknown"





typedef void* UCalendar;




enum UCalendarType {
  




  UCAL_TRADITIONAL,
  



  UCAL_DEFAULT = UCAL_TRADITIONAL,
  



  UCAL_GREGORIAN
};


typedef enum UCalendarType UCalendarType;




enum UCalendarDateFields {
  




  UCAL_ERA,

  



  UCAL_YEAR,

  


















  UCAL_MONTH,

  










  UCAL_WEEK_OF_YEAR,

 












  UCAL_WEEK_OF_MONTH,

 






  UCAL_DATE,

 




  UCAL_DAY_OF_YEAR,

 













  UCAL_DAY_OF_WEEK,

 






















  UCAL_DAY_OF_WEEK_IN_MONTH,

 








  UCAL_AM_PM,

 








  UCAL_HOUR,

 






  UCAL_HOUR_OF_DAY,

 





  UCAL_MINUTE,

 





  UCAL_SECOND,

 





  UCAL_MILLISECOND,

 




  UCAL_ZONE_OFFSET,

 




  UCAL_DST_OFFSET,
  
 






  UCAL_YEAR_WOY,

 





  UCAL_DOW_LOCAL,

  





  UCAL_EXTENDED_YEAR,

 









  UCAL_JULIAN_DAY, 

  








  UCAL_MILLISECONDS_IN_DAY,

  



  UCAL_IS_LEAP_MONTH,
  
  



  UCAL_FIELD_COUNT,

 







  UCAL_DAY_OF_MONTH=UCAL_DATE
};


typedef enum UCalendarDateFields UCalendarDateFields;
    







enum UCalendarDaysOfWeek {
  
  UCAL_SUNDAY = 1,
  
  UCAL_MONDAY,
  
  UCAL_TUESDAY,
  
  UCAL_WEDNESDAY,
  
  UCAL_THURSDAY,
  
  UCAL_FRIDAY,
  
  UCAL_SATURDAY
};


typedef enum UCalendarDaysOfWeek UCalendarDaysOfWeek;




enum UCalendarMonths {
  
  UCAL_JANUARY,
  
  UCAL_FEBRUARY,
  
  UCAL_MARCH,
  
  UCAL_APRIL,
  
  UCAL_MAY,
  
  UCAL_JUNE,
  
  UCAL_JULY,
  
  UCAL_AUGUST,
  
  UCAL_SEPTEMBER,
  
  UCAL_OCTOBER,
  
  UCAL_NOVEMBER,
  
  UCAL_DECEMBER,
  



  UCAL_UNDECIMBER
};


typedef enum UCalendarMonths UCalendarMonths;




enum UCalendarAMPMs {
    
  UCAL_AM,
  
  UCAL_PM
};


typedef enum UCalendarAMPMs UCalendarAMPMs;







enum USystemTimeZoneType {
    



    UCAL_ZONE_TYPE_ANY,
    



    UCAL_ZONE_TYPE_CANONICAL,
    



    UCAL_ZONE_TYPE_CANONICAL_LOCATION
};


typedef enum USystemTimeZoneType USystemTimeZoneType;
















 
U_STABLE UEnumeration* U_EXPORT2
ucal_openTimeZoneIDEnumeration(USystemTimeZoneType zoneType, const char* region,
                                const int32_t* rawOffset, UErrorCode* ec);












U_STABLE UEnumeration* U_EXPORT2
ucal_openTimeZones(UErrorCode* ec);

















U_STABLE UEnumeration* U_EXPORT2
ucal_openCountryTimeZones(const char* country, UErrorCode* ec);

















U_STABLE int32_t U_EXPORT2
ucal_getDefaultTimeZone(UChar* result, int32_t resultCapacity, UErrorCode* ec);










U_STABLE void U_EXPORT2
ucal_setDefaultTimeZone(const UChar* zoneID, UErrorCode* ec);

















U_STABLE int32_t U_EXPORT2
ucal_getDSTSavings(const UChar* zoneID, UErrorCode* ec);







U_STABLE UDate U_EXPORT2 
ucal_getNow(void);
























U_STABLE UCalendar* U_EXPORT2 
ucal_open(const UChar*   zoneID,
          int32_t        len,
          const char*    locale,
          UCalendarType  type,
          UErrorCode*    status);







U_STABLE void U_EXPORT2 
ucal_close(UCalendar *cal);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUCalendarPointer, UCalendar, ucal_close);

U_NAMESPACE_END

#endif









U_STABLE UCalendar* U_EXPORT2 
ucal_clone(const UCalendar* cal,
           UErrorCode*      status);










U_STABLE void U_EXPORT2 
ucal_setTimeZone(UCalendar*    cal,
                 const UChar*  zoneID,
                 int32_t       len,
                 UErrorCode*   status);





enum UCalendarDisplayNameType {
  
  UCAL_STANDARD,
  
  UCAL_SHORT_STANDARD,
  
  UCAL_DST,
  
  UCAL_SHORT_DST
};


typedef enum UCalendarDisplayNameType UCalendarDisplayNameType;














U_STABLE int32_t U_EXPORT2 
ucal_getTimeZoneDisplayName(const UCalendar*          cal,
                            UCalendarDisplayNameType  type,
                            const char*               locale,
                            UChar*                    result,
                            int32_t                   resultLength,
                            UErrorCode*               status);









U_STABLE UBool U_EXPORT2 
ucal_inDaylightTime(const UCalendar*  cal,
                    UErrorCode*       status );





















U_STABLE void U_EXPORT2
ucal_setGregorianChange(UCalendar *cal, UDate date, UErrorCode *pErrorCode);





















U_STABLE UDate U_EXPORT2
ucal_getGregorianChange(const UCalendar *cal, UErrorCode *pErrorCode);





enum UCalendarAttribute {
  



  UCAL_LENIENT,
  



  UCAL_FIRST_DAY_OF_WEEK,
  



  UCAL_MINIMAL_DAYS_IN_FIRST_WEEK
#ifndef U_HIDE_DRAFT_API
  ,
  




  UCAL_REPEATED_WALL_TIME,
  




  UCAL_SKIPPED_WALL_TIME
#endif  
};


typedef enum UCalendarAttribute UCalendarAttribute;






enum UCalendarWallTimeOption {
    





    UCAL_WALLTIME_LAST
#ifndef U_HIDE_DRAFT_API
    ,
    





    UCAL_WALLTIME_FIRST,
    




    UCAL_WALLTIME_NEXT_VALID
#endif  
};

typedef enum UCalendarWallTimeOption UCalendarWallTimeOption;












U_STABLE int32_t U_EXPORT2 
ucal_getAttribute(const UCalendar*    cal,
                  UCalendarAttribute  attr);












U_STABLE void U_EXPORT2 
ucal_setAttribute(UCalendar*          cal,
                  UCalendarAttribute  attr,
                  int32_t             newValue);










U_STABLE const char* U_EXPORT2 
ucal_getAvailable(int32_t localeIndex);









U_STABLE int32_t U_EXPORT2 
ucal_countAvailable(void);












U_STABLE UDate U_EXPORT2 
ucal_getMillis(const UCalendar*  cal,
               UErrorCode*       status);












U_STABLE void U_EXPORT2 
ucal_setMillis(UCalendar*   cal,
               UDate        dateTime,
               UErrorCode*  status );















U_STABLE void U_EXPORT2 
ucal_setDate(UCalendar*   cal,
             int32_t      year,
             int32_t      month,
             int32_t      date,
             UErrorCode*  status);


















U_STABLE void U_EXPORT2 
ucal_setDateTime(UCalendar*   cal,
                 int32_t      year,
                 int32_t      month,
                 int32_t      date,
                 int32_t      hour,
                 int32_t      minute,
                 int32_t      second,
                 UErrorCode*  status);










U_STABLE UBool U_EXPORT2 
ucal_equivalentTo(const UCalendar*  cal1,
                  const UCalendar*  cal2);



















U_STABLE void U_EXPORT2 
ucal_add(UCalendar*           cal,
         UCalendarDateFields  field,
         int32_t              amount,
         UErrorCode*          status);

























U_STABLE void U_EXPORT2 
ucal_roll(UCalendar*           cal,
          UCalendarDateFields  field,
          int32_t              amount,
          UErrorCode*          status);

















U_STABLE int32_t U_EXPORT2 
ucal_get(const UCalendar*     cal,
         UCalendarDateFields  field,
         UErrorCode*          status );
















U_STABLE void U_EXPORT2 
ucal_set(UCalendar*           cal,
         UCalendarDateFields  field,
         int32_t              value);
















U_STABLE UBool U_EXPORT2 
ucal_isSet(const UCalendar*     cal,
           UCalendarDateFields  field);















U_STABLE void U_EXPORT2 
ucal_clearField(UCalendar*           cal,
                UCalendarDateFields  field);











U_STABLE void U_EXPORT2 
ucal_clear(UCalendar* calendar);





enum UCalendarLimitType {
  
  UCAL_MINIMUM,
  
  UCAL_MAXIMUM,
  
  UCAL_GREATEST_MINIMUM,
  
  UCAL_LEAST_MAXIMUM,
  
  UCAL_ACTUAL_MINIMUM,
  
  UCAL_ACTUAL_MAXIMUM
};


typedef enum UCalendarLimitType UCalendarLimitType;















U_STABLE int32_t U_EXPORT2 
ucal_getLimit(const UCalendar*     cal,
              UCalendarDateFields  field,
              UCalendarLimitType   type,
              UErrorCode*          status);








U_STABLE const char * U_EXPORT2
ucal_getLocaleByType(const UCalendar *cal, ULocDataLocaleType type, UErrorCode* status);







U_STABLE const char * U_EXPORT2
ucal_getTZDataVersion(UErrorCode* status);



















U_STABLE int32_t U_EXPORT2
ucal_getCanonicalTimeZoneID(const UChar* id, int32_t len,
                            UChar* result, int32_t resultCapacity, UBool *isSystemID, UErrorCode* status);







U_STABLE const char * U_EXPORT2
ucal_getType(const UCalendar *cal, UErrorCode* status);

















U_STABLE UEnumeration* U_EXPORT2
ucal_getKeywordValuesForLocale(const char* key,
                               const char* locale,
                               UBool commonlyUsed,
                               UErrorCode* status);





enum UCalendarWeekdayType {
  



  UCAL_WEEKDAY,
  



  UCAL_WEEKEND,
  




  UCAL_WEEKEND_ONSET,
  




  UCAL_WEEKEND_CEASE
};


typedef enum UCalendarWeekdayType UCalendarWeekdayType;

















U_STABLE UCalendarWeekdayType U_EXPORT2
ucal_getDayOfWeekType(const UCalendar *cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode* status);
















U_STABLE int32_t U_EXPORT2
ucal_getWeekendTransition(const UCalendar *cal, UCalendarDaysOfWeek dayOfWeek, UErrorCode *status);











U_STABLE UBool U_EXPORT2
ucal_isWeekend(const UCalendar *cal, UDate date, UErrorCode *status);

























U_STABLE int32_t U_EXPORT2 
ucal_getFieldDifference(UCalendar* cal,
                        UDate target,
                        UCalendarDateFields field,
                        UErrorCode* status);

#ifndef U_HIDE_DRAFT_API




enum UTimeZoneTransitionType {
    




    UCAL_TZ_TRANSITION_NEXT,
    




    UCAL_TZ_TRANSITION_NEXT_INCLUSIVE,
    




    UCAL_TZ_TRANSITION_PREVIOUS,
    




    UCAL_TZ_TRANSITION_PREVIOUS_INCLUSIVE
};


typedef enum UTimeZoneTransitionType UTimeZoneTransitionType;
















U_DRAFT UBool U_EXPORT2 
ucal_getTimeZoneTransitionDate(const UCalendar* cal, UTimeZoneTransitionType type,
                               UDate* transition, UErrorCode* status);

#endif  

#endif 

#endif
