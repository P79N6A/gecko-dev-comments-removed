





#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "tzoffloc.h"

#include "unicode/ucal.h"
#include "unicode/timezone.h"
#include "unicode/calendar.h"
#include "unicode/dtrule.h"
#include "unicode/tzrule.h"
#include "unicode/rbtz.h"
#include "unicode/simpletz.h"
#include "unicode/tzrule.h"
#include "unicode/smpdtfmt.h"
#include "unicode/gregocal.h"

void
TimeZoneOffsetLocalTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char*  )
{
    if (exec) {
        logln("TestSuite TimeZoneOffsetLocalTest");
    }
    switch (index) {
        TESTCASE(0, TestGetOffsetAroundTransition);
        default: name = ""; break;
    }
}




void
TimeZoneOffsetLocalTest::TestGetOffsetAroundTransition() {
    const int32_t NUM_DATES = 10;
    const int32_t NUM_TIMEZONES = 3;

    const int32_t HOUR = 60*60*1000;
    const int32_t MINUTE = 60*1000;

    const int32_t DATES[NUM_DATES][6] = {
        {2006, UCAL_APRIL, 2, 1, 30, 1*HOUR+30*MINUTE},
        {2006, UCAL_APRIL, 2, 2, 00, 2*HOUR},
        {2006, UCAL_APRIL, 2, 2, 30, 2*HOUR+30*MINUTE},
        {2006, UCAL_APRIL, 2, 3, 00, 3*HOUR},
        {2006, UCAL_APRIL, 2, 3, 30, 3*HOUR+30*MINUTE},
        {2006, UCAL_OCTOBER, 29, 0, 30, 0*HOUR+30*MINUTE},
        {2006, UCAL_OCTOBER, 29, 1, 00, 1*HOUR},
        {2006, UCAL_OCTOBER, 29, 1, 30, 1*HOUR+30*MINUTE},
        {2006, UCAL_OCTOBER, 29, 2, 00, 2*HOUR},
        {2006, UCAL_OCTOBER, 29, 2, 30, 2*HOUR+30*MINUTE},
    };

    
    
    const int32_t OFFSETS1[NUM_DATES] = {
        
        -8*HOUR,
        -7*HOUR,
        -7*HOUR,
        -7*HOUR,
        -7*HOUR,

        
        -7*HOUR,
        -8*HOUR,
        -8*HOUR,
        -8*HOUR,
        -8*HOUR,
    };

    
    
    
    
    
    const int32_t OFFSETS2[NUM_DATES][2] = {
        
        {-8*HOUR, 0},
        {-8*HOUR, 0},
        {-8*HOUR, 0},
        {-8*HOUR, 1*HOUR},
        {-8*HOUR, 1*HOUR},

        
        {-8*HOUR, 1*HOUR},
        {-8*HOUR, 0},
        {-8*HOUR, 0},
        {-8*HOUR, 0},
        {-8*HOUR, 0},
    };

    
    
    
    const int32_t OFFSETS3[][2] = {
        
        {-8*HOUR, 0},
        {-8*HOUR, 1*HOUR},
        {-8*HOUR, 1*HOUR},
        {-8*HOUR, 1*HOUR},
        {-8*HOUR, 1*HOUR},

        
        {-8*HOUR, 1*HOUR},
        {-8*HOUR, 1*HOUR},
        {-8*HOUR, 1*HOUR},
        {-8*HOUR, 0},
        {-8*HOUR, 0},
    };

    UErrorCode status = U_ZERO_ERROR;

    int32_t rawOffset, dstOffset;
    TimeZone* utc = TimeZone::createTimeZone("UTC");
    Calendar* cal = Calendar::createInstance(*utc, status);
    if (U_FAILURE(status)) {
        dataerrln("Calendar::createInstance failed: %s", u_errorName(status));
        return;
    }
    cal->clear();

    
    BasicTimeZone *TESTZONES[NUM_TIMEZONES];

    TESTZONES[0] = (BasicTimeZone*)TimeZone::createTimeZone("America/Los_Angeles");
    TESTZONES[1] = new SimpleTimeZone(-8*HOUR, "Simple Pacific Time",
                                        UCAL_APRIL, 1, UCAL_SUNDAY, 2*HOUR,
                                        UCAL_OCTOBER, -1, UCAL_SUNDAY, 2*HOUR, status);
    if (U_FAILURE(status)) {
        errln("SimpleTimeZone constructor failed");
        return;
    }

    InitialTimeZoneRule *ir = new InitialTimeZoneRule(
            "Pacific Standard Time", 
            -8*HOUR,        
            0*HOUR);        

    RuleBasedTimeZone *rbPT = new RuleBasedTimeZone("Rule based Pacific Time", ir);

    DateTimeRule *dtr;
    AnnualTimeZoneRule *atzr;
    const int32_t STARTYEAR = 2000;

    dtr = new DateTimeRule(UCAL_APRIL, 1, UCAL_SUNDAY,
                        2*HOUR, DateTimeRule::WALL_TIME); 
    atzr = new AnnualTimeZoneRule("Pacific Daylight Time",
            -8*HOUR , 1*HOUR , dtr,
            STARTYEAR, AnnualTimeZoneRule::MAX_YEAR);
    rbPT->addTransitionRule(atzr, status);
    if (U_FAILURE(status)) {
        errln("Could not add DST start rule to the RuleBasedTimeZone rbPT");
        return;
    }

    dtr = new DateTimeRule(UCAL_OCTOBER, -1, UCAL_SUNDAY,
                        2*HOUR, DateTimeRule::WALL_TIME); 
    atzr = new AnnualTimeZoneRule("Pacific Standard Time",
            -8*HOUR , 0 , dtr,
            STARTYEAR, AnnualTimeZoneRule::MAX_YEAR);
    rbPT->addTransitionRule(atzr, status);
    if (U_FAILURE(status)) {
        errln("Could not add STD start rule to the RuleBasedTimeZone rbPT");
        return;
    }

    rbPT->complete(status);
    if (U_FAILURE(status)) {
        errln("complete() failed for RuleBasedTimeZone rbPT");
        return;
    }

    TESTZONES[2] = rbPT;

    
    UDate MILLIS[NUM_DATES];
    for (int32_t i = 0; i < NUM_DATES; i++) {
        cal->clear();
        cal->set(DATES[i][0], DATES[i][1], DATES[i][2], DATES[i][3], DATES[i][4]);
        MILLIS[i] = cal->getTime(status);
        if (U_FAILURE(status)) {
            errln("cal->getTime failed");
            return;
        }
    }

    SimpleDateFormat df(UnicodeString("yyyy-MM-dd HH:mm:ss"), status);
    if (U_FAILURE(status)) {
        dataerrln("Failed to initialize a SimpleDateFormat - %s", u_errorName(status));
    }
    df.setTimeZone(*utc);
    UnicodeString dateStr;

    
    
    for (int32_t i = 0; i < NUM_TIMEZONES; i++) {
        for (int32_t d = 0; d < NUM_DATES; d++) {
            status = U_ZERO_ERROR;
            int32_t offset = TESTZONES[i]->getOffset(GregorianCalendar::AD, DATES[d][0], DATES[d][1], DATES[d][2],
                                                UCAL_SUNDAY, DATES[d][5], status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"getOffset(era,year,month,day,dayOfWeek,millis,status) failed for TESTZONES[" + i + "]");
            } else if (offset != OFFSETS1[d]) {
                dateStr.remove();
                df.format(MILLIS[d], dateStr);
                dataerrln((UnicodeString)"Bad offset returned by TESTZONES[" + i + "] at "
                        + dateStr + "(standard) - Got: " + offset + " Expected: " + OFFSETS1[d]);
            }
        }
    }

    
    
    for (int32_t i = 0; i < NUM_TIMEZONES; i++) {
        for (int32_t m = 0; m < NUM_DATES; m++) {
            status = U_ZERO_ERROR;
            TESTZONES[i]->getOffset(MILLIS[m], TRUE, rawOffset, dstOffset, status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"getOffset(date,local,rawOfset,dstOffset,ec) failed for TESTZONES[" + i + "]");
            } else if (rawOffset != OFFSETS2[m][0] || dstOffset != OFFSETS2[m][1]) {
                dateStr.remove();
                df.format(MILLIS[m], dateStr);
                dataerrln((UnicodeString)"Bad offset returned by TESTZONES[" + i + "] at "
                        + dateStr + "(wall) - Got: "
                        + rawOffset + "/" + dstOffset
                        + " Expected: " + OFFSETS2[m][0] + "/" + OFFSETS2[m][1]);
            }
        }
    }

    
    
    
    for (int32_t i = 0; i < NUM_TIMEZONES; i++) {
        for (int m = 0; m < NUM_DATES; m++) {
            status = U_ZERO_ERROR;
            TESTZONES[i]->getOffsetFromLocal(MILLIS[m], BasicTimeZone::kStandard, BasicTimeZone::kStandard,
                rawOffset, dstOffset, status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"getOffsetFromLocal with kStandard/kStandard failed for TESTZONES[" + i + "]");
            } else if (rawOffset != OFFSETS2[m][0] || dstOffset != OFFSETS2[m][1]) {
                dateStr.remove();
                df.format(MILLIS[m], dateStr);
                dataerrln((UnicodeString)"Bad offset returned by TESTZONES[" + i + "] at "
                        + dateStr + "(wall/kStandard/kStandard) - Got: "
                        + rawOffset + "/" + dstOffset
                        + " Expected: " + OFFSETS2[m][0] + "/" + OFFSETS2[m][1]);
            }
        }
    }

    
    
    
    for (int32_t i = 0; i < NUM_TIMEZONES; i++) {
        for (int m = 0; m < NUM_DATES; m++) {
            status = U_ZERO_ERROR;
            TESTZONES[i]->getOffsetFromLocal(MILLIS[m], BasicTimeZone::kDaylight, BasicTimeZone::kDaylight,
                rawOffset, dstOffset, status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"getOffsetFromLocal with kDaylight/kDaylight failed for TESTZONES[" + i + "]");
            } else if (rawOffset != OFFSETS3[m][0] || dstOffset != OFFSETS3[m][1]) {
                dateStr.remove();
                df.format(MILLIS[m], dateStr);
                dataerrln((UnicodeString)"Bad offset returned by TESTZONES[" + i + "] at "
                        + dateStr + "(wall/kDaylight/kDaylight) - Got: "
                        + rawOffset + "/" + dstOffset
                        + " Expected: " + OFFSETS3[m][0] + "/" + OFFSETS3[m][1]);
            }
        }
    }

    
    
    
    for (int32_t i = 0; i < NUM_TIMEZONES; i++) {
        for (int m = 0; m < NUM_DATES; m++) {
            status = U_ZERO_ERROR;
            TESTZONES[i]->getOffsetFromLocal(MILLIS[m], BasicTimeZone::kFormer, BasicTimeZone::kLatter,
                rawOffset, dstOffset, status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"getOffsetFromLocal with kFormer/kLatter failed for TESTZONES[" + i + "]");
            } else if (rawOffset != OFFSETS2[m][0] || dstOffset != OFFSETS2[m][1]) {
                dateStr.remove();
                df.format(MILLIS[m], dateStr);
                dataerrln((UnicodeString)"Bad offset returned by TESTZONES[" + i + "] at "
                        + dateStr + "(wall/kFormer/kLatter) - Got: "
                        + rawOffset + "/" + dstOffset
                        + " Expected: " + OFFSETS2[m][0] + "/" + OFFSETS2[m][1]);
            }
        }
    }

    
    
    
    for (int32_t i = 0; i < NUM_TIMEZONES; i++) {
        for (int m = 0; m < NUM_DATES; m++) {
            status = U_ZERO_ERROR;
            TESTZONES[i]->getOffsetFromLocal(MILLIS[m], BasicTimeZone::kLatter, BasicTimeZone::kFormer,
                rawOffset, dstOffset, status);
            if (U_FAILURE(status)) {
                errln((UnicodeString)"getOffsetFromLocal with kLatter/kFormer failed for TESTZONES[" + i + "]");
            } else if (rawOffset != OFFSETS3[m][0] || dstOffset != OFFSETS3[m][1]) {
                dateStr.remove();
                df.format(MILLIS[m], dateStr);
                dataerrln((UnicodeString)"Bad offset returned by TESTZONES[" + i + "] at "
                        + dateStr + "(wall/kLatter/kFormer) - Got: "
                        + rawOffset + "/" + dstOffset
                        + " Expected: " + OFFSETS3[m][0] + "/" + OFFSETS3[m][1]);
            }
        }
    }

    for (int32_t i = 0; i < NUM_TIMEZONES; i++) {
        delete TESTZONES[i];
    }
    delete utc;
    delete cal;
}

#endif 
