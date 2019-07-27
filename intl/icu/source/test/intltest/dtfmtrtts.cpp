




 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/gregocal.h"
#include "dtfmtrtts.h"
#include "caltest.h"
#include "cstring.h"

#include <stdio.h>
#include <string.h>








#ifndef INFINITE
#define INFINITE 0
#endif








int32_t DateFormatRoundTripTest::SPARSENESS = 0;
int32_t DateFormatRoundTripTest::TRIALS = 4;
int32_t DateFormatRoundTripTest::DEPTH = 5;

DateFormatRoundTripTest::DateFormatRoundTripTest() : dateFormat(0) {
}

DateFormatRoundTripTest::~DateFormatRoundTripTest() {
    delete dateFormat;
}

#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break;

void 
DateFormatRoundTripTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* par )
{
    optionv = (par && *par=='v');
    switch (index) {
        CASE(0,TestDateFormatRoundTrip)
        CASE(1, TestCentury)
        default: name = ""; break;
    }
}

UBool 
DateFormatRoundTripTest::failure(UErrorCode status, const char* msg)
{
    if(U_FAILURE(status)) {
        errln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        return TRUE;
    }

    return FALSE;
}

UBool 
DateFormatRoundTripTest::failure(UErrorCode status, const char* msg, const UnicodeString& str)
{
    if(U_FAILURE(status)) {
        UnicodeString escaped;
        escape(str,escaped);
        errln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status) + ", str=" + escaped);
        return TRUE;
    }

    return FALSE;
}

void DateFormatRoundTripTest::TestCentury()
{
    UErrorCode status = U_ZERO_ERROR;
    Locale locale("es_PA");
    UnicodeString pattern = "MM/dd/yy hh:mm:ss a z";
    SimpleDateFormat fmt(pattern, locale, status);
    if (U_FAILURE(status)) {
        dataerrln("Fail: construct SimpleDateFormat: %s", u_errorName(status));
        return;
    }
    UDate date[] = {-55018555891590.05, 0, 0};
    UnicodeString result[2];

    fmt.format(date[0], result[0]);
    date[1] = fmt.parse(result[0], status);
    fmt.format(date[1], result[1]);
    date[2] = fmt.parse(result[1], status);

    











    


    
    if (date[1] != date[2]) {
        errln("Round trip failure: \"%S\" (%f), \"%S\" (%f)", result[0].getBuffer(), date[1], result[1].getBuffer(), date[2]);
    }
}



void DateFormatRoundTripTest::TestDateFormatRoundTrip() 
{
    UErrorCode status = U_ZERO_ERROR;

    getFieldCal = Calendar::createInstance(status);
    if (U_FAILURE(status)) {
        dataerrln("Fail: Calendar::createInstance: %s", u_errorName(status));
        return;
    }


    int32_t locCount = 0;
    const Locale *avail = DateFormat::getAvailableLocales(locCount);
    logln("DateFormat available locales: %d", locCount);
    if(quick) {
        SPARSENESS = 18;
        logln("Quick mode: only testing SPARSENESS = 18");
    }
    TimeZone *tz = TimeZone::createDefault();
    UnicodeString temp;
    logln("Default TimeZone:             " + tz->getID(temp));
    delete tz;

#ifdef TEST_ONE_LOC 
    Locale loc(TEST_ONE_LOC);
    test(loc);
#if INFINITE
    for(;;) {
      test(loc);
    }
#endif

#else
# if INFINITE
    
    Locale loc = Locale::getDefault();
    logln("ENTERING INFINITE TEST LOOP FOR Locale: " + loc.getDisplayName(temp));
    for(;;) 
        test(loc);
# else
    test(Locale::getDefault());

#if 1
    
    for (int i=0; i < locCount; ++i) {
        test(avail[i]);
    }
#endif

#if 1
    
    int32_t jCount = CalendarTest::testLocaleCount();
    for (int32_t j=0; j < jCount; ++j) {
        test(Locale(CalendarTest::testLocaleID(j)));
    }
#endif

# endif
#endif

    delete getFieldCal;
}

static const char *styleName(DateFormat::EStyle s)
{
    switch(s)
    {
    case DateFormat::SHORT: return "SHORT";
    case DateFormat::MEDIUM: return "MEDIUM";
    case DateFormat::LONG: return "LONG";
    case DateFormat::FULL: return "FULL";

    case DateFormat::DATE_OFFSET: return "DATE_OFFSET";
    case DateFormat::NONE: return "NONE";
    case DateFormat::DATE_TIME: return "DATE_TIME";
    default: return "Unknown";
    }
}

void DateFormatRoundTripTest::test(const Locale& loc) 
{
    UnicodeString temp;
#if !INFINITE
    logln("Locale: " + loc.getDisplayName(temp));
#endif

    
    
    
    
    UBool TEST_TABLE [24];
    int32_t i = 0;
    for(i = 0; i < 24; ++i) 
        TEST_TABLE[i] = TRUE;

    
    
    for(i = 0; i < SPARSENESS; ) {
        int random = (int)(randFraction() * 24);
        if (random >= 0 && random < 24 && TEST_TABLE[i]) {
            TEST_TABLE[i] = FALSE;
            ++i;
        }
    }

    int32_t itable = 0;
    int32_t style = 0;
    for(style = DateFormat::FULL; style <= DateFormat::SHORT; ++style) {
        if(TEST_TABLE[itable++]) {
            logln("Testing style " + UnicodeString(styleName((DateFormat::EStyle)style)));
            DateFormat *df = DateFormat::createDateInstance((DateFormat::EStyle)style, loc);
            if(df == NULL) {
              errln(UnicodeString("Could not DF::createDateInstance ") + UnicodeString(styleName((DateFormat::EStyle)style)) +      " Locale: " + loc.getDisplayName(temp));
            } else {
              test(df, loc);
              delete df;
            }
        }
    }
    
    for(style = DateFormat::FULL; style <= DateFormat::SHORT; ++style) {
        if (TEST_TABLE[itable++]) {
          logln("Testing style " + UnicodeString(styleName((DateFormat::EStyle)style)));
            DateFormat *df = DateFormat::createTimeInstance((DateFormat::EStyle)style, loc);
            if(df == NULL) {
              errln(UnicodeString("Could not DF::createTimeInstance ") + UnicodeString(styleName((DateFormat::EStyle)style)) + " Locale: " + loc.getDisplayName(temp));
            } else {
              test(df, loc, TRUE);
              delete df;
            }
        }
    }
    
    for(int32_t dstyle = DateFormat::FULL; dstyle <= DateFormat::SHORT; ++dstyle) {
        for(int32_t tstyle = DateFormat::FULL; tstyle <= DateFormat::SHORT; ++tstyle) {
            if(TEST_TABLE[itable++]) {
                logln("Testing dstyle" + UnicodeString(styleName((DateFormat::EStyle)dstyle)) + ", tstyle" + UnicodeString(styleName((DateFormat::EStyle)tstyle)) );
                DateFormat *df = DateFormat::createDateTimeInstance((DateFormat::EStyle)dstyle, (DateFormat::EStyle)tstyle, loc);
                if(df == NULL) {
                    dataerrln(UnicodeString("Could not DF::createDateTimeInstance ") + UnicodeString(styleName((DateFormat::EStyle)dstyle)) + ", tstyle" + UnicodeString(styleName((DateFormat::EStyle)tstyle))    + "Locale: " + loc.getDisplayName(temp));
                } else {
                    test(df, loc);
                    delete df;
                }
            }
        }
    }
}

void DateFormatRoundTripTest::test(DateFormat *fmt, const Locale &origLocale, UBool timeOnly) 
{
    UnicodeString pat;
    if(fmt->getDynamicClassID() != SimpleDateFormat::getStaticClassID()) {
        errln("DateFormat wasn't a SimpleDateFormat");
        return;
    }
    
    UBool isGregorian = FALSE;
    UErrorCode minStatus = U_ZERO_ERROR;
    if(fmt->getCalendar() == NULL) {
      errln((UnicodeString)"DateFormatRoundTripTest::test, DateFormat getCalendar() returns null for " + origLocale.getName());
      return;
    } 
    UDate minDate = CalendarTest::minDateOfCalendar(*fmt->getCalendar(), isGregorian, minStatus);
    if(U_FAILURE(minStatus)) {
      errln((UnicodeString)"Failure getting min date for " + origLocale.getName());
      return;
    } 
    

    pat = ((SimpleDateFormat*)fmt)->toPattern(pat);

    
    
    
    

    UBool hasEra = (pat.indexOf(UnicodeString("G")) != -1);
    UBool hasZoneDisplayName = (pat.indexOf(UnicodeString("z")) != -1) || (pat.indexOf(UnicodeString("v")) != -1) 
        || (pat.indexOf(UnicodeString("V")) != -1);

    
    
    
    
    
    
    
    

    
        for(int i = 0; i < TRIALS; ++i) {
            UDate *d                = new UDate    [DEPTH];
            UnicodeString *s    = new UnicodeString[DEPTH];

            if(isGregorian == TRUE) {
              d[0] = generateDate();
            } else {
              d[0] = generateDate(minDate);
            }

            UErrorCode status = U_ZERO_ERROR;

            
            
            
            
            int loop;
            int dmatch = 0; 
            int smatch = 0; 
            for(loop = 0; loop < DEPTH; ++loop) {
                if (loop > 0)  {
                    d[loop] = fmt->parse(s[loop-1], status);
                    failure(status, "fmt->parse", s[loop-1]+" in locale: " + origLocale.getName() + " with pattern: " + pat);
                    status = U_ZERO_ERROR; 
                }

                s[loop] = fmt->format(d[loop], s[loop]);
                
                
                
                
                if(s[loop].length() == 0) {
                  errln("FAIL: fmt->format gave 0-length string in " + pat + " with number " + d[loop] + " in locale " + origLocale.getName());
                }

                if(loop > 0) {
                    if(smatch == 0) {
                        UBool match = s[loop] == s[loop-1];
                        if(smatch == 0) {
                            if(match) 
                                smatch = loop;
                        }
                        else if( ! match) 
                            errln("FAIL: String mismatch after match");
                    }

                    if(dmatch == 0) {
                        
                        UBool match = d[loop] == d[loop-1];
                        if(dmatch == 0) {
                            if(match) 
                                dmatch = loop;
                        }
                        else if( ! match) 
                            errln("FAIL: Date mismatch after match");
                    }

                    if(smatch != 0 && dmatch != 0) 
                        break;
                }
            }
            
            
            

            
            int maxDmatch = 2;
            int maxSmatch = 1;
            if (dmatch > maxDmatch) {
                
                if(timeOnly && hasZoneDisplayName) {
                    int32_t startRaw, startDst;
                    fmt->getTimeZone().getOffset(d[0], FALSE, startRaw, startDst, status);
                    failure(status, "TimeZone::getOffset");
                    
                    
                    
                    
                    if (startRaw + startDst > -28800000) {
                        maxDmatch = 3;
                        maxSmatch = 2;
                    }
                }
            }

            
            if(smatch > maxSmatch) { 
                UBool in0;
                
                if( ! hasEra && getField(d[0], UCAL_ERA) == GregorianCalendar::BC)
                    maxSmatch = 2;
                
                else if((in0=fmt->getTimeZone().inDaylightTime(d[0], status)) && ! failure(status, "gettingDaylightTime") &&
                         pat.indexOf(UnicodeString("yyyy")) == -1)
                    maxSmatch = 2;
                
                else if (!in0 &&
                         fmt->getTimeZone().inDaylightTime(d[1], status) && !failure(status, "gettingDaylightTime"))
                    maxSmatch = 2;
                
                
                else if (pat.indexOf(UnicodeString("y")) != -1
                        && pat.indexOf(UnicodeString("yyyy")) == -1
                        && getField(d[0], UCAL_YEAR)
                            != getField(d[dmatch], UCAL_YEAR)
                        && !failure(status, "error status [smatch>maxSmatch]")
                        && ((hasZoneDisplayName
                         && (fmt->getTimeZone().inDaylightTime(d[0], status)
                                == fmt->getTimeZone().inDaylightTime(d[dmatch], status)
                            || getField(d[0], UCAL_MONTH) == UCAL_APRIL
                            || getField(d[0], UCAL_MONTH) == UCAL_OCTOBER))
                         || !hasZoneDisplayName)
                         )
                {
                    maxSmatch = 2;
                }
                
                else if (hasZoneDisplayName && d[0] < 0) {
                    maxSmatch = 2;
                }
            }

            



            if(dmatch > maxDmatch || smatch > maxSmatch) {
              const char *type = fmt->getCalendar()->getType();
              if(!strcmp(type,"japanese") || (!strcmp(type,"buddhist"))) {
                maxSmatch = 4;
                maxDmatch = 4;
              } else if(!strcmp(type,"hebrew")) {
                  maxSmatch = 3;
                  maxDmatch = 3;
                }
            }

            
            UBool fail = (dmatch > maxDmatch || smatch > maxSmatch);
            if (optionv || fail) {
                if (fail) {
                    errln(UnicodeString("\nFAIL: Pattern: ") + pat +
                          " in Locale: " + origLocale.getName());
                } else {
                    errln(UnicodeString("\nOk: Pattern: ") + pat +
                          " in Locale: " + origLocale.getName());
                }
                
                logln("Date iters until match=%d (max allowed=%d), string iters until match=%d (max allowed=%d)",
                      dmatch,maxDmatch, smatch, maxSmatch);

                for(int j = 0; j <= loop && j < DEPTH; ++j) {
                    UnicodeString temp;
                    FieldPosition pos(FieldPosition::DONT_CARE);
                    errln((j>0?" P> ":"    ") + fullFormat(d[j]) + " F> " +
                          escape(s[j], temp) + UnicodeString(" d=") + d[j] + 
                          (j > 0 && d[j]==d[j-1]?" d==":"") +
                          (j > 0 && s[j] == s[j-1]?" s==":""));
                }
            }
            delete[] d;
            delete[] s;
        }
    




}

const UnicodeString& DateFormatRoundTripTest::fullFormat(UDate d) {
    UErrorCode ec = U_ZERO_ERROR;
    if (dateFormat == 0) {
        dateFormat = new SimpleDateFormat((UnicodeString)"EEE MMM dd HH:mm:ss.SSS zzz yyyy G", ec);
        if (U_FAILURE(ec) || dateFormat == 0) {
            fgStr = "[FAIL: SimpleDateFormat constructor]";
            delete dateFormat;
            dateFormat = 0;
            return fgStr;
        }
    }
    fgStr.truncate(0);
    dateFormat->format(d, fgStr);
    return fgStr;
}




int32_t DateFormatRoundTripTest::getField(UDate d, int32_t f) {
    
    UErrorCode status = U_ZERO_ERROR;
    getFieldCal->setTime(d, status);
    failure(status, "getfieldCal->setTime");
    int32_t ret = getFieldCal->get((UCalendarDateFields)f, status);
    failure(status, "getfieldCal->get");
    return ret;
}

UnicodeString& DateFormatRoundTripTest::escape(const UnicodeString& src, UnicodeString& dst ) 
{
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

#define U_MILLIS_PER_YEAR (365.25 * 24 * 60 * 60 * 1000)

UDate DateFormatRoundTripTest::generateDate(UDate minDate)
{
  
  if(minDate < (U_MILLIS_PER_YEAR * -(4000-1970))) {
    minDate = (U_MILLIS_PER_YEAR * -(4000-1970));
  }
  for(int i=0;i<8;i++) {
    double a = randFraction();
    
    
    double dateRange = (0.0 - minDate) + (U_MILLIS_PER_YEAR + (8000-1970));
    
    a *= dateRange;

    
    a += minDate;
   
    
    if(a>=minDate) {
      return a;
    }
  }
  return minDate;
}

UDate DateFormatRoundTripTest::generateDate() 
{
    double a = randFraction();
    
    
    a *= 8000;
    
    
    a -= 4000;
    
    
    a *= 365.25 * 24 * 60 * 60 * 1000;

    
    return a;
}

#endif 


