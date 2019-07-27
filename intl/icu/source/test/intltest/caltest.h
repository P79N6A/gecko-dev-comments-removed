




#ifndef __CalendarTest__
#define __CalendarTest__
 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "unicode/smpdtfmt.h"
#include "caltztst.h"

class CalendarTest: public CalendarTimeZoneTest {
public:
    
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par );
public:
    



    virtual void TestRog(void);
    



    virtual void TestDOW943(void);
    


    void dowTest(UBool lenient);
    


    virtual void TestClonesUnique908(void);
    


    virtual void TestGregorianChange768(void);
    


    virtual void TestDisambiguation765(void);
    


    virtual void TestGenericAPI(void); 

    virtual void TestWOY(void);
    
    virtual void TestDebug(void);
 
public: 
    


    virtual void verify765(const UnicodeString& msg, Calendar* c, int32_t year, int32_t month, int32_t day);
    


    virtual void verify765(const UnicodeString& msg, UErrorCode status);
 
public:
    


    virtual void TestGMTvsLocal4064654(void);
 
public: 
    


    virtual void test4064654(int32_t yr, int32_t mo, int32_t dt, int32_t hr, int32_t mn, int32_t sc);
 
public:
    



    virtual void TestAddSetOrder621(void);
    


    virtual void TestAdd520(void);
    


    virtual void TestAddRollExtensive(void);

public: 
    
    virtual void check520(Calendar* c, 
                            int32_t y, int32_t m, int32_t d, 
                            int32_t hr, int32_t min, int32_t sec, 
                            int32_t ms, UCalendarDateFields field);
 
    virtual void check520(Calendar* c, 
                            int32_t y, int32_t m, int32_t d);

public:
    



    virtual void TestFieldSet4781(void);






 
public:
    


 
    virtual void TestSecondsZero121(void);
    




    virtual void TestAddSetGet0610(void);
 
public: 
    
    static UnicodeString value(Calendar* calendar);
 
public:
    


    virtual void TestFields060(void);
 
public: 
    static int32_t EXPECTED_FIELDS[];
    static const int32_t EXPECTED_FIELDS_length;
 
public:
    



    virtual void TestEpochStartFields(void);
 
public: 
    static int32_t EPOCH_FIELDS[];
 
public:
    



    virtual void TestDOWProgression(void);
    


    virtual void TestDOW_LOCALandYEAR_WOY(void);
    
    virtual void doYEAR_WOYLoop(Calendar *cal, 
        SimpleDateFormat *sdf, int32_t times, UErrorCode& status);
    
    virtual void loop_addroll(Calendar *cal, 
        int times, UCalendarDateFields field, UCalendarDateFields field2, 
        UErrorCode& errorCode);

    void TestYWOY(void);
    void TestJD(void);

    void yearAddTest(Calendar& cal, UErrorCode& status);
 
public: 
    
    virtual void marchByDelta(Calendar* cal, int32_t delta);

 public:
    
    static UnicodeString fieldName(UCalendarDateFields f);
    static UnicodeString calToStr(const Calendar & cal);
    
    

    


    static int32_t testLocaleCount();

    



    static const char* testLocaleID(int32_t i);

    






    static UDate minDateOfCalendar(const Calendar& cal, UBool &isGregorian, UErrorCode& status);

    






    static UDate minDateOfCalendar(const Locale& locale, UBool &isGregorian, UErrorCode& status);

  
 public:
    void Test6703(void);
    void Test3785(void);
    void Test1624(void);
    void TestIslamicUmAlQura(void);
    void TestIslamicTabularDates(void);

    


    void TestTimeStamp(void);
    


    void TestISO8601(void);

    


    void TestAmbiguousWallTimeAPIs(void);
    void TestRepeatedWallTime(void);
    void TestSkippedWallTime(void);

    void TestCloneLocale(void);

    void TestHebrewMonthValidation(void);

    


    void setAndTestCalendar(Calendar* cal, int32_t initMonth, int32_t initDay, int32_t initYear, UErrorCode& status);
    void setAndTestWholeYear(Calendar* cal, int32_t startYear, UErrorCode& status);

    void TestWeekData(void);

    void TestAddAcrossZoneTransition(void);
};

#endif 
 
#endif 
