








#include "unicode/calendar.h"
#include "unicode/gregocal.h"
#include <stdio.h>

extern "C" void c_main();

void cpp_main()
{
  UErrorCode status = U_ZERO_ERROR;
  puts("C++ sample");
  GregorianCalendar* gc = new GregorianCalendar(status);
  if (U_FAILURE(status)) {
    puts("Couldn't create GregorianCalendar");
    return;
  }
  
  gc->set(2000, UCAL_FEBRUARY, 26);
  gc->set(UCAL_HOUR_OF_DAY, 23);
  gc->set(UCAL_MINUTE, 0);
  gc->set(UCAL_SECOND, 0);
  gc->set(UCAL_MILLISECOND, 0);
  
  for (int32_t i = 0; i < 30; i++) {
    
    
    printf("year: %d, month: %d (%d in the implementation), day: %d\n",
           gc->get(UCAL_YEAR, status),
           gc->get(UCAL_MONTH, status) + 1,
           gc->get(UCAL_MONTH, status),
           gc->get(UCAL_DATE, status));
    if (U_FAILURE(status))
      {
        puts("Calendar::get failed");
        return;
      }
    
    gc->add(UCAL_DATE, 1, status);
    if (U_FAILURE(status)) {
      puts("Calendar::add failed");
      return;
    }
  }
  delete gc;
}
                


int main( void )
{
  puts("Date-Calendar sample program");

  cpp_main();

  c_main();

  return 0;
}

