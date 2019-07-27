








#include "unicode/ucal.h"
#include <stdio.h>

void c_main()
{
  puts("----");
  puts("C Sample");

UErrorCode status = U_ZERO_ERROR;
int32_t i;
UCalendar *cal = ucal_open(NULL, -1, NULL, UCAL_GREGORIAN, &status);
if (U_FAILURE(status)) {
      puts("Couldn't create GregorianCalendar");
      return;
      }
      
      ucal_set(cal, UCAL_YEAR, 2000);
      ucal_set(cal, UCAL_MONTH, UCAL_FEBRUARY);   
      ucal_set(cal, UCAL_DATE, 26);
      ucal_set(cal, UCAL_HOUR_OF_DAY, 23);
      ucal_set(cal, UCAL_MINUTE, 0);
      ucal_set(cal, UCAL_SECOND, 0);
      ucal_set(cal, UCAL_MILLISECOND, 0);
      
      for (i = 0; i < 30; i++) {
          
          
          printf("year: %d, month: %d (%d in the implementation), day: %d\n",
              ucal_get(cal, UCAL_YEAR, &status),
              ucal_get(cal, UCAL_MONTH, &status) + 1,
              ucal_get(cal, UCAL_MONTH, &status),
              ucal_get(cal, UCAL_DATE, &status));
          if (U_FAILURE(status)) {
          puts("Calendar::get failed");
          return;
          }
          
          ucal_add(cal, UCAL_DATE, 1, &status);
          if (U_FAILURE(status))
          {
              puts("Calendar::add failed");
              return;
          }
      }
      ucal_close(cal);  
}
