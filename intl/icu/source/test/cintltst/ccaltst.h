













#ifndef _CCALTST
#define _CCALTST

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "cintltst.h"


    


    static void TestCalendar(void);
    


    static void TestGetSetDateAPI(void);
    



    static void TestFieldGetSet(void);
    


    static void TestAddRollExtensive(void);
    


    static void TestGetLimits(void);
    



    static void TestDOWProgression(void);
    


    static void TestGMTvsLocal(void);
    


    static void testZones(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
    


    static void TestGetKeywordValuesForLocale(void);
    


    static void TestWeekend(void);
    


    static void TestAmbiguousWallTime(void);


    


    static void checkDate(UCalendar* c, int32_t y, int32_t m, int32_t d);

    static void checkDateTime(UCalendar* c, int32_t y, int32_t m, int32_t d, 
                            int32_t hr, int32_t min, int32_t sec, int32_t ms, 
                                                    UCalendarDateFields field);

    


    static void verify1(const char* msg, UCalendar* c, UDateFormat* dat, int32_t year, int32_t month, int32_t day);

    static void verify2(const char* msg, UCalendar* c, UDateFormat* dat, int32_t year, int32_t month, int32_t day,
                                                                int32_t hour, int32_t min, int32_t sec, int32_t am_pm);

#endif 

#endif
