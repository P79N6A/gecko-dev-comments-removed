



 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/simpletz.h"
#include "unicode/smpdtfmt.h"
#include "unicode/strenum.h"
#include "tzregts.h"
#include "calregts.h"
#include "cmemory.h"





#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))
#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break

void 
TimeZoneRegressionTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char*  )
{
    
    switch (index) {

        CASE(0, Test4052967);
        CASE(1, Test4073209);
        CASE(2, Test4073215);
        CASE(3, Test4084933);
        CASE(4, Test4096952);
        CASE(5, Test4109314);
        CASE(6, Test4126678);
        CASE(7, Test4151406);
        CASE(8, Test4151429);
        CASE(9, Test4154537);
        CASE(10, Test4154542);
        CASE(11, Test4154650);
        CASE(12, Test4154525);
        CASE(13, Test4162593);
        CASE(14, TestJ186);
        CASE(15, TestJ449);
        CASE(16, TestJDK12API);
        CASE(17, Test4176686);
        CASE(18, Test4184229);
        default: name = ""; break;
    }
}

UBool 
TimeZoneRegressionTest::failure(UErrorCode status, const char* msg)
{
    if(U_FAILURE(status)) {
        errln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        return TRUE;
    }

    return FALSE;
}




void TimeZoneRegressionTest:: Test4052967() {
    
    



}




void TimeZoneRegressionTest:: Test4073209() {
    TimeZone *z1 = TimeZone::createTimeZone("PST");
    TimeZone *z2 = TimeZone::createTimeZone("PST");
    if (z1 == z2) 
        errln("Fail: TimeZone should return clones");
    delete z1;
    delete z2;
}

UDate TimeZoneRegressionTest::findTransitionBinary(const SimpleTimeZone& tz, UDate min, UDate max) {
    UErrorCode status = U_ZERO_ERROR;
    UBool startsInDST = tz.inDaylightTime(min, status);
    if (failure(status, "SimpleTimeZone::inDaylightTime")) return 0;
    if (tz.inDaylightTime(max, status) == startsInDST) {
        logln((UnicodeString)"Error: inDaylightTime() != " + ((!startsInDST)?"TRUE":"FALSE"));
        return 0;
    }
    if (failure(status, "SimpleTimeZone::inDaylightTime")) return 0;
    while ((max - min) > 100) { 
        UDate mid = (min + max) / 2;
        if (tz.inDaylightTime(mid, status) == startsInDST) {
            min = mid;
        } else {
            max = mid;
        }
        if (failure(status, "SimpleTimeZone::inDaylightTime")) return 0;
    }
    return (min + max) / 2;
}

UDate TimeZoneRegressionTest::findTransitionStepwise(const SimpleTimeZone& tz, UDate min, UDate max) {
    UErrorCode status = U_ZERO_ERROR;
    UBool startsInDST = tz.inDaylightTime(min, status);
    if (failure(status, "SimpleTimeZone::inDaylightTime")) return 0;
    while (min < max) {
        if (tz.inDaylightTime(min, status) != startsInDST) {
            return min;
        }
        if (failure(status, "SimpleTimeZone::inDaylightTime")) return 0;
        min += (UDate)24*60*60*1000; 
    }
    return 0;
}





void TimeZoneRegressionTest:: Test4073215() 
{
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str, str2;
    SimpleTimeZone *z = new SimpleTimeZone(0, "GMT");
    if (z->useDaylightTime())
        errln("Fail: Fix test to start with non-DST zone");
    z->setStartRule(UCAL_FEBRUARY, 1, UCAL_SUNDAY, 0, status);
    failure(status, "z->setStartRule()");
    z->setEndRule(UCAL_MARCH, -1, UCAL_SUNDAY, 0, status);
    failure(status, "z->setStartRule()");
    if (!z->useDaylightTime())
        errln("Fail: DST not active");

    GregorianCalendar cal(1997, UCAL_JANUARY, 31, status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      return;
    }
    failure(status, "new GregorianCalendar");
    cal.adoptTimeZone(z);

    SimpleDateFormat sdf((UnicodeString)"E d MMM yyyy G HH:mm", status); 
    if(U_FAILURE(status)) {
      dataerrln("Error creating date format %s", u_errorName(status));
      return;
    }
    sdf.setCalendar(cal); 
    failure(status, "new SimpleDateFormat");

    UDate jan31, mar1, mar31;

    UBool indt = z->inDaylightTime(jan31=cal.getTime(status), status);
    failure(status, "inDaylightTime or getTime call on Jan 31");
    if (indt) {
        errln("Fail: Jan 31 inDaylightTime=TRUE, exp FALSE");
    }
    cal.set(1997, UCAL_MARCH, 1);
    indt = z->inDaylightTime(mar1=cal.getTime(status), status);
    failure(status, "inDaylightTime or getTime call on Mar 1");
    if (!indt) {
        UnicodeString str;
        sdf.format(cal.getTime(status), str);
        failure(status, "getTime");
        errln((UnicodeString)"Fail: " + str + " inDaylightTime=FALSE, exp TRUE");
    }
    cal.set(1997, UCAL_MARCH, 31);
    indt = z->inDaylightTime(mar31=cal.getTime(status), status);
    failure(status, "inDaylightTime or getTime call on Mar 31");
    if (indt) {
        errln("Fail: Mar 31 inDaylightTime=TRUE, exp FALSE");
    }

    
















}










void TimeZoneRegressionTest:: Test4084933() {
    UErrorCode status = U_ZERO_ERROR;
    TimeZone *tz = TimeZone::createTimeZone("PST");

    int32_t offset1 = tz->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (2*60*60*1000), status);
    int32_t offset2 = tz->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (2*60*60*1000)-1, status);

    int32_t offset3 = tz->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (1*60*60*1000), status);
    int32_t offset4 = tz->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (1*60*60*1000)-1, status);

    



    int32_t offset5 = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (2*60*60*1000), status);
    int32_t offset6 = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (2*60*60*1000)-1, status);
    int32_t offset5a = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (3*60*60*1000), status);
    int32_t offset6a = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (3*60*60*1000)-1, status);
    int32_t offset7 = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (1*60*60*1000), status);
    int32_t offset8 = tz->getOffset(1,
        1997, UCAL_APRIL, 6, UCAL_SUNDAY, (1*60*60*1000)-1, status);
    int32_t SToffset = (int32_t)(-8 * 60*60*1000L);
    int32_t DToffset = (int32_t)(-7 * 60*60*1000L);
        
#define ERR_IF_FAIL(x) if(x) { dataerrln("FAIL: TimeZone misbehaving - %s", #x); }

        ERR_IF_FAIL(U_FAILURE(status))
        ERR_IF_FAIL(offset1 != SToffset)
        ERR_IF_FAIL(offset2 != SToffset)
        ERR_IF_FAIL(offset3 != SToffset)
        ERR_IF_FAIL(offset4 != DToffset)
        ERR_IF_FAIL(offset5 != DToffset)
        ERR_IF_FAIL(offset6 != SToffset)
        ERR_IF_FAIL(offset5a != DToffset)
        ERR_IF_FAIL(offset6a != DToffset)
        ERR_IF_FAIL(offset7 != SToffset)
        ERR_IF_FAIL(offset8 != SToffset)

#undef ERR_IF_FAIL

    delete tz;
}




void TimeZoneRegressionTest:: Test4096952() {
    








































}




void TimeZoneRegressionTest:: Test4109314() {
    UErrorCode status = U_ZERO_ERROR;
    GregorianCalendar *testCal = (GregorianCalendar*)Calendar::createInstance(status); 
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete testCal;
      return;
    }
    failure(status, "Calendar::createInstance");
    TimeZone *PST = TimeZone::createTimeZone("PST");
    



    UDate testData [] = {
        CalendarRegressionTest::makeDate(98,UCAL_APRIL,4,22,0),    
        CalendarRegressionTest::makeDate(98, UCAL_APRIL,5,6,0),
        CalendarRegressionTest::makeDate(98,UCAL_OCTOBER,24,22,0), 
        CalendarRegressionTest::makeDate(98,UCAL_OCTOBER,25,6,0)
    };
    UBool pass = TRUE;
    for (int32_t i = 0; i < 4; i+=2) {
        
        testCal->setTimeZone(*PST);
        UDate t        = testData[i];
        UDate end    = testData[i+1];
        while(testCal->getTime(status) < end) { 
            testCal->setTime(t, status);
            if ( ! checkCalendar314(testCal, PST))
                pass = FALSE;
            t += 60*60*1000.0;
        } 
    }
    if ( ! pass) 
        errln("Fail: TZ API inconsistent");

    delete testCal;
    delete PST;
} 

UBool 
TimeZoneRegressionTest::checkCalendar314(GregorianCalendar *testCal, TimeZone *testTZ) 
{
    UErrorCode status = U_ZERO_ERROR;
    

    int32_t tzOffset, tzRawOffset; 
    float tzOffsetFloat,tzRawOffsetFloat; 
    
    
    
    UDate millis = testCal->get(UCAL_MILLISECOND, status) +
        1000.0 * (testCal->get(UCAL_SECOND, status) +
        60.0 * (testCal->get(UCAL_MINUTE, status) +
        60.0 * (testCal->get(UCAL_HOUR_OF_DAY, status)))) -
        testCal->get(UCAL_DST_OFFSET, status);

    




    int32_t date = testCal->get(UCAL_DATE, status);
    int32_t dow  = testCal->get(UCAL_DAY_OF_WEEK, status);
    while(millis < 0) {
        millis += U_MILLIS_PER_DAY;
        --date;
        dow = UCAL_SUNDAY + ((dow - UCAL_SUNDAY + 6) % 7);
    }
    while (millis >= U_MILLIS_PER_DAY) {
        millis -= U_MILLIS_PER_DAY;
        ++date;
        dow = UCAL_SUNDAY + ((dow - UCAL_SUNDAY + 1) % 7);
    }

    tzOffset = testTZ->getOffset((uint8_t)testCal->get(UCAL_ERA, status), 
                                testCal->get(UCAL_YEAR, status), 
                                testCal->get(UCAL_MONTH, status), 
                                date, 
                                (uint8_t)dow, 
                                (int32_t)millis,
                                status); 
    tzRawOffset = testTZ->getRawOffset(); 
    tzOffsetFloat = (float)tzOffset/(float)3600000; 
    tzRawOffsetFloat = (float)tzRawOffset/(float)3600000; 

    UDate testDate = testCal->getTime(status); 

    UBool inDaylightTime = testTZ->inDaylightTime(testDate, status); 
    SimpleDateFormat *sdf = new SimpleDateFormat((UnicodeString)"MM/dd/yyyy HH:mm", status); 
    sdf->setCalendar(*testCal); 
    UnicodeString inDaylightTimeString; 

    UBool passed; 

    if(inDaylightTime) 
    { 
        inDaylightTimeString = " DST "; 
        passed = (tzOffset == (tzRawOffset + 3600000));
    } 
    else 
    { 
        inDaylightTimeString = "     "; 
        passed = (tzOffset == tzRawOffset);
    } 

    UnicodeString output;
    FieldPosition pos(0);
    output = testTZ->getID(output) + " " + sdf->format(testDate, output, pos) +
        " Offset(" + tzOffsetFloat + ")" +
        " RawOffset(" + tzRawOffsetFloat + ")" + 
        " " + millis/(float)3600000 + " " +
        inDaylightTimeString; 

    if (passed) 
        output += "     "; 
    else 
        output += "ERROR"; 

    if (passed) 
        logln(output); 
    else 
        errln(output);

    delete sdf;
    return passed;
} 














void TimeZoneRegressionTest:: Test4126678() 
{
    UErrorCode status = U_ZERO_ERROR;
    Calendar *cal = Calendar::createInstance(status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete cal;
      return;
    }
    failure(status, "Calendar::createInstance");
    TimeZone *tz = TimeZone::createTimeZone("PST");
    cal->adoptTimeZone(tz);

    cal->set(1998, UCAL_APRIL, 5, 10, 0);
    
    if (! tz->useDaylightTime() || U_FAILURE(status))
        dataerrln("We're not in Daylight Savings Time and we should be. - %s", u_errorName(status));

    
    int32_t era = cal->get(UCAL_ERA, status);
    int32_t year = cal->get(UCAL_YEAR, status);
    int32_t month = cal->get(UCAL_MONTH, status);
    int32_t day = cal->get(UCAL_DATE, status);
    int32_t dayOfWeek = cal->get(UCAL_DAY_OF_WEEK, status);
    int32_t millis = cal->get(UCAL_MILLISECOND, status) +
        (cal->get(UCAL_SECOND, status) +
         (cal->get(UCAL_MINUTE, status) +
          (cal->get(UCAL_HOUR, status) * 60) * 60) * 1000) -
        cal->get(UCAL_DST_OFFSET, status);

    failure(status, "cal->get");
    int32_t offset = tz->getOffset((uint8_t)era, year, month, day, (uint8_t)dayOfWeek, millis, status);
    int32_t raw_offset = tz->getRawOffset();

    if (offset == raw_offset)
        dataerrln("Offsets should match");

    delete cal;
}






void TimeZoneRegressionTest:: Test4151406() {
    int32_t max = 0;
    for (int32_t h=-28; h<=30; ++h) {
        
        int32_t rawoffset = h * 1800000;
        int32_t hh = (h<0) ? -h : h;
        UnicodeString hname = UnicodeString((h<0) ? "GMT-" : "GMT+") +
            ((hh/2 < 10) ? "0" : "") +
            (hh/2) + ':' +
            ((hh%2==0) ? "00" : "30");
        
            UErrorCode ec = U_ZERO_ERROR;
            int32_t count;
            StringEnumeration* ids = TimeZone::createEnumeration(rawoffset);
            if (ids == NULL) {
                dataerrln("Fail: TimeZone::createEnumeration(rawoffset)");
                continue;
            }
            count = ids->count(ec);
            if (count> max) 
                max = count;
            if (count > 0) {
                logln(hname + ' ' + (UnicodeString)count + (UnicodeString)" e.g. " + *ids->snext(ec));
            } else {
                logln(hname + ' ' + count);
            }
            
            delete ids;
            
            
        


    }
    logln("Maximum zones per offset = %d", max);
}




void TimeZoneRegressionTest:: Test4151429() {
    
    
        




    
}






void TimeZoneRegressionTest:: Test4154537() {
    UErrorCode status = U_ZERO_ERROR;
    
    SimpleTimeZone *tz1 = new SimpleTimeZone(0, "1", 0, 0, 0, 0, 2, 0, 0, 0, status);
    SimpleTimeZone *tz2 = new SimpleTimeZone(0, "2", 1, 0, 0, 0, 3, 0, 0, 0, status);
    
    SimpleTimeZone *tza = new SimpleTimeZone(0, "a", 0, 1, 0, 0, 3, 2, 0, 0, status);
    SimpleTimeZone *tzA = new SimpleTimeZone(0, "A", 0, 1, 0, 0, 3, 2, 0, 0, status);
    
    SimpleTimeZone *tzb = new SimpleTimeZone(0, "b", 0, 1, 0, 0, 3, 1, 0, 0, status);
    
    if(U_FAILURE(status))
        errln("Couldn't create TimeZones");

    if (tz1->useDaylightTime() || tz2->useDaylightTime() ||
        !tza->useDaylightTime() || !tzA->useDaylightTime() ||
        !tzb->useDaylightTime()) {
        errln("Test is broken -- rewrite it");
    }
    if (!tza->hasSameRules(*tzA) || tza->hasSameRules(*tzb)) {
        errln("Fail: hasSameRules() broken for zones with rules");
    }
    if (!tz1->hasSameRules(*tz2)) {
        errln("Fail: hasSameRules() returns false for zones without rules");
        
        
    }

    delete tz1;
    delete tz2;
    delete tza;
    delete tzA;
    delete tzb;
}






void TimeZoneRegressionTest:: Test4154542() 
{
    const int32_t GOOD = 1;
    const int32_t BAD  = 0;

    const int32_t GOOD_MONTH       = UCAL_JANUARY;
    const int32_t GOOD_DAY         = 1;
    const int32_t GOOD_DAY_OF_WEEK = UCAL_SUNDAY;
    const int32_t GOOD_TIME        = 0;

    int32_t DATA [] = {
        GOOD, INT32_MIN,    0,  INT32_MAX,   INT32_MIN,
        GOOD, UCAL_JANUARY,    -5,  UCAL_SUNDAY,     0,
        GOOD, UCAL_DECEMBER,    5,  UCAL_SATURDAY,   24*60*60*1000,
        BAD,  UCAL_DECEMBER,    5,  UCAL_SATURDAY,   24*60*60*1000+1,
        BAD,  UCAL_DECEMBER,    5,  UCAL_SATURDAY,  -1,
        BAD,  UCAL_JANUARY,    -6,  UCAL_SUNDAY,     0,
        BAD,  UCAL_DECEMBER,    6,  UCAL_SATURDAY,   24*60*60*1000,
        GOOD, UCAL_DECEMBER,    1,  0,                   0,
        GOOD, UCAL_DECEMBER,   31,  0,                   0,
        BAD,  UCAL_APRIL,      31,  0,                   0,
        BAD,  UCAL_DECEMBER,   32,  0,                   0,
        BAD,  UCAL_JANUARY-1,   1,  UCAL_SUNDAY,     0,
        BAD,  UCAL_DECEMBER+1,  1,  UCAL_SUNDAY,     0,
        GOOD, UCAL_DECEMBER,   31, -UCAL_SUNDAY,     0,
        GOOD, UCAL_DECEMBER,   31, -UCAL_SATURDAY,   0,
        BAD,  UCAL_DECEMBER,   32, -UCAL_SATURDAY,   0,
        BAD,  UCAL_DECEMBER,  -32, -UCAL_SATURDAY,   0,
        BAD,  UCAL_DECEMBER,   31, -UCAL_SATURDAY-1, 0,
    };
    SimpleTimeZone *zone = new SimpleTimeZone(0, "Z");
    for (int32_t i=0; i < 18*5; i+=5) {
        UBool shouldBeGood = (DATA[i] == GOOD);
        int32_t month     = DATA[i+1];
        int32_t day       = DATA[i+2];
        int32_t dayOfWeek = DATA[i+3];
        int32_t time      = DATA[i+4];

        UErrorCode status = U_ZERO_ERROR;

        
        
            zone->setStartRule(month, day, dayOfWeek, time, status);
        
        
        
        if (U_SUCCESS(status) != shouldBeGood) {
            errln(UnicodeString("setStartRule(month=") + month + ", day=" + day +
                  ", dayOfWeek=" + dayOfWeek + ", time=" + time +
                  (shouldBeGood ? (") should work")
                   : ") should fail but doesn't"));
        }

        
        
        status = U_ZERO_ERROR;
            zone->setEndRule(month, day, dayOfWeek, time, status);
        
        
        
        if (U_SUCCESS(status) != shouldBeGood) {
            errln(UnicodeString("setEndRule(month=") + month + ", day=" + day +
                  ", dayOfWeek=" + dayOfWeek + ", time=" + time +
                  (shouldBeGood ? (") should work")
                   : ") should fail but doesn't"));
        }

        
        
        
        status = U_ZERO_ERROR;
            SimpleTimeZone *temp = new SimpleTimeZone(0, "Z",
                    (int8_t)month, (int8_t)day, (int8_t)dayOfWeek, time,
                    (int8_t)GOOD_MONTH, (int8_t)GOOD_DAY, (int8_t)GOOD_DAY_OF_WEEK, 
                    GOOD_TIME,status);
        
        
        
        if (U_SUCCESS(status) != shouldBeGood) {
            errln(UnicodeString("SimpleTimeZone(month=") + month + ", day=" + day +
                  ", dayOfWeek=" + dayOfWeek + ", time=" + time +
                  (shouldBeGood ? (", <end>) should work")
                   : ", <end>) should fail but doesn't"));
        }
  
        delete temp;
        
        
        status = U_ZERO_ERROR;
            temp = new SimpleTimeZone(0, "Z",
                    (int8_t)GOOD_MONTH, (int8_t)GOOD_DAY, (int8_t)GOOD_DAY_OF_WEEK, 
                    GOOD_TIME,
                    (int8_t)month, (int8_t)day, (int8_t)dayOfWeek, time,status);
        
        
        
        if (U_SUCCESS(status) != shouldBeGood) {
            errln(UnicodeString("SimpleTimeZone(<start>, month=") + month + ", day=" + day +
                  ", dayOfWeek=" + dayOfWeek + ", time=" + time +
                  (shouldBeGood ? (") should work")
                   : ") should fail but doesn't"));
        }            
        delete temp;
    }
    delete zone;
}







void 
TimeZoneRegressionTest::Test4154525() 
{
    const int32_t GOOD = 1, BAD = 0;
    
    int32_t DATA [] = {
        1, GOOD,
        0, BAD,
        -1, BAD,
        60*60*1000, GOOD,
        INT32_MIN, BAD,
        
    };

    UErrorCode status = U_ZERO_ERROR;
    for(int32_t i = 0; i < 10; i+=2) {
        int32_t savings = DATA[i];
        UBool valid = DATA[i+1] == GOOD;
        UnicodeString method;
        for(int32_t j=0; j < 2; ++j) {
            SimpleTimeZone *z=NULL;
            switch (j) {
                case 0:
                    method = "constructor";
                    z = new SimpleTimeZone(0, "id",
                        UCAL_JANUARY, 1, 0, 0,
                        UCAL_MARCH, 1, 0, 0,
                        savings, status); 
                    break;
                case 1:
                    method = "setDSTSavings()";
                    z = new SimpleTimeZone(0, "GMT");
                    z->setDSTSavings(savings, status);
                    break;
            }

            if(U_FAILURE(status)) {
                if(valid) {
                    errln(UnicodeString("Fail: DST savings of ") + savings + " to " + method + " gave " + u_errorName(status));
                }
                else {
                    logln(UnicodeString("Pass: DST savings of ") + savings + " to " + method + " gave " + u_errorName(status));
                }
            }
            else {
                if(valid) {
                    logln(UnicodeString("Pass: DST savings of ") + savings + " accepted by " + method);
                } 
                else {
                    errln(UnicodeString("Fail: DST savings of ") + savings + " accepted by " + method);
                }
            }
            status = U_ZERO_ERROR;
            delete z;
        }
    }
}





void 
TimeZoneRegressionTest::Test4154650() 
{
    const int32_t GOOD = 1, BAD = 0;
    const int32_t GOOD_ERA = GregorianCalendar::AD, GOOD_YEAR = 1998, GOOD_MONTH = UCAL_AUGUST;
    const int32_t GOOD_DAY = 2, GOOD_DOW = UCAL_SUNDAY, GOOD_TIME = 16*3600000;

    int32_t DATA []= {
        GOOD, GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, GOOD_TIME,

        GOOD, GregorianCalendar::BC, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        GOOD, GregorianCalendar::AD, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        BAD,  GregorianCalendar::BC-1, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        BAD,  GregorianCalendar::AD+1, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, GOOD_TIME,

        GOOD, GOOD_ERA, GOOD_YEAR, UCAL_JANUARY, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        GOOD, GOOD_ERA, GOOD_YEAR, UCAL_DECEMBER, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, UCAL_JANUARY-1, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, UCAL_DECEMBER+1, GOOD_DAY, GOOD_DOW, GOOD_TIME,
        
        GOOD, GOOD_ERA, GOOD_YEAR, UCAL_JANUARY, 1, GOOD_DOW, GOOD_TIME,
        GOOD, GOOD_ERA, GOOD_YEAR, UCAL_JANUARY, 31, GOOD_DOW, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, UCAL_JANUARY, 0, GOOD_DOW, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, UCAL_JANUARY, 32, GOOD_DOW, GOOD_TIME,

        GOOD, GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, UCAL_SUNDAY, GOOD_TIME,
        GOOD, GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, UCAL_SATURDAY, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, UCAL_SUNDAY-1, GOOD_TIME,
        BAD,  GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, UCAL_SATURDAY+1, GOOD_TIME,

        GOOD, GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, 0,
        GOOD, GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, 24*3600000-1,
        BAD,  GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, -1,
        BAD,  GOOD_ERA, GOOD_YEAR, GOOD_MONTH, GOOD_DAY, GOOD_DOW, 24*3600000,
    };

    int32_t dataLen = (int32_t)(sizeof(DATA) / sizeof(DATA[0]));

    UErrorCode status = U_ZERO_ERROR;
    TimeZone *tz = TimeZone::createDefault();
    for(int32_t i = 0; i < dataLen; i += 7) {
        UBool good = DATA[i] == GOOD;
        
        
            
        tz->getOffset((uint8_t)DATA[i+1], DATA[i+2], DATA[i+3],
                      DATA[i+4], (uint8_t)DATA[i+5], DATA[i+6], status); 
        
        
        
        if(good != U_SUCCESS(status)) {
            UnicodeString errMsg;
            if (good) {
                errMsg = (UnicodeString(") threw ") + u_errorName(status));
            }
            else {
                errMsg = UnicodeString(") accepts invalid args", "");
            }
            errln(UnicodeString("Fail: getOffset(") +
                  DATA[i+1] + ", " + DATA[i+2] + ", " + DATA[i+3] + ", " +
                  DATA[i+4] + ", " + DATA[i+5] + ", " + DATA[i+6] +
                  errMsg);
        }
        status = U_ZERO_ERROR; 
    }
    delete tz;
}






void 
TimeZoneRegressionTest::Test4162593() 
{
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *fmt = new SimpleDateFormat("z", Locale::getUS(), status);
    if(U_FAILURE(status)) {
      dataerrln("Error creating calendar %s", u_errorName(status));
      delete fmt;
      return;
    }
    const int32_t ONE_HOUR = 60*60*1000;

    SimpleTimeZone *asuncion = new SimpleTimeZone(-4*ONE_HOUR, "America/Asuncion" ,
        UCAL_OCTOBER, 1, 0 , 0*ONE_HOUR,
        UCAL_MARCH, 1, 0 , 0*ONE_HOUR, 1*ONE_HOUR, status);

    



    TimeZone *DATA_TZ [] = {
      0, 0, 0 };

    int32_t DATA_INT [] [5] = {
        
        {1998, UCAL_SEPTEMBER, 30, 22, 0},
        {2000, UCAL_FEBRUARY, 28, 22, 0},
        {2000, UCAL_FEBRUARY, 29, 22, 0},
     };

    UBool DATA_BOOL [] = {
        TRUE,
        FALSE,
        TRUE,
    };
    
    UnicodeString zone [4];
    DATA_TZ[0] =  
        new SimpleTimeZone(2*ONE_HOUR, "Asia/Damascus" ,
            UCAL_APRIL, 1, 0 , 0*ONE_HOUR,
            UCAL_OCTOBER, 1, 0 , 0*ONE_HOUR, 1*ONE_HOUR, status);
    DATA_TZ[1] = asuncion;  DATA_TZ[2] = asuncion;  

    for(int32_t j = 0; j < 3; j++) {
        TimeZone *tz = (TimeZone*)DATA_TZ[j];
        TimeZone::setDefault(*tz);
        fmt->setTimeZone(*tz);

        
        int32_t *p = (int32_t*)DATA_INT[j];
        UDate d = CalendarRegressionTest::makeDate(p[0], p[1], p[2], p[3], p[4]);
       UBool transitionExpected = DATA_BOOL[j];

        UnicodeString temp;
        logln(tz->getID(temp) + ":");
        for (int32_t i = 0; i < 4; ++i) {
            FieldPosition pos(0);
            zone[i].remove();
            zone[i] = fmt->format(d+ i*ONE_HOUR, zone[i], pos);
            logln(UnicodeString("") + i + ": " + d + " / " + zone[i]);
            
        }
        if(zone[0] == zone[1] &&
            (zone[1] == zone[2]) != transitionExpected &&
            zone[2] == zone[3]) {
            logln(UnicodeString("Ok: transition ") + transitionExpected);
        } 
        else {
            errln("Fail: boundary transition incorrect");
        }
    }
    delete fmt;
    delete asuncion;
    delete DATA_TZ[0];
}

  


void TimeZoneRegressionTest::Test4176686() {
    
    
    UErrorCode status = U_ZERO_ERROR;
    int32_t offset = 90 * 60000; 
    SimpleTimeZone* z1 = new SimpleTimeZone(offset, "_std_zone_");
    z1->setDSTSavings(45 * 60000, status); 

    
    SimpleTimeZone* z2 = new SimpleTimeZone(offset, "_dst_zone_");
    z2->setDSTSavings(45 * 60000, status); 
    z2->setStartRule(UCAL_JANUARY, 1, 0, status);
    z2->setEndRule(UCAL_JULY, 1, 0, status);

    
    DateFormat* fmt1 = new SimpleDateFormat(UnicodeString("z"), status);
    if (U_FAILURE(status)) {
        dataerrln("Failure trying to construct: %s", u_errorName(status));
        return;
    }
    fmt1->setTimeZone(*z1); 
    DateFormat* fmt2 = new SimpleDateFormat(UnicodeString("z"), status);
    if(!assertSuccess("trying to construct", status))return;
    fmt2->setTimeZone(*z2); 
    Calendar* tempcal = Calendar::createInstance(status);
    tempcal->clear();
    tempcal->set(1970, UCAL_FEBRUARY, 1);
    UDate dst = tempcal->getTime(status); 
    tempcal->set(1970, UCAL_AUGUST, 1);
    UDate std = tempcal->getTime(status); 

    
    UnicodeString a,b,c,d,e,f,g,h,i,j,k,l;
    UnicodeString DATA[] = {
        "z1->getDisplayName(false, SHORT)/std zone",
        z1->getDisplayName(FALSE, TimeZone::SHORT, a), "GMT+1:30",
        "z1->getDisplayName(false, LONG)/std zone",
        z1->getDisplayName(FALSE, TimeZone::LONG, b), "GMT+01:30",
        "z1->getDisplayName(true, SHORT)/std zone",
        z1->getDisplayName(TRUE, TimeZone::SHORT, c), "GMT+1:30",
        "z1->getDisplayName(true, LONG)/std zone",
        z1->getDisplayName(TRUE, TimeZone::LONG, d ), "GMT+01:30",
        "z2->getDisplayName(false, SHORT)/dst zone",
        z2->getDisplayName(FALSE, TimeZone::SHORT, e), "GMT+1:30",
        "z2->getDisplayName(false, LONG)/dst zone",
        z2->getDisplayName(FALSE, TimeZone::LONG, f ), "GMT+01:30",
        "z2->getDisplayName(true, SHORT)/dst zone",
        z2->getDisplayName(TRUE, TimeZone::SHORT, g), "GMT+2:15",
        "z2->getDisplayName(true, LONG)/dst zone",
        z2->getDisplayName(TRUE, TimeZone::LONG, h ), "GMT+02:15",
        "DateFormat.format(std)/std zone", fmt1->format(std, i), "GMT+1:30",
        "DateFormat.format(dst)/std zone", fmt1->format(dst, j), "GMT+1:30",
        "DateFormat.format(std)/dst zone", fmt2->format(std, k), "GMT+1:30",
        "DateFormat.format(dst)/dst zone", fmt2->format(dst, l), "GMT+2:15",
    };

    for (int32_t idx=0; idx<(int32_t)ARRAY_LENGTH(DATA); idx+=3) {
        if (DATA[idx+1]!=(DATA[idx+2])) {
            errln("FAIL: " + DATA[idx] + " -> " + DATA[idx+1] + ", exp " + DATA[idx+2]);
        }
    }
    delete z1;
    delete z2;
    delete fmt1;
    delete fmt2;
    delete tempcal;
}





void TimeZoneRegressionTest::TestJ186() {
    UErrorCode status = U_ZERO_ERROR;
    
    
    
    SimpleTimeZone z(0, "ID");
    
    z.setStartRule(UCAL_FEBRUARY, 1, UCAL_SUNDAY, 0, status);
    failure(status, "setStartRule()");
    if (z.useDaylightTime()) {
        errln("Fail: useDaylightTime true with start rule only");
    }
    
    
    
    z.setEndRule(UCAL_MARCH, -1, UCAL_SUNDAY, 0, status);
    failure(status, "setStartRule()");
    if (!z.useDaylightTime()) {
        errln("Fail: useDaylightTime false with rules set");
    }
    if (z.getDSTSavings() == 0) {
        errln("Fail: dst savings == 0 with rules set");
    }
}












void TimeZoneRegressionTest::TestJ449() {
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString str;

    
    
    
    const char* idWithLocaleData = "America/Los_Angeles";
    const char* idWithoutLocaleData = "US/Pacific";
    const Locale loc("en", "", "");

    TimeZone *zoneWith = TimeZone::createTimeZone(idWithLocaleData);
    TimeZone *zoneWithout = TimeZone::createTimeZone(idWithoutLocaleData);
    
    if (zoneWith->getID(str) != UnicodeString(idWithLocaleData) ||
        zoneWithout->getID(str) != UnicodeString(idWithoutLocaleData)) {
      dataerrln(UnicodeString("Fail: Unable to create zones - wanted ") + idWithLocaleData + ", got " + zoneWith->getID(str) + ", and wanted " + idWithoutLocaleData + " but got " + zoneWithout->getID(str));
    } else {
        GregorianCalendar calWith(*zoneWith, status);
        GregorianCalendar calWithout(*zoneWithout, status);
        SimpleDateFormat fmt("MMM d yyyy hh:mm a zzz", loc, status);
        if (U_FAILURE(status)) {
            errln("Fail: Unable to create GregorianCalendar/SimpleDateFormat");
        } else {
            UDate date = 0;
            UnicodeString strWith, strWithout;
            fmt.setCalendar(calWith);
            fmt.format(date, strWith);
            fmt.setCalendar(calWithout);
            fmt.format(date, strWithout);
            if (strWith == strWithout) {
                logln((UnicodeString)"Ok: " + idWithLocaleData + " -> " +
                      strWith + "; " + idWithoutLocaleData + " -> " +
                      strWithout);
            } else {
                errln((UnicodeString)"FAIL: " + idWithLocaleData + " -> " +
                      strWith + "; " + idWithoutLocaleData + " -> " +
                      strWithout);
            }
        }
    }

    delete zoneWith;
    delete zoneWithout;
}


void
TimeZoneRegressionTest::TestJDK12API()
{
    
    
    UErrorCode ec = U_ZERO_ERROR;
    
    TimeZone *pst = new SimpleTimeZone(-28800*U_MILLIS_PER_SECOND,
                                       "PST",
                                       3,1,-1,120*U_MILLIS_PER_MINUTE,
                                       SimpleTimeZone::WALL_TIME,
                                       9,-1,1,120*U_MILLIS_PER_MINUTE,
                                       SimpleTimeZone::WALL_TIME,
                                       60*U_MILLIS_PER_MINUTE,ec);
    
    TimeZone *cst1 = new SimpleTimeZone(-21600*U_MILLIS_PER_SECOND,
                                       "CST",
                                       3,1,-1,120*U_MILLIS_PER_MINUTE,
                                       SimpleTimeZone::WALL_TIME,
                                       9,-1,1,120*U_MILLIS_PER_MINUTE,
                                       SimpleTimeZone::WALL_TIME,
                                       60*U_MILLIS_PER_MINUTE,ec);
    if (U_FAILURE(ec)) {
        errln("FAIL: SimpleTimeZone constructor");
        return;
    }

    SimpleTimeZone *cst = dynamic_cast<SimpleTimeZone *>(cst1);

    if(pst->hasSameRules(*cst)) {
        errln("FAILURE: PST and CST have same rules");
    }

    UErrorCode status = U_ZERO_ERROR;
    int32_t offset1 = pst->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (2*60*60*1000), status);
    failure(status, "getOffset() failed");


    int32_t offset2 = cst->getOffset(1,
        1997, UCAL_OCTOBER, 26, UCAL_SUNDAY, (2*60*60*1000), 31, status);
    failure(status, "getOffset() failed");

    if(offset1 == offset2)
        errln("FAILURE: Sunday Oct. 26 1997 2:00 has same offset for PST and CST");

    
    pst->getOffset(1,
        1997, UCAL_FIELD_COUNT+1, 26, UCAL_SUNDAY, (2*60*60*1000), status);
    if(U_SUCCESS(status))
        errln("FAILURE: getOffset() succeeded with -1 for month");

    status = U_ZERO_ERROR;
    cst->setDSTSavings(60*60*1000, status);
    failure(status, "setDSTSavings() failed");

    int32_t savings = cst->getDSTSavings();
    if(savings != 60*60*1000) {
        errln("setDSTSavings() failed");
    }

    delete pst;
    delete cst;
}



void TimeZoneRegressionTest::Test4184229() {
    SimpleTimeZone* zone = NULL;
    UErrorCode status = U_ZERO_ERROR;
    zone = new SimpleTimeZone(0, "A", 0, -1, 0, 0, 0, 0, 0, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 startDay");
    }else{
       logln("(a) " + UnicodeString( u_errorName(status)));
    }
    status = U_ZERO_ERROR;
    delete zone;

    zone = new SimpleTimeZone(0, "A", 0, 0, 0, 0, 0, -1, 0, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 endDay");
    }else{
       logln("(b) " + UnicodeString(u_errorName(status)));
    }
    status = U_ZERO_ERROR;
    delete zone;

    zone = new SimpleTimeZone(0, "A", 0, -1, 0, 0, 0, 0, 0, 1000, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 startDay+savings");
    }else{
       logln("(c) " + UnicodeString(u_errorName(status)));
    }
    status = U_ZERO_ERROR;
    delete zone;
    zone = new SimpleTimeZone(0, "A", 0, 0, 0, 0, 0, -1, 0, 0, 1000, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 endDay+ savings");
    }else{
       logln("(d) " + UnicodeString(u_errorName(status)));
    }
    status = U_ZERO_ERROR;
    delete zone;
    
    zone = new SimpleTimeZone(0, "A", 0, 1, 0, 0, 0, 1, 0, 0, status);
    
    zone->setStartRule(0, -1, 0, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 setStartRule +savings");
    } else{
        logln("(e) " + UnicodeString(u_errorName(status)));
    }
    zone->setStartRule(0, -1, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 setStartRule");
    } else{
        logln("(f) " + UnicodeString(u_errorName(status)));
    }

    zone->setEndRule(0, -1, 0, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 setEndRule+savings");
    } else{
        logln("(g) " + UnicodeString(u_errorName(status)));
    }   

    zone->setEndRule(0, -1, 0, status);
    if(U_SUCCESS(status)){
        errln("Failed. No exception has been thrown for DOM -1 setEndRule");
    } else{
        logln("(h) " + UnicodeString(u_errorName(status)));
    }
    delete zone;
}

#endif 
