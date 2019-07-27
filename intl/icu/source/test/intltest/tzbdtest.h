





#ifndef __TimeZoneBoundaryTest__
#define __TimeZoneBoundaryTest__
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/timezone.h"
#include "unicode/simpletz.h"
#include "caltztst.h"





class TimeZoneBoundaryTest: public CalendarTimeZoneTest {
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public: 

    TimeZoneBoundaryTest();

    







    


    virtual void findDaylightBoundaryUsingDate(UDate d, const char* startMode, UDate expectedBoundary);
    virtual void findDaylightBoundaryUsingTimeZone(UDate d, UBool startsInDST, UDate expectedBoundary);
    virtual void findDaylightBoundaryUsingTimeZone(UDate d, UBool startsInDST, UDate expectedBoundary, TimeZone* tz);
 
private:
    
    UnicodeString showDate(UDate d);
    static UnicodeString showNN(int32_t n);
 
public: 
    



    virtual void verifyDST(UDate d, TimeZone* time_zone, UBool expUseDaylightTime, UBool expInDaylightTime, UDate expZoneOffset, UDate expDSTOffset);
 
    



    virtual void TestBoundaries(void);
 
    


    virtual void testUsingBinarySearch(SimpleTimeZone* tz, UDate d, UDate expectedBoundary);
 
    


    virtual void TestNewRules(void);
 
    


    virtual void findBoundariesStepwise(int32_t year, UDate interval, TimeZone* z, int32_t expectedChanges);
 
    


 
    virtual void TestStepwise(void);
    void verifyMapping(Calendar& cal, int year, int month, int dom, int hour,
                    double epochHours) ;
private:
    const UDate ONE_SECOND;
    const UDate ONE_MINUTE;
    const UDate ONE_HOUR;
    const UDate ONE_DAY;
    const UDate ONE_YEAR;
    const UDate SIX_MONTHS;
    static const int32_t MONTH_LENGTH[];
    static const UDate PST_1997_BEG;
    static const UDate PST_1997_END;
    static const UDate INTERVAL;
};

#endif 
 
#endif 
