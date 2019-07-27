







#include "unicode/utypes.h"
#include "string.h"
#include "unicode/locid.h"
#include "japancal.h"

#if !UCONFIG_NO_FORMATTING

#include <stdio.h>
#include "caltest.h"

#define CHECK(status, msg) \
    if (U_FAILURE(status)) { \
      dataerrln((UnicodeString(u_errorName(status)) + UnicodeString(" : " ) )+ msg); \
        return; \
    }


static UnicodeString escape( const UnicodeString&src)
{
  UnicodeString dst;
    dst.remove();
    for (int32_t i = 0; i < src.length(); ++i) {
        UChar c = src[i];
        if(c < 0x0080) 
            dst += c;
        else {
            dst += UnicodeString("[");
            char buf [8];
            sprintf(buf, "%#x", c);
            dst += UnicodeString(buf);
            dst += UnicodeString("]");
        }
    }

    return dst;
}


#include "incaltst.h"
#include "unicode/gregocal.h"
#include "unicode/smpdtfmt.h"
#include "unicode/simpletz.h"
 






#define U_DEBUG_DUMPCALS  


#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break


void IntlCalendarTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char*  )
{
    if (exec) logln("TestSuite IntlCalendarTest");
    switch (index) {
    CASE(0,TestTypes);
    CASE(1,TestGregorian);
    CASE(2,TestBuddhist);
    CASE(3,TestJapanese);
    CASE(4,TestBuddhistFormat);
    CASE(5,TestJapaneseFormat);
    CASE(6,TestJapanese3860);
    CASE(7,TestPersian);
    CASE(8,TestPersianFormat);
    CASE(9,TestTaiwan);
    default: name = ""; break;
    }
}

#undef CASE







void
IntlCalendarTest::TestTypes()
{
  Calendar *c = NULL;
  UErrorCode status = U_ZERO_ERROR;
  int j;
  const char *locs [40] = { "en_US_VALLEYGIRL",     
                            "en_US_VALLEYGIRL@collation=phonebook;calendar=japanese",
                            "en_US_VALLEYGIRL@collation=phonebook;calendar=gregorian",
                            "ja_JP@calendar=japanese",   
                            "th_TH@calendar=buddhist", 
                            "ja_JP_TRADITIONAL",   
                            "th_TH_TRADITIONAL", 
                            "th_TH_TRADITIONAL@calendar=gregorian", 
                            "en_US",
                            "th_TH",    
                            "th",       
                            "en_TH",    
                            "en-TH-u-ca-gregory",
                            NULL };
  const char *types[40] = { "gregorian", 
                            "japanese",
                            "gregorian",
                            "japanese",
                            "buddhist",
                            "japanese",
                            "buddhist",           
                            "gregorian",
                            "gregorian",
                            "buddhist",           
                            "buddhist",           
                            "buddhist",           
                            "gregorian",
                            NULL };

  for(j=0;locs[j];j++) {
    logln(UnicodeString("Creating calendar of locale ")  + locs[j]);
    status = U_ZERO_ERROR;
    c = Calendar::createInstance(locs[j], status);
    CHECK(status, "creating '" + UnicodeString(locs[j]) + "' calendar");
    if(U_SUCCESS(status)) {
      logln(UnicodeString(" type is ") + c->getType());
      if(strcmp(c->getType(), types[j])) {
        dataerrln(UnicodeString(locs[j]) + UnicodeString("Calendar type ") + c->getType() + " instead of " + types[j]);
      }
    }
    delete c;
  }
}










void IntlCalendarTest::quasiGregorianTest(Calendar& cal, const Locale& gcl, const int32_t *data) {
  UErrorCode status = U_ZERO_ERROR;
  
  
  
  
  Calendar *grego = Calendar::createInstance(gcl, status);
  if (U_FAILURE(status)) {
    dataerrln("Error calling Calendar::createInstance"); 
    return;
  }

  int32_t tz1 = cal.get(UCAL_ZONE_OFFSET,status);
  int32_t tz2 = grego -> get (UCAL_ZONE_OFFSET, status);
  if(tz1 != tz2) { 
    errln((UnicodeString)"cal's tz " + tz1 + " != grego's tz " + tz2);
  }

  for (int32_t i=0; data[i]!=-1; ) {
    int32_t era = data[i++];
    int32_t year = data[i++];
    int32_t gregorianYear = data[i++];
    int32_t month = data[i++];
    int32_t dayOfMonth = data[i++];
    
    grego->clear();
    grego->set(gregorianYear, month, dayOfMonth);
    UDate D = grego->getTime(status);
    
    cal.clear();
    cal.set(UCAL_ERA, era);
    cal.set(year, month, dayOfMonth);
    UDate d = cal.getTime(status);
#ifdef U_DEBUG_DUMPCALS
    logln((UnicodeString)"cal  : " + CalendarTest::calToStr(cal));
    logln((UnicodeString)"grego: " + CalendarTest::calToStr(*grego));
#endif
    if (d == D) {
      logln(UnicodeString("OK: ") + era + ":" + year + "/" + (month+1) + "/" + dayOfMonth +
            " => " + d + " (" + UnicodeString(cal.getType()) + ")");
    } else {
      errln(UnicodeString("Fail: (fields to millis)") + era + ":" + year + "/" + (month+1) + "/" + dayOfMonth +
            " => " + d + ", expected " + D + " (" + UnicodeString(cal.getType()) + "Off by: " + (d-D));
    }
    
    
    cal.clear();
    cal.setTime(D, status);
    int e = cal.get(UCAL_ERA, status);
    int y = cal.get(UCAL_YEAR, status);
#ifdef U_DEBUG_DUMPCALS
    logln((UnicodeString)"cal  : " + CalendarTest::calToStr(cal));
    logln((UnicodeString)"grego: " + CalendarTest::calToStr(*grego));
#endif
    if (y == year && e == era) {
      logln((UnicodeString)"OK: " + D + " => " + cal.get(UCAL_ERA, status) + ":" +
            cal.get(UCAL_YEAR, status) + "/" +
            (cal.get(UCAL_MONTH, status)+1) + "/" + cal.get(UCAL_DATE, status) +  " (" + UnicodeString(cal.getType()) + ")");
    } else {
      errln((UnicodeString)"Fail: (millis to fields)" + D + " => " + cal.get(UCAL_ERA, status) + ":" +
            cal.get(UCAL_YEAR, status) + "/" +
            (cal.get(UCAL_MONTH, status)+1) + "/" + cal.get(UCAL_DATE, status) +
            ", expected " + era + ":" + year + "/" + (month+1) + "/" +
            dayOfMonth +  " (" + UnicodeString(cal.getType()));
    }
  }
  delete grego;
  CHECK(status, "err during quasiGregorianTest()");
}


void IntlCalendarTest::TestGregorian() { 
    UDate timeA = Calendar::getNow();
    int32_t data[] = { 
        GregorianCalendar::AD, 1868, 1868, UCAL_SEPTEMBER, 8,
        GregorianCalendar::AD, 1868, 1868, UCAL_SEPTEMBER, 9,
        GregorianCalendar::AD, 1869, 1869, UCAL_JUNE, 4,
        GregorianCalendar::AD, 1912, 1912, UCAL_JULY, 29,
        GregorianCalendar::AD, 1912, 1912, UCAL_JULY, 30,
        GregorianCalendar::AD, 1912, 1912, UCAL_AUGUST, 1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance( status);
    CHECK(status, UnicodeString("Creating de_CH calendar"));
    
    UDate timeB = Calendar::getNow();
    UDate timeCal = cal->getTime(status);

    if(!(timeA <= timeCal) || !(timeCal <= timeB)) {
      errln((UnicodeString)"Error: Calendar time " + timeCal +
            " is not within sampled times [" + timeA + " to " + timeB + "]!");
    }
    

    
    
    
    

    quasiGregorianTest(*cal,Locale("fr_FR"),data);
    delete cal;
}





void IntlCalendarTest::TestBuddhist() {
    
    UDate timeA = Calendar::getNow();

    int32_t data[] = {
        0,           
        2542,        
        1999,        
        UCAL_JUNE,   
        4,           

        0,           
        3,           
        -540,        
        UCAL_FEBRUARY, 
        12,          

        0,           
        4795,        
        4252,        
        UCAL_FEBRUARY,
        29,

        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance("th_TH@calendar=buddhist", status);
    CHECK(status, UnicodeString("Creating th_TH@calendar=buddhist calendar"));

    
    UDate timeB = Calendar::getNow();
    UDate timeCal = cal->getTime(status);

    if(!(timeA <= timeCal) || !(timeCal <= timeB)) {
      errln((UnicodeString)"Error: Calendar time " + timeCal +
            " is not within sampled times [" + timeA + " to " + timeB + "]!");
    }
    


    quasiGregorianTest(*cal,Locale("th_TH@calendar=gregorian"),data);
    delete cal;
}






void IntlCalendarTest::TestTaiwan() {
    
    UDate timeA = Calendar::getNow();
    
    
    int32_t data[] = {
        1,           
        1,        
        1912,        
        UCAL_JUNE,   
        4,           

        1,           
        3,           
        1914,        
        UCAL_FEBRUARY, 
        12,          

        1,           
        96,           
        2007,        
        UCAL_FEBRUARY, 
        12,          

        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance("en_US@calendar=roc", status);
    CHECK(status, UnicodeString("Creating en_US@calendar=roc calendar"));

    
    UDate timeB = Calendar::getNow();
    UDate timeCal = cal->getTime(status);

    if(!(timeA <= timeCal) || !(timeCal <= timeB)) {
      errln((UnicodeString)"Error: Calendar time " + timeCal +
            " is not within sampled times [" + timeA + " to " + timeB + "]!");
    }
    


    quasiGregorianTest(*cal,Locale("en_US"),data);
    delete cal;
}







void IntlCalendarTest::TestJapanese() {
    UDate timeA = Calendar::getNow();
    
    
#define JapaneseCalendar_MEIJI  232
#define JapaneseCalendar_TAISHO 233
#define JapaneseCalendar_SHOWA  234
#define JapaneseCalendar_HEISEI 235
    
    
    int32_t data[] = { 
        
        JapaneseCalendar_MEIJI, 1, 1868, UCAL_SEPTEMBER, 8,
        JapaneseCalendar_MEIJI, 1, 1868, UCAL_SEPTEMBER, 9,
        JapaneseCalendar_MEIJI, 2, 1869, UCAL_JUNE, 4,
        JapaneseCalendar_MEIJI, 45, 1912, UCAL_JULY, 29,
        JapaneseCalendar_TAISHO, 1, 1912, UCAL_JULY, 30,
        JapaneseCalendar_TAISHO, 1, 1912, UCAL_AUGUST, 1,
        
        
        JapaneseCalendar_SHOWA,     64,   1989,  UCAL_JANUARY, 7,  
        JapaneseCalendar_HEISEI,    1,   1989,  UCAL_JANUARY, 8,   
        JapaneseCalendar_HEISEI,    1,   1989,  UCAL_JANUARY, 9,
        JapaneseCalendar_HEISEI,    1,   1989,  UCAL_DECEMBER, 20,
        JapaneseCalendar_HEISEI,  15,  2003,  UCAL_MAY, 22,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance("ja_JP@calendar=japanese", status);
    CHECK(status, UnicodeString("Creating ja_JP@calendar=japanese calendar"));
    
    UDate timeB = Calendar::getNow();
    UDate timeCal = cal->getTime(status);

    if(!(timeA <= timeCal) || !(timeCal <= timeB)) {
      errln((UnicodeString)"Error: Calendar time " + timeCal +
            " is not within sampled times [" + timeA + " to " + timeB + "]!");
    }
    
    quasiGregorianTest(*cal,Locale("ja_JP"),data);
    delete cal;
}



void IntlCalendarTest::TestBuddhistFormat() {
    UErrorCode status = U_ZERO_ERROR;
    
    
    
    
    UDate aDate = 999932400000.0; 
    SimpleDateFormat *fmt = new SimpleDateFormat(UnicodeString("MMMM d, yyyy G"), Locale("en_US@calendar=buddhist"), status);
    CHECK(status, "creating date format instance");
    SimpleDateFormat *fmt2 = new SimpleDateFormat(UnicodeString("MMMM d, yyyy G"), Locale("en_US@calendar=gregorian"), status);
    CHECK(status, "creating gregorian date format instance");
    if(!fmt) { 
        errln("Coudln't create en_US instance");
    } else {
        UnicodeString str;
        fmt2->format(aDate, str);
        logln(UnicodeString() + "Test Date: " + str);
        str.remove();
        fmt->format(aDate, str);
        logln(UnicodeString() + "as Buddhist Calendar: " + escape(str));
        UnicodeString expected("September 8, 2544 BE");
        if(str != expected) {
            errln("Expected " + escape(expected) + " but got " + escape(str));
        }
        UDate otherDate = fmt->parse(expected, status);
        if(otherDate != aDate) { 
            UnicodeString str3;
            fmt->format(otherDate, str3);
            errln("Parse incorrect of " + escape(expected) + " - wanted " + aDate + " but got " +  otherDate + ", " + escape(str3));
        } else {
            logln("Parsed OK: " + expected);
        }
        delete fmt;
    }
    delete fmt2;
    
    CHECK(status, "Error occured testing Buddhist Calendar in English ");
    
    status = U_ZERO_ERROR;
    
    {
        UnicodeString expect = CharsToUnicodeString("\\u0E27\\u0E31\\u0E19\\u0E40\\u0E2A\\u0E32\\u0E23\\u0E4C\\u0E17\\u0E35\\u0E48"
            " 8 \\u0E01\\u0E31\\u0e19\\u0e22\\u0e32\\u0e22\\u0e19 \\u0e1e.\\u0e28. 2544");
        UDate         expectDate = 999932400000.0;
        Locale        loc("th_TH_TRADITIONAL"); 
        
        simpleTest(loc, expect, expectDate, status);
    }
    status = U_ZERO_ERROR;
    {
        UnicodeString expect = CharsToUnicodeString("\\u0E27\\u0E31\\u0E19\\u0E40\\u0E2A\\u0E32\\u0E23\\u0E4C\\u0E17\\u0E35\\u0E48"
            " 8 \\u0E01\\u0E31\\u0e19\\u0e22\\u0e32\\u0e22\\u0e19 \\u0e1e.\\u0e28. 2544");
        UDate         expectDate = 999932400000.0;
        Locale        loc("th_TH@calendar=buddhist");
        
        simpleTest(loc, expect, expectDate, status);
    }
    status = U_ZERO_ERROR;
    {
        UnicodeString expect = CharsToUnicodeString("\\u0E27\\u0E31\\u0E19\\u0E40\\u0E2A\\u0E32\\u0E23\\u0E4C\\u0E17\\u0E35\\u0E48"
            " 8 \\u0E01\\u0E31\\u0e19\\u0e22\\u0e32\\u0e22\\u0e19 \\u0e04.\\u0e28. 2001");
        UDate         expectDate = 999932400000.0;
        Locale        loc("th_TH@calendar=gregorian");
        
        simpleTest(loc, expect, expectDate, status);
    }
    status = U_ZERO_ERROR;
    {
        UnicodeString expect = CharsToUnicodeString("\\u0E27\\u0E31\\u0E19\\u0E40\\u0E2A\\u0E32\\u0E23\\u0E4C\\u0E17\\u0E35\\u0E48"
            " 8 \\u0E01\\u0E31\\u0e19\\u0e22\\u0e32\\u0e22\\u0e19 \\u0e04.\\u0e28. 2001");
        UDate         expectDate = 999932400000.0;
        Locale        loc("th_TH_TRADITIONAL@calendar=gregorian");
        
        simpleTest(loc, expect, expectDate, status);
    }
}




void IntlCalendarTest::TestJapaneseFormat() {
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance("ja_JP_TRADITIONAL", status);
    CHECK(status, UnicodeString("Creating ja_JP_TRADITIONAL calendar"));
    
    Calendar *cal2 = cal->clone();
    delete cal;
    cal = NULL;
    
    
    
    UDate aDate = 999932400000.0; 
    SimpleDateFormat *fmt = new SimpleDateFormat(UnicodeString("MMMM d, yy G"), Locale("en_US@calendar=japanese"), status);
    SimpleDateFormat *fmt2 = new SimpleDateFormat(UnicodeString("MMMM d, yyyy G"), Locale("en_US@calendar=gregorian"), status);
    CHECK(status, "creating date format instance");
    if(!fmt) { 
        errln("Coudln't create en_US instance");
    } else {
        UnicodeString str;
        fmt2->format(aDate, str);
        logln(UnicodeString() + "Test Date: " + str);
        str.remove();
        fmt->format(aDate, str);
        logln(UnicodeString() + "as Japanese Calendar: " + str);
        UnicodeString expected("September 8, 13 Heisei");
        if(str != expected) {
            errln("Expected " + expected + " but got " + str);
        }
        UDate otherDate = fmt->parse(expected, status);
        if(otherDate != aDate) { 
            UnicodeString str3;
            ParsePosition pp;
            fmt->parse(expected, *cal2, pp);
            fmt->format(otherDate, str3);
            errln("Parse incorrect of " + expected + " - wanted " + aDate + " but got " +  " = " +   otherDate + ", " + str3 + " = " + CalendarTest::calToStr(*cal2) );
            
        } else {
            logln("Parsed OK: " + expected);
        }
        delete fmt;
    }

    
    fmt = new SimpleDateFormat(UnicodeString("G y"), Locale("en_US@calendar=japanese"), status);
    aDate = -3197117222000.0;
    CHECK(status, "creating date format instance");
    if(!fmt) { 
        errln("Coudln't create en_US instance");
    } else {
        UnicodeString str;
        fmt2->format(aDate, str);
        logln(UnicodeString() + "Test Date: " + str);
        str.remove();
        fmt->format(aDate, str);
        logln(UnicodeString() + "as Japanese Calendar: " + str);
        UnicodeString expected("Meiji 1");
        if(str != expected) {
            errln("Expected " + expected + " but got " + str);
        }
        UDate otherDate = fmt->parse(expected, status);
        if(otherDate != aDate) { 
            UnicodeString str3;
            ParsePosition pp;
            fmt->parse(expected, *cal2, pp);
            fmt->format(otherDate, str3);
            errln("Parse incorrect of " + expected + " - wanted " + aDate + " but got " +  " = " +
                otherDate + ", " + str3 + " = " + CalendarTest::calToStr(*cal2) );
        } else {
            logln("Parsed OK: " + expected);
        }
        delete fmt;
    }

    delete cal2;
    delete fmt2;
    CHECK(status, "Error occured");
    
    
    {
        UnicodeString expect = CharsToUnicodeString("\\u5e73\\u621013\\u5e749\\u67088\\u65e5\\u571f\\u66dc\\u65e5");
        UDate         expectDate = 999932400000.0; 
        Locale        loc("ja_JP@calendar=japanese");
        
        status = U_ZERO_ERROR;
        simpleTest(loc, expect, expectDate, status);
    }
    {
        UnicodeString expect = CharsToUnicodeString("\\u5e73\\u621013\\u5e749\\u67088\\u65e5\\u571f\\u66dc\\u65e5");
        UDate         expectDate = 999932400000.0; 
        Locale        loc("ja_JP_TRADITIONAL"); 
        
        status = U_ZERO_ERROR;
        simpleTest(loc, expect, expectDate, status);
    }
    {
        UnicodeString expect = CharsToUnicodeString("\\u5b89\\u6c385\\u5e747\\u67084\\u65e5\\u6728\\u66dc\\u65e5");
        UDate         expectDate = -6106032422000.0; 
        Locale        loc("ja_JP@calendar=japanese");
        
        status = U_ZERO_ERROR;
        simpleTest(loc, expect, expectDate, status);    
        
    }
    {   
        UnicodeString expect = CharsToUnicodeString("\\u662d\\u548c64\\u5e741\\u67086\\u65e5\\u91d1\\u66dc\\u65e5");
        UDate         expectDate = 600076800000.0;
        Locale        loc("ja_JP@calendar=japanese");
        
        status = U_ZERO_ERROR;
        simpleTest(loc, expect, expectDate, status);    
        
    }
    {   
        UnicodeString expect = CharsToUnicodeString("\\u5EB7\\u6B632\\u5e742\\u670829\\u65e5\\u65e5\\u66dc\\u65e5");
        UDate         expectDate =  -16214400422000.0;  
        Locale        loc("ja_JP@calendar=japanese");
        
        status = U_ZERO_ERROR;
        simpleTest(loc, expect, expectDate, status);    
        
    }
}

void IntlCalendarTest::TestJapanese3860()
{
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance("ja_JP@calendar=japanese", status);
    CHECK(status, UnicodeString("Creating ja_JP@calendar=japanese calendar"));
    Calendar *cal2 = cal->clone();
    SimpleDateFormat *fmt2 = new SimpleDateFormat(UnicodeString("HH:mm:ss.S MMMM d, yyyy G"), Locale("en_US@calendar=gregorian"), status);
    UnicodeString str;

    
    {
        
        UDate aDate = 0; 
        
        
        
        logln("Testing parse w/ missing era...");
        SimpleDateFormat *fmt = new SimpleDateFormat(UnicodeString("y.M.d"), Locale("ja_JP@calendar=japanese"), status);
        CHECK(status, "creating date format instance");
        if(!fmt) { 
            errln("Coudln't create en_US instance");
        } else {
            UErrorCode s2 = U_ZERO_ERROR;
            cal2->clear();
            UnicodeString samplestr("1.1.9");
            logln(UnicodeString() + "Test Year: " + samplestr);
            aDate = fmt->parse(samplestr, s2);
            ParsePosition pp=0;
            fmt->parse(samplestr, *cal2, pp);
            CHECK(s2, "parsing the 1.1.9 string");
            logln("*cal2 after 119 parse:");
            str.remove();
            fmt2->format(aDate, str);
            logln(UnicodeString() + "as Gregorian Calendar: " + str);

            cal2->setTime(aDate, s2);
            int32_t gotYear = cal2->get(UCAL_YEAR, s2);
            int32_t gotEra = cal2->get(UCAL_ERA, s2);
            int32_t expectYear = 1;
            int32_t expectEra = JapaneseCalendar::getCurrentEra();
            if((gotYear!=1) || (gotEra != expectEra)) {
                errln(UnicodeString("parse "+samplestr+" of 'y.m.d' as Japanese Calendar, expected year ") + expectYear + 
                    UnicodeString(" and era ") + expectEra +", but got year " + gotYear + " and era " + gotEra + " (Gregorian:" + str +")");
            } else {            
                logln(UnicodeString() + " year: " + gotYear + ", era: " + gotEra);
            }
            delete fmt;
        }
    }
    
    {
        
        UDate aDate = 0; 
        
        
        
        logln("Testing parse w/ just year...");
        SimpleDateFormat *fmt = new SimpleDateFormat(UnicodeString("y"), Locale("ja_JP@calendar=japanese"), status);
        CHECK(status, "creating date format instance");
        if(!fmt) { 
            errln("Coudln't create en_US instance");
        } else {
            UErrorCode s2 = U_ZERO_ERROR;
            cal2->clear();
            UnicodeString samplestr("1");
            logln(UnicodeString() + "Test Year: " + samplestr);
            aDate = fmt->parse(samplestr, s2);
            ParsePosition pp=0;
            fmt->parse(samplestr, *cal2, pp);
            CHECK(s2, "parsing the 1 string");
            logln("*cal2 after 1 parse:");
            str.remove();
            fmt2->format(aDate, str);
            logln(UnicodeString() + "as Gregorian Calendar: " + str);

            cal2->setTime(aDate, s2);
            int32_t gotYear = cal2->get(UCAL_YEAR, s2);
            int32_t gotEra = cal2->get(UCAL_ERA, s2);
            int32_t expectYear = 1;
            int32_t expectEra = 235; 
            if((gotYear!=1) || (gotEra != expectEra)) {
                errln(UnicodeString("parse "+samplestr+" of 'y' as Japanese Calendar, expected year ") + expectYear + 
                    UnicodeString(" and era ") + expectEra +", but got year " + gotYear + " and era " + gotEra + " (Gregorian:" + str +")");
            } else {            
                logln(UnicodeString() + " year: " + gotYear + ", era: " + gotEra);
            }
            delete fmt;
        }
    }    

    delete cal2;
    delete cal;
    delete fmt2;
}







void IntlCalendarTest::TestPersian() {
    UDate timeA = Calendar::getNow();
    
    Calendar *cal;
    UErrorCode status = U_ZERO_ERROR;
    cal = Calendar::createInstance("fa_IR@calendar=persian", status);
    CHECK(status, UnicodeString("Creating fa_IR@calendar=persian calendar"));
    
    UDate timeB = Calendar::getNow();
    UDate timeCal = cal->getTime(status);

    if(!(timeA <= timeCal) || !(timeCal <= timeB)) {
      errln((UnicodeString)"Error: Calendar time " + timeCal +
            " is not within sampled times [" + timeA + " to " + timeB + "]!");
    }
    

    
    int32_t data[] = { 
        1925, 4, 24, 1304, 2, 4,
        2011, 1, 11, 1389, 10, 21,
        1986, 2, 25, 1364, 12, 6, 
        1934, 3, 14, 1312, 12, 23,

        2090, 3, 19, 1468, 12, 29,
        2007, 2, 22, 1385, 12, 3,
        1969, 12, 31, 1348, 10, 10,
        1945, 11, 12, 1324, 8, 21,
        1925, 3, 31, 1304, 1, 11,

        1996, 3, 19, 1374, 12, 29,
        1996, 3, 20, 1375, 1, 1,
        1997, 3, 20, 1375, 12, 30,
        1997, 3, 21, 1376, 1, 1,

        2008, 3, 19, 1386, 12, 29,
        2008, 3, 20, 1387, 1, 1,
        2004, 3, 19, 1382, 12, 29,
        2004, 3, 20, 1383, 1, 1,

        2006, 3, 20, 1384, 12, 29,
        2006, 3, 21, 1385, 1, 1,

        2005, 4, 20, 1384, 1, 31,
        2005, 4, 21, 1384, 2, 1,
        2005, 5, 21, 1384, 2, 31,
        2005, 5, 22, 1384, 3, 1,
        2005, 6, 21, 1384, 3, 31,
        2005, 6, 22, 1384, 4, 1,
        2005, 7, 22, 1384, 4, 31,
        2005, 7, 23, 1384, 5, 1,
        2005, 8, 22, 1384, 5, 31,
        2005, 8, 23, 1384, 6, 1,
        2005, 9, 22, 1384, 6, 31,
        2005, 9, 23, 1384, 7, 1,
        2005, 10, 22, 1384, 7, 30,
        2005, 10, 23, 1384, 8, 1,
        2005, 11, 21, 1384, 8, 30,
        2005, 11, 22, 1384, 9, 1,
        2005, 12, 21, 1384, 9, 30,
        2005, 12, 22, 1384, 10, 1,
        2006, 1, 20, 1384, 10, 30,
        2006, 1, 21, 1384, 11, 1,
        2006, 2, 19, 1384, 11, 30,
        2006, 2, 20, 1384, 12, 1,
        2006, 3, 20, 1384, 12, 29,
        2006, 3, 21, 1385, 1, 1,

        
        2025, 3, 21, 1404, 1, 1,
        
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };

    Calendar *grego = Calendar::createInstance("fa_IR@calendar=gregorian", status);
    for (int32_t i=0; data[i]!=-1; ) {
        int32_t gregYear = data[i++];
        int32_t gregMonth = data[i++]-1;
        int32_t gregDay = data[i++];
        int32_t persYear = data[i++];
        int32_t persMonth = data[i++]-1;
        int32_t persDay = data[i++];
        
        
        grego->clear();
        grego->set(gregYear, gregMonth, gregDay);

        cal->clear();
        cal->set(persYear, persMonth, persDay);

        UDate persTime = cal->getTime(status);
        UDate gregTime = grego->getTime(status);

        if (persTime != gregTime) {
          errln(UnicodeString("Expected ") + gregTime + " but got " + persTime);
        }

        
        cal->clear();
        cal->setTime(gregTime, status);

        int32_t computedYear = cal->get(UCAL_YEAR, status);
        int32_t computedMonth = cal->get(UCAL_MONTH, status);
        int32_t computedDay = cal->get(UCAL_DATE, status);

        if ((persYear != computedYear) ||
            (persMonth != computedMonth) ||
            (persDay != computedDay)) {
          errln(UnicodeString("Expected ") + persYear + "/" + (persMonth+1) + "/" + persDay +
                " but got " +  computedYear + "/" + (computedMonth+1) + "/" + computedDay);
        }

    }

    delete cal;
    delete grego;
}

void IntlCalendarTest::TestPersianFormat() {
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *fmt = new SimpleDateFormat(UnicodeString("MMMM d, yyyy G"), Locale(" en_US@calendar=persian"), status);
    CHECK(status, "creating date format instance");
    SimpleDateFormat *fmt2 = new SimpleDateFormat(UnicodeString("MMMM d, yyyy G"), Locale("en_US@calendar=gregorian"), status);
    CHECK(status, "creating gregorian date format instance");
    UnicodeString gregorianDate("January 18, 2007 AD");
    UDate aDate = fmt2->parse(gregorianDate, status); 
    if(!fmt) { 
        errln("Coudln't create en_US instance");
    } else {
        UnicodeString str;
        fmt->format(aDate, str);
        logln(UnicodeString() + "as Persian Calendar: " + escape(str));
        UnicodeString expected("Dey 28, 1385 AP");
        if(str != expected) {
            errln("Expected " + escape(expected) + " but got " + escape(str));
        }
        UDate otherDate = fmt->parse(expected, status); 
        if(otherDate != aDate) { 
            UnicodeString str3;
            fmt->format(otherDate, str3);
            errln("Parse incorrect of " + escape(expected) + " - wanted " + aDate + " but got " +  otherDate + ", " + escape(str3)); 
        } else {
            logln("Parsed OK: " + expected);
        }
        
        fmt->applyPattern("yy-MM-dd");
        str.remove();
        fmt->format(aDate, str);
        expected.setTo("85-10-28");
        if(str != expected) {
            errln("Expected " + escape(expected) + " but got " + escape(str));
        }
        otherDate = fmt->parse(expected, status);
        if (otherDate != aDate) {
            errln("Parse incorrect of " + escape(expected) + " - wanted " + aDate + " but got " + otherDate); 
        } else {
            logln("Parsed OK: " + expected);
        }
        delete fmt;
    }
    delete fmt2;
    
    CHECK(status, "Error occured testing Persian Calendar in English "); 
}


void IntlCalendarTest::simpleTest(const Locale& loc, const UnicodeString& expect, UDate expectDate, UErrorCode& status)
{
    UnicodeString tmp;
    UDate         d;
    DateFormat *fmt0 = DateFormat::createDateTimeInstance(DateFormat::kFull, DateFormat::kFull);

    logln("Try format/parse of " + (UnicodeString)loc.getName());
    DateFormat *fmt2 = DateFormat::createDateInstance(DateFormat::kFull, loc);
    if(fmt2) { 
        fmt2->format(expectDate, tmp);
        logln(escape(tmp) + " ( in locale " + loc.getName() + ")");
        if(tmp != expect) {
            errln(UnicodeString("Failed to format " ) + loc.getName() + " expected " + escape(expect) + " got " + escape(tmp) );
        }

        d = fmt2->parse(expect,status);
        CHECK(status, "Error occured parsing " + UnicodeString(loc.getName()));
        if(d != expectDate) {
            fmt2->format(d,tmp);
            errln(UnicodeString("Failed to parse " ) + escape(expect) + ", " + loc.getName() + " expect " + (double)expectDate + " got " + (double)d  + " " + escape(tmp));
            logln( "wanted " + escape(fmt0->format(expectDate,tmp.remove())) + " but got " + escape(fmt0->format(d,tmp.remove())));
        }
        delete fmt2;
    } else {
        errln((UnicodeString)"Can't create " + loc.getName() + " date instance");
    }
    delete fmt0;
}

#undef CHECK

#endif 


