











#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/tstdtmod.h"
#include "tsdate.h"
#include "dadrfmt.h"
#include "unicode/calendar.h"
#include "intltest.h"
#include <string.h>
#include "unicode/schriter.h"
#include "unicode/regex.h"
#include "unicode/smpdtfmt.h"
#include "dbgutil.h"
#include "fldset.h"


#include <stdio.h>

DataDrivenFormatTest::DataDrivenFormatTest() {
    UErrorCode status = U_ZERO_ERROR;
    driver = TestDataModule::getTestDataModule("format", *this, status);
}

DataDrivenFormatTest::~DataDrivenFormatTest() {
    delete driver;
}

void DataDrivenFormatTest::runIndexedTest(int32_t index, UBool exec,
        const char* &name, char* ) {
    if (driver != NULL) {
        if (exec) {
            
        }
        const DataMap *info= NULL;
        UErrorCode status= U_ZERO_ERROR;
        TestData *testData = driver->createTestData(index, status);
        if (U_SUCCESS(status)) {
            name = testData->getName();
            if (testData->getInfo(info, status)) {
                log(info->getString("Description", status));
            }
            if (exec) {
                log(name);
                logln("---");
                logln("");

                processTest(testData);
            }
            delete testData;
        } else {
            name = "";
        }
    } else {
        dataerrln("format/DataDriven*Test data (format.res) not initialized!");
        name = "";
    }

}





















void DataDrivenFormatTest::testConvertDate(TestData *testData,
        const DataMap * , UBool fmt) {
    UnicodeString kPATTERN("PATTERN="); 
    UnicodeString kMILLIS("MILLIS="); 
    UnicodeString kRELATIVE_MILLIS("RELATIVE_MILLIS="); 
    UnicodeString kRELATIVE_ADD("RELATIVE_ADD:"); 
    
    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat basicFmt(UnicodeString("EEE MMM dd yyyy / YYYY'-W'ww-ee"),
            status);
    if (U_FAILURE(status)) {
        dataerrln("FAIL: Couldn't create basic SimpleDateFormat: %s",
                u_errorName(status));
        return;
    }

    const DataMap *currentCase= NULL;
    
    int n = 0;
    while (testData->nextCase(currentCase, status)) {
        char calLoc[256] = "";
        DateTimeStyleSet styleSet;
        UnicodeString pattern;
        UBool usePattern = FALSE;
        (void)usePattern;   
        CalendarFieldsSet fromSet;
        UDate fromDate = 0;
        UBool useDate = FALSE;
        
        UDate now = Calendar::getNow();
        
        ++n;

        char theCase[200];
        sprintf(theCase, "case %d:", n);
        UnicodeString caseString(theCase, "");
        
        
        UnicodeString locale = currentCase->getString("locale", status);
        if (U_FAILURE(status)) {
            errln("case %d: No 'locale' line.", n);
            continue;
        }
        UnicodeString zone = currentCase->getString("zone", status);
        if (U_FAILURE(status)) {
            errln("case %d: No 'zone' line.", n);
            continue;
        }
        UnicodeString spec = currentCase->getString("spec", status);
        if(U_FAILURE(status)) {
            errln("case %d: No 'spec' line.", n);
            continue;
        }
        UnicodeString date = currentCase->getString("date", status);
        if(U_FAILURE(status)) {
            errln("case %d: No 'date' line.", n);
            continue;
        }
        UnicodeString expectStr= currentCase->getString("str", status);
        if(U_FAILURE(status)) {
            errln("case %d: No 'str' line.", n);
            continue;
        }
                
        DateFormat *format = NULL;
        
        
        locale.extract(0, locale.length(), calLoc, (const char*)0); 
        Locale loc(calLoc);
        if(spec.startsWith(kPATTERN)) {
            pattern = UnicodeString(spec,kPATTERN.length());
            usePattern = TRUE;
            format = new SimpleDateFormat(pattern, loc, status);
            if(U_FAILURE(status)) {
                errln("case %d: could not create SimpleDateFormat from pattern: %s", n, u_errorName(status));
                continue;
            }
        } else {
            if(styleSet.parseFrom(spec, status)<0 || U_FAILURE(status)) {
                errln("case %d: could not parse spec as style fields: %s", n, u_errorName(status));
                continue;
            }
            format = DateFormat::createDateTimeInstance((DateFormat::EStyle)styleSet.getDateStyle(), (DateFormat::EStyle)styleSet.getTimeStyle(), loc);
            if(format == NULL ) {
                errln("case %d: could not create SimpleDateFormat from styles.", n);
                continue;
            }
        }

        Calendar *cal = Calendar::createInstance(loc, status);
        if(U_FAILURE(status)) {
            errln("case %d: could not create calendar from %s", n, calLoc);
        }
        
        if (zone.length() > 0) {
            TimeZone * tz = TimeZone::createTimeZone(zone);
            cal->setTimeZone(*tz);
            format->setTimeZone(*tz);
            delete tz;
        }

        
        if(date.startsWith(kMILLIS)) {
            UnicodeString millis = UnicodeString(date, kMILLIS.length());
            useDate = TRUE;
            fromDate = udbg_stod(millis);
        } else if(date.startsWith(kRELATIVE_MILLIS)) {
            UnicodeString millis = UnicodeString(date, kRELATIVE_MILLIS.length());
            useDate = TRUE;
            fromDate = udbg_stod(millis) + now;
        } else if(date.startsWith(kRELATIVE_ADD)) {
            UnicodeString add = UnicodeString(date, kRELATIVE_ADD.length());  
            if(fromSet.parseFrom(add, status)<0 || U_FAILURE(status)) {
                errln("case %d: could not parse date as RELATIVE_ADD calendar fields: %s", n, u_errorName(status));
                continue;
            }
            useDate=TRUE;
            cal->clear();
            cal->setTime(now, status);
            for (int q=0; q<UCAL_FIELD_COUNT; q++) {
                if (fromSet.isSet((UCalendarDateFields)q)) {
                    
                    if (q == UCAL_DATE) {
                        cal->add((UCalendarDateFields)q,
                                    fromSet.get((UCalendarDateFields)q), status);
                    } else {
                        cal->set((UCalendarDateFields)q,
                                    fromSet.get((UCalendarDateFields)q));
                    }
                    
                }
            }
            fromDate = cal->getTime(status);
            if(U_FAILURE(status)) {
                errln("case %d: could not apply date as RELATIVE_ADD calendar fields: %s", n, u_errorName(status));
                continue;
            }
        } else if(fromSet.parseFrom(date, status)<0 || U_FAILURE(status)) {
            errln("case %d: could not parse date as calendar fields: %s", n, u_errorName(status));
            continue;
        }
        
        
        if (fmt) {
            FieldPosition pos;


            cal->clear();
            UnicodeString output;
            output.remove();
            
            if(useDate) {





                format->format(fromDate, output, pos, status);
            } else {
                fromSet.setOnCalendar(cal, status);
                if(U_FAILURE(status)) {
                    errln("case %d: could not set fields on calendar: %s", n, u_errorName(status));
                    continue;
                }
                format->format(*cal, output, pos);
            }            
            
            
            if(U_FAILURE(status)) {
                errln("case %d: could not format(): %s", n, u_errorName(status)); 
            }



            
            if(output == expectStr) {
                logln(caseString+": format: SUCCESS! "+UnicodeString("expect=output=")+output);
            } else {
                UnicodeString result;
                UnicodeString result2;
                errln(caseString+": format:  output!=expectStr, got " + *udbg_escape(output, &result) + " expected " + *udbg_escape(expectStr, &result2));
            }
        } else {
            cal->clear();
            ParsePosition pos;
            format->parse(expectStr,*cal,pos);
            if(useDate) {
                UDate gotDate = cal->getTime(status);
                if(U_FAILURE(status)) {
                    errln(caseString+": parse: could not get time on calendar: "+UnicodeString(u_errorName(status)));
                    continue;
                }
                if(gotDate == fromDate) {
                    logln(caseString+": parse: SUCCESS! "+UnicodeString("gotDate=parseDate=")+expectStr);
                } else {
                    UnicodeString expectDateStr, gotDateStr;
                    basicFmt.format(fromDate,expectDateStr);
                    basicFmt.format(gotDate,gotDateStr);
                    errln(caseString+": parse: FAIL. parsed '"+expectStr+"' and got "+gotDateStr+", expected " + expectDateStr);
                }
            } else {



                if(U_FAILURE(status)) {
                    errln("case %d: parse: could not set fields on calendar: %s", n, u_errorName(status));
                    continue;
                }
                
                CalendarFieldsSet diffSet;

                if (!fromSet.matches(cal, diffSet, status)) {
                    UnicodeString diffs = diffSet.diffFrom(fromSet, status);
                    errln((UnicodeString)"FAIL: "+caseString
                            +", Differences: '"+ diffs
                            +"', status: "+ u_errorName(status));
                } else if (U_FAILURE(status)) {
                    errln("FAIL: "+caseString+" parse SET SOURCE calendar Failed to match: "
                            +u_errorName(status));
                } else {
                    logln("PASS: "+caseString+" parse.");
                }
                

                
            }
        }
        delete cal;
        delete format;

    }

}

void DataDrivenFormatTest::processTest(TestData *testData) {
    
    
    
    char testType[256];
    const DataMap *settings= NULL;
    
    UErrorCode status = U_ZERO_ERROR;
    UnicodeString testSetting;
    int n = 0;
    while (testData->nextSettings(settings, status)) {
        status = U_ZERO_ERROR;
        
        testSetting = settings->getString("Type", status);
        if (U_SUCCESS(status)) {
            if ((++n)>0) {
                logln("---");
            }
            logln(testSetting + "---");
            testSetting.extract(0, testSetting.length(), testType, "");
        } else {
            errln("Unable to extract 'Type'. Skipping..");
            continue;
        }

        if (!strcmp(testType, "date_format")) {
            testConvertDate(testData, settings, true);
        } else if (!strcmp(testType, "date_parse")) {
            testConvertDate(testData, settings, false);
        } else {
            errln("Unknown type: %s", testType);
        }
    }
}

#endif
