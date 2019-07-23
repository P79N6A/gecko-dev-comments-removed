




















































#include "jsstddef.h"
#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsprf.h"
#include "prmjtime.h"
#include "jsutil.h" 
#include "jsapi.h"
#include "jsconfig.h"
#include "jscntxt.h"
#include "jsdate.h"
#include "jsinterp.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstr.h"














































































#define HalfTimeDomain  8.64e15
#define HoursPerDay     24.0
#define MinutesPerDay   (HoursPerDay * MinutesPerHour)
#define MinutesPerHour  60.0
#define SecondsPerDay   (MinutesPerDay * SecondsPerMinute)
#define SecondsPerHour  (MinutesPerHour * SecondsPerMinute)
#define SecondsPerMinute 60.0

#if defined(XP_WIN) || defined(XP_OS2)





static jsdouble msPerSecond = 1000.0;
static jsdouble msPerDay = SecondsPerDay * 1000.0;
static jsdouble msPerHour = SecondsPerHour * 1000.0;
static jsdouble msPerMinute = SecondsPerMinute * 1000.0;
#else
#define msPerDay        (SecondsPerDay * msPerSecond)
#define msPerHour       (SecondsPerHour * msPerSecond)
#define msPerMinute     (SecondsPerMinute * msPerSecond)
#define msPerSecond     1000.0
#endif

#define Day(t)          floor((t) / msPerDay)

static jsdouble
TimeWithinDay(jsdouble t)
{
    jsdouble result;
    result = fmod(t, msPerDay);
    if (result < 0)
        result += msPerDay;
    return result;
}

#define DaysInYear(y)   ((y) % 4 == 0 && ((y) % 100 || ((y) % 400 == 0))  \
                         ? 366 : 365)




#define DayFromYear(y)  (365 * ((y)-1970) + floor(((y)-1969)/4.0)            \
                         - floor(((y)-1901)/100.0) + floor(((y)-1601)/400.0))
#define TimeFromYear(y) (DayFromYear(y) * msPerDay)

static jsint
YearFromTime(jsdouble t)
{
    jsint y = (jsint) floor(t /(msPerDay*365.2425)) + 1970;
    jsdouble t2 = (jsdouble) TimeFromYear(y);

    if (t2 > t) {
        y--;
    } else {
        if (t2 + msPerDay * DaysInYear(y) <= t)
            y++;
    }
    return y;
}

#define InLeapYear(t)   (JSBool) (DaysInYear(YearFromTime(t)) == 366)

#define DayWithinYear(t, year) ((intN) (Day(t) - DayFromYear(year)))





static jsdouble firstDayOfMonth[2][12] = {
    {0.0, 31.0, 59.0, 90.0, 120.0, 151.0, 181.0, 212.0, 243.0, 273.0, 304.0, 334.0},
    {0.0, 31.0, 60.0, 91.0, 121.0, 152.0, 182.0, 213.0, 244.0, 274.0, 305.0, 335.0}
};

#define DayFromMonth(m, leap) firstDayOfMonth[leap][(intN)m];

static intN
MonthFromTime(jsdouble t)
{
    intN d, step;
    jsint year = YearFromTime(t);
    d = DayWithinYear(t, year);

    if (d < (step = 31))
        return 0;
    step += (InLeapYear(t) ? 29 : 28);
    if (d < step)
        return 1;
    if (d < (step += 31))
        return 2;
    if (d < (step += 30))
        return 3;
    if (d < (step += 31))
        return 4;
    if (d < (step += 30))
        return 5;
    if (d < (step += 31))
        return 6;
    if (d < (step += 31))
        return 7;
    if (d < (step += 30))
        return 8;
    if (d < (step += 31))
        return 9;
    if (d < (step += 30))
        return 10;
    return 11;
}

static intN
DateFromTime(jsdouble t)
{
    intN d, step, next;
    jsint year = YearFromTime(t);
    d = DayWithinYear(t, year);

    if (d <= (next = 30))
        return d + 1;
    step = next;
    next += (InLeapYear(t) ? 29 : 28);
    if (d <= next)
        return d - step;
    step = next;
    if (d <= (next += 31))
        return d - step;
    step = next;
    if (d <= (next += 30))
        return d - step;
    step = next;
    if (d <= (next += 31))
        return d - step;
    step = next;
    if (d <= (next += 30))
        return d - step;
    step = next;
    if (d <= (next += 31))
        return d - step;
    step = next;
    if (d <= (next += 31))
        return d - step;
    step = next;
    if (d <= (next += 30))
        return d - step;
    step = next;
    if (d <= (next += 31))
        return d - step;
    step = next;
    if (d <= (next += 30))
        return d - step;
    step = next;
    return d - step;
}

static intN
WeekDay(jsdouble t)
{
    jsint result;
    result = (jsint) Day(t) + 4;
    result = result % 7;
    if (result < 0)
        result += 7;
    return (intN) result;
}

#define MakeTime(hour, min, sec, ms) \
((((hour) * MinutesPerHour + (min)) * SecondsPerMinute + (sec)) * msPerSecond + (ms))

static jsdouble
MakeDay(jsdouble year, jsdouble month, jsdouble date)
{
    JSBool leap;
    jsdouble yearday;
    jsdouble monthday;

    year += floor(month / 12);

    month = fmod(month, 12.0);
    if (month < 0)
        month += 12;

    leap = (DaysInYear((jsint) year) == 366);

    yearday = floor(TimeFromYear(year) / msPerDay);
    monthday = DayFromMonth(month, leap);

    return yearday + monthday + date - 1;
}

#define MakeDate(day, time) ((day) * msPerDay + (time))










static jsint yearStartingWith[2][7] = {
    {1978, 1973, 1974, 1975, 1981, 1971, 1977},
    {1984, 1996, 1980, 1992, 1976, 1988, 1972}
};








static jsint
EquivalentYearForDST(jsint year)
{
    jsint day;
    JSBool isLeapYear;

    day = (jsint) DayFromYear(year) + 4;
    day = day % 7;
    if (day < 0)
        day += 7;

    isLeapYear = (DaysInYear(year) == 366);

    return yearStartingWith[isLeapYear][day];
}


static jsdouble LocalTZA;

static jsdouble
DaylightSavingTA(jsdouble t)
{
    volatile int64 PR_t;
    int64 ms2us;
    int64 offset;
    jsdouble result;

    
    if (JSDOUBLE_IS_NaN(t))
        return t;

    



    if (t < 0.0 || t > 2145916800000.0) {
        jsint year;
        jsdouble day;

        year = EquivalentYearForDST(YearFromTime(t));
        day = MakeDay(year, MonthFromTime(t), DateFromTime(t));
        t = MakeDate(day, TimeWithinDay(t));
    }

    
    JSLL_D2L(PR_t, t);
    JSLL_I2L(ms2us, PRMJ_USEC_PER_MSEC);
    JSLL_MUL(PR_t, PR_t, ms2us);

    offset = PRMJ_DSTOffset(PR_t);

    JSLL_DIV(offset, offset, ms2us);
    JSLL_L2D(result, offset);
    return result;
}


#define AdjustTime(t)   fmod(LocalTZA + DaylightSavingTA(t), msPerDay)

#define LocalTime(t)    ((t) + AdjustTime(t))

static jsdouble
UTC(jsdouble t)
{
    return t - AdjustTime(t - LocalTZA);
}

static intN
HourFromTime(jsdouble t)
{
    intN result = (intN) fmod(floor(t/msPerHour), HoursPerDay);
    if (result < 0)
        result += (intN)HoursPerDay;
    return result;
}

static intN
MinFromTime(jsdouble t)
{
    intN result = (intN) fmod(floor(t / msPerMinute), MinutesPerHour);
    if (result < 0)
        result += (intN)MinutesPerHour;
    return result;
}

static intN
SecFromTime(jsdouble t)
{
    intN result = (intN) fmod(floor(t / msPerSecond), SecondsPerMinute);
    if (result < 0)
        result += (intN)SecondsPerMinute;
    return result;
}

static intN
msFromTime(jsdouble t)
{
    intN result = (intN) fmod(t, msPerSecond);
    if (result < 0)
        result += (intN)msPerSecond;
    return result;
}

#define TIMECLIP(d) ((JSDOUBLE_IS_FINITE(d) \
                      && !((d < 0 ? -d : d) > HalfTimeDomain)) \
                     ? js_DoubleToInteger(d + (+0.)) : *cx->runtime->jsNaN)













#define UTC_TIME_SLOT           0
#define LOCAL_TIME_SLOT         1

JSClass js_DateClass = {
    js_Date_str,
    JSCLASS_HAS_RESERVED_SLOTS(2) | JSCLASS_HAS_CACHED_PROTO(JSProto_Date),
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};



static const char* wtb[] = {
    "am", "pm",
    "monday", "tuesday", "wednesday", "thursday", "friday",
    "saturday", "sunday",
    "january", "february", "march", "april", "may", "june",
    "july", "august", "september", "october", "november", "december",
    "gmt", "ut", "utc",
    "est", "edt",
    "cst", "cdt",
    "mst", "mdt",
    "pst", "pdt"
    
};

static int ttb[] = {
    -1, -2, 0, 0, 0, 0, 0, 0, 0,       
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
    10000 + 0, 10000 + 0, 10000 + 0,   
    10000 + 5 * 60, 10000 + 4 * 60,    
    10000 + 6 * 60, 10000 + 5 * 60,    
    10000 + 7 * 60, 10000 + 6 * 60,    
    10000 + 8 * 60, 10000 + 7 * 60     
};


static JSBool
date_regionMatches(const char* s1, int s1off, const jschar* s2, int s2off,
                   int count, int ignoreCase)
{
    JSBool result = JS_FALSE;
    

    while (count > 0 && s1[s1off] && s2[s2off]) {
        if (ignoreCase) {
            if (JS_TOLOWER((jschar)s1[s1off]) != JS_TOLOWER(s2[s2off])) {
                break;
            }
        } else {
            if ((jschar)s1[s1off] != s2[s2off]) {
                break;
            }
        }
        s1off++;
        s2off++;
        count--;
    }

    if (count == 0) {
        result = JS_TRUE;
    }

    return result;
}


static jsdouble
date_msecFromDate(jsdouble year, jsdouble mon, jsdouble mday, jsdouble hour,
                  jsdouble min, jsdouble sec, jsdouble msec)
{
    jsdouble day;
    jsdouble msec_time;
    jsdouble result;

    day = MakeDay(year, mon, mday);
    msec_time = MakeTime(hour, min, sec, msec);
    result = MakeDate(day, msec_time);
    return result;
}




#define MAXARGS        7

static JSBool
date_UTC(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv;
    uintN loop;
    jsdouble array[MAXARGS];
    jsdouble d;

    argv = vp + 2;
    for (loop = 0; loop < MAXARGS; loop++) {
        if (loop < argc) {
            if (!js_ValueToNumber(cx, argv[loop], &d))
                return JS_FALSE;
            
            if (!JSDOUBLE_IS_FINITE(d)) {
                return js_NewNumberValue(cx, d, vp);
            }
            array[loop] = floor(d);
        } else {
            array[loop] = 0;
        }
    }

    
    if (array[0] >= 0 && array[0] <= 99)
        array[0] += 1900;

    

    if (array[2] < 1)
        array[2] = 1;

    d = date_msecFromDate(array[0], array[1], array[2],
                              array[3], array[4], array[5], array[6]);
    d = TIMECLIP(d);

    return js_NewNumberValue(cx, d, vp);
}

static JSBool
date_parseString(JSString *str, jsdouble *result)
{
    jsdouble msec;

    const jschar *s = JSSTRING_CHARS(str);
    size_t limit = JSSTRING_LENGTH(str);
    size_t i = 0;
    int year = -1;
    int mon = -1;
    int mday = -1;
    int hour = -1;
    int min = -1;
    int sec = -1;
    int c = -1;
    int n = -1;
    int tzoffset = -1;
    int prevc = 0;
    JSBool seenplusminus = JS_FALSE;
    int temp;
    JSBool seenmonthname = JS_FALSE;

    if (limit == 0)
        goto syntax;
    while (i < limit) {
        c = s[i];
        i++;
        if (c <= ' ' || c == ',' || c == '-') {
            if (c == '-' && '0' <= s[i] && s[i] <= '9') {
              prevc = c;
            }
            continue;
        }
        if (c == '(') { 
            int depth = 1;
            while (i < limit) {
                c = s[i];
                i++;
                if (c == '(') depth++;
                else if (c == ')')
                    if (--depth <= 0)
                        break;
            }
            continue;
        }
        if ('0' <= c && c <= '9') {
            n = c - '0';
            while (i < limit && '0' <= (c = s[i]) && c <= '9') {
                n = n * 10 + c - '0';
                i++;
            }

            



            



            if ((prevc == '+' || prevc == '-')) {
                
                seenplusminus = JS_TRUE;

                
                if (n < 24)
                    n = n * 60; 
                else
                    n = n % 100 + n / 100 * 60; 
                if (prevc == '+')       
                    n = -n;
                if (tzoffset != 0 && tzoffset != -1)
                    goto syntax;
                tzoffset = n;
            } else if (prevc == '/' && mon >= 0 && mday >= 0 && year < 0) {
                if (c <= ' ' || c == ',' || c == '/' || i >= limit)
                    year = n;
                else
                    goto syntax;
            } else if (c == ':') {
                if (hour < 0)
                    hour =  n;
                else if (min < 0)
                    min =  n;
                else
                    goto syntax;
            } else if (c == '/') {
                

                if (mon < 0)
                    mon =  n;
                else if (mday < 0)
                    mday =  n;
                else
                    goto syntax;
            } else if (i < limit && c != ',' && c > ' ' && c != '-' && c != '(') {
                goto syntax;
            } else if (seenplusminus && n < 60) {  
                if (tzoffset < 0)
                    tzoffset -= n;
                else
                    tzoffset += n;
            } else if (hour >= 0 && min < 0) {
                min =  n;
            } else if (prevc == ':' && min >= 0 && sec < 0) {
                sec =  n;
            } else if (mon < 0) {
                mon = n;
            } else if (mon >= 0 && mday < 0) {
                mday =  n;
            } else if (mon >= 0 && mday >= 0 && year < 0) {
                year = n;
            } else {
                goto syntax;
            }
            prevc = 0;
        } else if (c == '/' || c == ':' || c == '+' || c == '-') {
            prevc = c;
        } else {
            size_t st = i - 1;
            int k;
            while (i < limit) {
                c = s[i];
                if (!(('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')))
                    break;
                i++;
            }
            if (i <= st + 1)
                goto syntax;
            for (k = (sizeof(wtb)/sizeof(char*)); --k >= 0;)
                if (date_regionMatches(wtb[k], 0, s, st, i-st, 1)) {
                    int action = ttb[k];
                    if (action != 0) {
                        if (action < 0) {
                            



                            JS_ASSERT(action == -1 || action == -2);
                            if (hour > 12 || hour < 0) {
                                goto syntax;
                            } else {
                                if (action == -1 && hour == 12) { 
                                    hour = 0;
                                } else if (action == -2 && hour != 12) { 
                                    hour += 12;
                                }
                            }
                        } else if (action <= 13) { 
                            

                            if (seenmonthname) {
                                goto syntax;
                            }
                            seenmonthname = JS_TRUE;
                            temp =  (action - 2) + 1;

                            if (mon < 0) {
                                mon = temp;
                            } else if (mday < 0) {
                                mday = mon;
                                mon = temp;
                            } else if (year < 0) {
                                year = mon;
                                mon = temp;
                            } else {
                                goto syntax;
                            }
                        } else {
                            tzoffset = action - 10000;
                        }
                    }
                    break;
                }
            if (k < 0)
                goto syntax;
            prevc = 0;
        }
    }
    if (year < 0 || mon < 0 || mday < 0)
        goto syntax;
    

























    if (seenmonthname) {
        if ((mday >= 70 && year >= 70) || (mday < 70 && year < 70)) {
            goto syntax;
        }
        if (mday > year) {
            temp = year;
            year = mday;
            mday = temp;
        }
        if (year >= 70 && year < 100) {
            year += 1900;
        }
    } else if (mon < 70) { 
        if (year < 100) {
            year += 1900;
        }
    } else if (mon < 100) { 
        if (mday < 70) {
            temp = year;
            year = mon + 1900;
            mon = mday;
            mday = temp;
        } else {
            goto syntax;
        }
    } else { 
        if (mday < 70) {
            temp = year;
            year = mon;
            mon = mday;
            mday = temp;
        } else {
            goto syntax;
        }
    }
    mon -= 1; 
    if (sec < 0)
        sec = 0;
    if (min < 0)
        min = 0;
    if (hour < 0)
        hour = 0;
    if (tzoffset == -1) { 
        jsdouble msec_time;
        msec_time = date_msecFromDate(year, mon, mday, hour, min, sec, 0);

        *result = UTC(msec_time);
        return JS_TRUE;
    }

    msec = date_msecFromDate(year, mon, mday, hour, min, sec, 0);
    msec += tzoffset * msPerMinute;
    *result = msec;
    return JS_TRUE;

syntax:
    
    *result = 0;
    return JS_FALSE;
}

static JSBool
date_parse(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;
    jsdouble result;

    str = js_ValueToString(cx, vp[2]);
    if (!str)
        return JS_FALSE;
    if (!date_parseString(str, &result)) {
        *vp = DOUBLE_TO_JSVAL(cx->runtime->jsNaN);
        return JS_TRUE;
    }

    result = TIMECLIP(result);
    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_now(JSContext *cx, uintN argc, jsval *vp)
{
    int64 us, ms, us2ms;
    jsdouble msec_time;

    us = PRMJ_Now();
    JSLL_UI2L(us2ms, PRMJ_USEC_PER_MSEC);
    JSLL_DIV(ms, us, us2ms);
    JSLL_L2D(msec_time, ms);

    return js_NewDoubleValue(cx, msec_time, vp);
}





static JSBool
GetUTCTime(JSContext *cx, JSObject *obj, jsval *vp, jsdouble *dp)
{
    jsval v;

    if (vp && !JS_InstanceOf(cx, obj, &js_DateClass, vp + 2))
        return JS_FALSE;
    if (!JS_GetReservedSlot(cx, obj, UTC_TIME_SLOT, &v))
        return JS_FALSE;

    *dp = *JSVAL_TO_DOUBLE(v);
    return JS_TRUE;
}







static JSBool
SetUTCTimePtr(JSContext *cx, JSObject *obj, jsval *vp, jsdouble *dp)
{
    if (vp && !JS_InstanceOf(cx, obj, &js_DateClass, vp + 2))
        return JS_FALSE;

    
    if (!JS_SetReservedSlot(cx, obj, LOCAL_TIME_SLOT,
                            DOUBLE_TO_JSVAL(cx->runtime->jsNaN))) {
        return JS_FALSE;
    }

    return JS_SetReservedSlot(cx, obj, UTC_TIME_SLOT, DOUBLE_TO_JSVAL(dp));
}




static JSBool
SetUTCTime(JSContext *cx, JSObject *obj, jsval *vp, jsdouble t)
{
    jsdouble *dp = js_NewDouble(cx, t, 0);
    if (!dp)
        return JS_FALSE;
    return SetUTCTimePtr(cx, obj, vp, dp);
}





static JSBool
GetLocalTime(JSContext *cx, JSObject *obj, jsval *vp, jsdouble *dp)
{
    jsval v;
    jsdouble result;
    jsdouble *cached;

    if (!JS_GetReservedSlot(cx, obj, LOCAL_TIME_SLOT, &v))
        return JS_FALSE;

    result = *JSVAL_TO_DOUBLE(v);

    if (JSDOUBLE_IS_NaN(result)) {
        if (!GetUTCTime(cx, obj, vp, &result))
            return JS_FALSE;

        
        if (JSDOUBLE_IS_FINITE(result))
            result = LocalTime(result);

        cached = js_NewDouble(cx, result, 0);
        if (!cached)
            return JS_FALSE;

        if (!JS_SetReservedSlot(cx, obj, LOCAL_TIME_SLOT,
                                DOUBLE_TO_JSVAL(cached))) {
            return JS_FALSE;
        }
    }

    *dp = result;
    return JS_TRUE;
}




static JSBool
date_getTime(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    return GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result) &&
           js_NewNumberValue(cx, result, vp);
}

static JSBool
GetYear(JSContext *cx, JSBool fullyear, jsval *vp)
{
    jsdouble result;

    if (!GetLocalTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result)) {
        result = YearFromTime(result);

        
        if (!fullyear)
            result -= 1900;
    }

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getYear(JSContext *cx, uintN argc, jsval *vp)
{
    return GetYear(cx, JS_FALSE, vp);
}

static JSBool
date_getFullYear(JSContext *cx, uintN argc, jsval *vp)
{
    return GetYear(cx, JS_TRUE, vp);
}

static JSBool
date_getUTCFullYear(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = YearFromTime(result);

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getMonth(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetLocalTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = MonthFromTime(result);

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getUTCMonth(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = MonthFromTime(result);

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getDate(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetLocalTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = DateFromTime(result);

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getUTCDate(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = DateFromTime(result);

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getDay(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetLocalTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = WeekDay(result);

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getUTCDay(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = WeekDay(result);

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getHours(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetLocalTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = HourFromTime(result);

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getUTCHours(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = HourFromTime(result);

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getMinutes(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetLocalTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = MinFromTime(result);

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getUTCMinutes(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = MinFromTime(result);

    return js_NewNumberValue(cx, result, vp);
}



static JSBool
date_getUTCSeconds(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = SecFromTime(result);

    return js_NewNumberValue(cx, result, vp);
}



static JSBool
date_getUTCMilliseconds(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &result))
        return JS_FALSE;

    if (JSDOUBLE_IS_FINITE(result))
        result = msFromTime(result);

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_getTimezoneOffset(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsdouble utctime, localtime, result;

    obj = JSVAL_TO_OBJECT(vp[1]);
    if (!GetUTCTime(cx, obj, vp, &utctime))
        return JS_FALSE;
    if (!GetLocalTime(cx, obj, NULL, &localtime))
        return JS_FALSE;

    




    result = (utctime - localtime) / msPerMinute;
    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_setTime(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble result;

    if (!js_ValueToNumber(cx, vp[2], &result))
        return JS_FALSE;

    result = TIMECLIP(result);

    if (!SetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, result))
        return JS_FALSE;

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_makeTime(JSContext *cx, uintN maxargs, JSBool local, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsval *argv;
    uintN i;
    jsdouble args[4], *argp, *stop;
    jsdouble hour, min, sec, msec;
    jsdouble lorutime; 

    jsdouble msec_time;
    jsdouble result;

    obj = JSVAL_TO_OBJECT(vp[1]);
    if (!GetUTCTime(cx, obj, vp, &result))
        return JS_FALSE;

    
    if (!JSDOUBLE_IS_FINITE(result))
        return js_NewNumberValue(cx, result, vp);

    








    if (argc == 0)
        argc = 1;   
    else if (argc > maxargs)
        argc = maxargs;  
    JS_ASSERT(1 <= argc && argc <= 4);

    argv = vp + 2;
    for (i = 0; i < argc; i++) {
        if (!js_ValueToNumber(cx, argv[i], &args[i]))
            return JS_FALSE;
        if (!JSDOUBLE_IS_FINITE(args[i])) {
            if (!SetUTCTimePtr(cx, obj, NULL, cx->runtime->jsNaN))
                return JS_FALSE;
            *vp = DOUBLE_TO_JSVAL(cx->runtime->jsNaN);
            return JS_TRUE;
        }
        args[i] = js_DoubleToInteger(args[i]);
    }

    if (local)
        lorutime = LocalTime(result);
    else
        lorutime = result;

    argp = args;
    stop = argp + argc;
    if (maxargs >= 4 && argp < stop)
        hour = *argp++;
    else
        hour = HourFromTime(lorutime);

    if (maxargs >= 3 && argp < stop)
        min = *argp++;
    else
        min = MinFromTime(lorutime);

    if (maxargs >= 2 && argp < stop)
        sec = *argp++;
    else
        sec = SecFromTime(lorutime);

    if (maxargs >= 1 && argp < stop)
        msec = *argp;
    else
        msec = msFromTime(lorutime);

    msec_time = MakeTime(hour, min, sec, msec);
    result = MakeDate(Day(lorutime), msec_time);



    if (local)
        result = UTC(result);



    result = TIMECLIP(result);
    if (!SetUTCTime(cx, obj, NULL, result))
        return JS_FALSE;

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_setMilliseconds(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeTime(cx, 1, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCMilliseconds(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeTime(cx, 1, JS_FALSE, argc, vp);
}

static JSBool
date_setSeconds(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeTime(cx, 2, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCSeconds(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeTime(cx, 2, JS_FALSE, argc, vp);
}

static JSBool
date_setMinutes(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeTime(cx, 3, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCMinutes(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeTime(cx, 3, JS_FALSE, argc, vp);
}

static JSBool
date_setHours(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeTime(cx, 4, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCHours(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeTime(cx, 4, JS_FALSE, argc, vp);
}

static JSBool
date_makeDate(JSContext *cx, uintN maxargs, JSBool local, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsval *argv;
    uintN i;
    jsdouble lorutime; 
    jsdouble args[3], *argp, *stop;
    jsdouble year, month, day;
    jsdouble result;

    obj = JSVAL_TO_OBJECT(vp[1]);
    if (!GetUTCTime(cx, obj, vp, &result))
        return JS_FALSE;

    
    if (argc == 0)
        argc = 1;   
    else if (argc > maxargs)
        argc = maxargs;   
    JS_ASSERT(1 <= argc && argc <= 3);

    argv = vp + 2;
    for (i = 0; i < argc; i++) {
        if (!js_ValueToNumber(cx, argv[i], &args[i]))
            return JS_FALSE;
        if (!JSDOUBLE_IS_FINITE(args[i])) {
            if (!SetUTCTimePtr(cx, obj, NULL, cx->runtime->jsNaN))
                return JS_FALSE;
            *vp = DOUBLE_TO_JSVAL(cx->runtime->jsNaN);
            return JS_TRUE;
        }
        args[i] = js_DoubleToInteger(args[i]);
    }

    

    if (!(JSDOUBLE_IS_FINITE(result))) {
        if (maxargs < 3)
            return js_NewNumberValue(cx, result, vp);
        lorutime = +0.;
    } else {
        lorutime = local ? LocalTime(result) : result;
    }

    argp = args;
    stop = argp + argc;
    if (maxargs >= 3 && argp < stop)
        year = *argp++;
    else
        year = YearFromTime(lorutime);

    if (maxargs >= 2 && argp < stop)
        month = *argp++;
    else
        month = MonthFromTime(lorutime);

    if (maxargs >= 1 && argp < stop)
        day = *argp++;
    else
        day = DateFromTime(lorutime);

    day = MakeDay(year, month, day); 
    result = MakeDate(day, TimeWithinDay(lorutime));

    if (local)
        result = UTC(result);

    result = TIMECLIP(result);
    if (!SetUTCTime(cx, obj, NULL, result))
        return JS_FALSE;

    return js_NewNumberValue(cx, result, vp);
}

static JSBool
date_setDate(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeDate(cx, 1, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCDate(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeDate(cx, 1, JS_FALSE, argc, vp);
}

static JSBool
date_setMonth(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeDate(cx, 2, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCMonth(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeDate(cx, 2, JS_FALSE, argc, vp);
}

static JSBool
date_setFullYear(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeDate(cx, 3, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCFullYear(JSContext *cx, uintN argc, jsval *vp)
{
    return date_makeDate(cx, 3, JS_FALSE, argc, vp);
}

static JSBool
date_setYear(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsdouble t;
    jsdouble year;
    jsdouble day;
    jsdouble result;

    obj = JSVAL_TO_OBJECT(vp[1]);
    if (!GetUTCTime(cx, obj, vp, &result))
        return JS_FALSE;

    if (!js_ValueToNumber(cx, vp[2], &year))
        return JS_FALSE;
    if (!JSDOUBLE_IS_FINITE(year)) {
        if (!SetUTCTimePtr(cx, obj, NULL, cx->runtime->jsNaN))
            return JS_FALSE;
        return js_NewNumberValue(cx, *cx->runtime->jsNaN, vp);
    }

    year = js_DoubleToInteger(year);

    if (!JSDOUBLE_IS_FINITE(result)) {
        t = +0.0;
    } else {
        t = LocalTime(result);
    }

    if (year >= 0 && year <= 99)
        year += 1900;

    day = MakeDay(year, MonthFromTime(t), DateFromTime(t));
    result = MakeDate(day, TimeWithinDay(t));
    result = UTC(result);

    result = TIMECLIP(result);
    if (!SetUTCTime(cx, obj, NULL, result))
        return JS_FALSE;

    return js_NewNumberValue(cx, result, vp);
}


static char js_NaN_date_str[] = "Invalid Date";
static const char* days[] =
{
   "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};
static const char* months[] =
{
   "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

static JSBool
date_toGMTString(JSContext *cx, uintN argc, jsval *vp)
{
    char buf[100];
    JSString *str;
    jsdouble utctime;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &utctime))
        return JS_FALSE;

    if (!JSDOUBLE_IS_FINITE(utctime)) {
        JS_snprintf(buf, sizeof buf, js_NaN_date_str);
    } else {
        


        JS_snprintf(buf, sizeof buf, "%s, %.2d %s %.4d %.2d:%.2d:%.2d GMT",
                    days[WeekDay(utctime)],
                    DateFromTime(utctime),
                    months[MonthFromTime(utctime)],
                    YearFromTime(utctime),
                    HourFromTime(utctime),
                    MinFromTime(utctime),
                    SecFromTime(utctime));
    }
    str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}





static void
new_explode(jsdouble timeval, PRMJTime *split, JSBool findEquivalent)
{
    jsint year = YearFromTime(timeval);
    int16 adjustedYear;

    
    if (year > 32767 || year < -32768) {
        if (findEquivalent) {
            



            jsint cycles;
#define CYCLE_YEARS 2800L
            cycles = (year >= 0) ? year / CYCLE_YEARS
                                 : -1 - (-1 - year) / CYCLE_YEARS;
            adjustedYear = (int16)(year - cycles * CYCLE_YEARS);
        } else {
            
            adjustedYear = (int16)((year > 0) ? 32767 : - 32768);
        }
    } else {
        adjustedYear = (int16)year;
    }

    split->tm_usec = (int32) msFromTime(timeval) * 1000;
    split->tm_sec = (int8) SecFromTime(timeval);
    split->tm_min = (int8) MinFromTime(timeval);
    split->tm_hour = (int8) HourFromTime(timeval);
    split->tm_mday = (int8) DateFromTime(timeval);
    split->tm_mon = (int8) MonthFromTime(timeval);
    split->tm_wday = (int8) WeekDay(timeval);
    split->tm_year = (int16) adjustedYear;
    split->tm_yday = (int16) DayWithinYear(timeval, year);

    

    split->tm_isdst = (DaylightSavingTA(timeval) != 0);
}

typedef enum formatspec {
    FORMATSPEC_FULL, FORMATSPEC_DATE, FORMATSPEC_TIME
} formatspec;


static JSBool
date_format(JSContext *cx, jsdouble date, formatspec format, jsval *rval)
{
    char buf[100];
    JSString *str;
    char tzbuf[100];
    JSBool usetz;
    size_t i, tzlen;
    PRMJTime split;

    if (!JSDOUBLE_IS_FINITE(date)) {
        JS_snprintf(buf, sizeof buf, js_NaN_date_str);
    } else {
        jsdouble local = LocalTime(date);

        

        jsint minutes = (jsint) floor(AdjustTime(date) / msPerMinute);

        
        intN offset = (minutes / 60) * 100 + minutes % 60;

        








        

        new_explode(date, &split, JS_TRUE);
        if (PRMJ_FormatTime(tzbuf, sizeof tzbuf, "(%Z)", &split) != 0) {

            





            usetz = JS_TRUE;
            tzlen = strlen(tzbuf);
            if (tzlen > 100) {
                usetz = JS_FALSE;
            } else {
                for (i = 0; i < tzlen; i++) {
                    jschar c = tzbuf[i];
                    if (c > 127 ||
                        !(isalpha(c) || isdigit(c) ||
                          c == ' ' || c == '(' || c == ')')) {
                        usetz = JS_FALSE;
                    }
                }
            }

            
            if (tzbuf[0] != '(' || tzbuf[1] == ')')
                usetz = JS_FALSE;
        } else
            usetz = JS_FALSE;

        switch (format) {
          case FORMATSPEC_FULL:
            



            
            JS_snprintf(buf, sizeof buf,
                        "%s %s %.2d %.4d %.2d:%.2d:%.2d GMT%+.4d%s%s",
                        days[WeekDay(local)],
                        months[MonthFromTime(local)],
                        DateFromTime(local),
                        YearFromTime(local),
                        HourFromTime(local),
                        MinFromTime(local),
                        SecFromTime(local),
                        offset,
                        usetz ? " " : "",
                        usetz ? tzbuf : "");
            break;
          case FORMATSPEC_DATE:
            
            JS_snprintf(buf, sizeof buf,
                        "%s %s %.2d %.4d",
                        days[WeekDay(local)],
                        months[MonthFromTime(local)],
                        DateFromTime(local),
                        YearFromTime(local));
            break;
          case FORMATSPEC_TIME:
            
            JS_snprintf(buf, sizeof buf,
                        "%.2d:%.2d:%.2d GMT%+.4d%s%s",
                        HourFromTime(local),
                        MinFromTime(local),
                        SecFromTime(local),
                        offset,
                        usetz ? " " : "",
                        usetz ? tzbuf : "");
            break;
        }
    }

    str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return JS_FALSE;
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
date_toLocaleHelper(JSContext *cx, const char *format, jsval *vp)
{
    JSObject *obj;
    char buf[100];
    JSString *str;
    PRMJTime split;
    jsdouble utctime;

    obj = JSVAL_TO_OBJECT(vp[1]);
    if (!GetUTCTime(cx, obj, vp, &utctime))
        return JS_FALSE;

    if (!JSDOUBLE_IS_FINITE(utctime)) {
        JS_snprintf(buf, sizeof buf, js_NaN_date_str);
    } else {
        intN result_len;
        jsdouble local = LocalTime(utctime);
        new_explode(local, &split, JS_FALSE);

        
        result_len = PRMJ_FormatTime(buf, sizeof buf, format, &split);

        
        if (result_len == 0)
            return date_format(cx, utctime, FORMATSPEC_FULL, vp);

        
        if (strcmp(format, "%x") == 0 && result_len >= 6 &&
            

            !isdigit(buf[result_len - 3]) &&
            isdigit(buf[result_len - 2]) && isdigit(buf[result_len - 1]) &&
            
            !(isdigit(buf[0]) && isdigit(buf[1]) &&
              isdigit(buf[2]) && isdigit(buf[3]))) {
            JS_snprintf(buf + (result_len - 2), (sizeof buf) - (result_len - 2),
                        "%d", js_DateGetYear(cx, obj));
        }

    }

    if (cx->localeCallbacks && cx->localeCallbacks->localeToUnicode)
        return cx->localeCallbacks->localeToUnicode(cx, buf, vp);

    str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
date_toLocaleString(JSContext *cx, uintN argc, jsval *vp)
{
    



    return date_toLocaleHelper(cx,
#if defined(_WIN32) && !defined(__MWERKS__)
                                   "%#c"
#else
                                   "%c"
#endif
                               , vp);
}

static JSBool
date_toLocaleDateString(JSContext *cx, uintN argc, jsval *vp)
{
    



    return date_toLocaleHelper(cx,
#if defined(_WIN32) && !defined(__MWERKS__)
                                   "%#x"
#else
                                   "%x"
#endif
                               , vp);
}

static JSBool
date_toLocaleTimeString(JSContext *cx, uintN argc, jsval *vp)
{
    return date_toLocaleHelper(cx, "%X", vp);
}

static JSBool
date_toLocaleFormat(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *fmt;
    const char *fmtbytes;

    if (argc == 0)
        return date_toLocaleString(cx, argc, vp);

    fmt = js_ValueToString(cx, vp[2]);
    if (!fmt)
        return JS_FALSE;
    vp[2] = STRING_TO_JSVAL(fmt);
    fmtbytes = js_GetStringBytes(cx, fmt);
    if (!fmtbytes)
        return JS_FALSE;

    return date_toLocaleHelper(cx, fmtbytes, vp);
}

static JSBool
date_toTimeString(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble utctime;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &utctime))
        return JS_FALSE;
    return date_format(cx, utctime, FORMATSPEC_TIME, vp);
}

static JSBool
date_toDateString(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble utctime;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &utctime))
        return JS_FALSE;
    return date_format(cx, utctime, FORMATSPEC_DATE, vp);
}

#if JS_HAS_TOSOURCE
#include <string.h>
#include "jsdtoa.h"

static JSBool
date_toSource(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble utctime;
    char buf[DTOSTR_STANDARD_BUFFER_SIZE], *numStr, *bytes;
    JSString *str;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &utctime))
        return JS_FALSE;

    numStr = JS_dtostr(buf, sizeof buf, DTOSTR_STANDARD, 0, utctime);
    if (!numStr) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    bytes = JS_smprintf("(new %s(%s))", js_Date_str, numStr);
    if (!bytes) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    str = JS_NewString(cx, bytes, strlen(bytes));
    if (!str) {
        free(bytes);
        return JS_FALSE;
    }
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}
#endif

static JSBool
date_toString(JSContext *cx, uintN argc, jsval *vp)
{
    jsdouble utctime;

    if (!GetUTCTime(cx, JSVAL_TO_OBJECT(vp[1]), vp, &utctime))
        return JS_FALSE;
    return date_format(cx, utctime, FORMATSPEC_FULL, vp);
}

static JSBool
date_valueOf(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str, *str2;

    




    
    if (argc == 0)
        return date_getTime(cx, argc, vp);

    
    str = js_ValueToString(cx, vp[2]);
    if (!str)
        return JS_FALSE;
    str2 = ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[JSTYPE_NUMBER]);
    if (js_EqualStrings(str, str2))
        return date_getTime(cx, argc, vp);
    return date_toString(cx, argc, vp);
}






static JSFunctionSpec date_static_methods[] = {
    JS_FN("UTC",                 date_UTC,                2,MAXARGS,0,0),
    JS_FN("parse",               date_parse,              1,1,0,0),
    JS_FN("now",                 date_now,                0,0,0,0),
    JS_FS_END
};

static JSFunctionSpec date_methods[] = {
    JS_FN("getTime",             date_getTime,            0,0,0,0),
    JS_FN("getTimezoneOffset",   date_getTimezoneOffset,  0,0,0,0),
    JS_FN("getYear",             date_getYear,            0,0,0,0),
    JS_FN("getFullYear",         date_getFullYear,        0,0,0,0),
    JS_FN("getUTCFullYear",      date_getUTCFullYear,     0,0,0,0),
    JS_FN("getMonth",            date_getMonth,           0,0,0,0),
    JS_FN("getUTCMonth",         date_getUTCMonth,        0,0,0,0),
    JS_FN("getDate",             date_getDate,            0,0,0,0),
    JS_FN("getUTCDate",          date_getUTCDate,         0,0,0,0),
    JS_FN("getDay",              date_getDay,             0,0,0,0),
    JS_FN("getUTCDay",           date_getUTCDay,          0,0,0,0),
    JS_FN("getHours",            date_getHours,           0,0,0,0),
    JS_FN("getUTCHours",         date_getUTCHours,        0,0,0,0),
    JS_FN("getMinutes",          date_getMinutes,         0,0,0,0),
    JS_FN("getUTCMinutes",       date_getUTCMinutes,      0,0,0,0),
    JS_FN("getSeconds",          date_getUTCSeconds,      0,0,0,0),
    JS_FN("getUTCSeconds",       date_getUTCSeconds,      0,0,0,0),
    JS_FN("getMilliseconds",     date_getUTCMilliseconds, 0,0,0,0),
    JS_FN("getUTCMilliseconds",  date_getUTCMilliseconds, 0,0,0,0),
    JS_FN("setTime",             date_setTime,            1,1,0,0),
    JS_FN("setYear",             date_setYear,            1,1,0,0),
    JS_FN("setFullYear",         date_setFullYear,        1,3,0,0),
    JS_FN("setUTCFullYear",      date_setUTCFullYear,     1,3,0,0),
    JS_FN("setMonth",            date_setMonth,           1,2,0,0),
    JS_FN("setUTCMonth",         date_setUTCMonth,        1,2,0,0),
    JS_FN("setDate",             date_setDate,            1,1,0,0),
    JS_FN("setUTCDate",          date_setUTCDate,         1,1,0,0),
    JS_FN("setHours",            date_setHours,           1,4,0,0),
    JS_FN("setUTCHours",         date_setUTCHours,        1,4,0,0),
    JS_FN("setMinutes",          date_setMinutes,         1,3,0,0),
    JS_FN("setUTCMinutes",       date_setUTCMinutes,      1,3,0,0),
    JS_FN("setSeconds",          date_setSeconds,         1,2,0,0),
    JS_FN("setUTCSeconds",       date_setUTCSeconds,      1,2,0,0),
    JS_FN("setMilliseconds",     date_setMilliseconds,    1,1,0,0),
    JS_FN("setUTCMilliseconds",  date_setUTCMilliseconds, 1,1,0,0),
    JS_FN("toUTCString",         date_toGMTString,        0,0,0,0),
    JS_FN(js_toLocaleString_str, date_toLocaleString,     0,0,0,0),
    JS_FN("toLocaleDateString",  date_toLocaleDateString, 0,0,0,0),
    JS_FN("toLocaleTimeString",  date_toLocaleTimeString, 0,0,0,0),
    JS_FN("toLocaleFormat",      date_toLocaleFormat,     0,0,0,0),
    JS_FN("toDateString",        date_toDateString,       0,0,0,0),
    JS_FN("toTimeString",        date_toTimeString,       0,0,0,0),
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,       date_toSource,           0,0,0,0),
#endif
    JS_FN(js_toString_str,       date_toString,           0,0,0,0),
    JS_FN(js_valueOf_str,        date_valueOf,            0,0,0,0),
    JS_FS_END
};

static jsdouble *
date_constructor(JSContext *cx, JSObject* obj)
{
    jsdouble *date;

    date = js_NewDouble(cx, 0.0, 0);
    if (!date)
        return NULL;

    JS_SetReservedSlot(cx, obj, UTC_TIME_SLOT,
                       DOUBLE_TO_JSVAL(date));
    JS_SetReservedSlot(cx, obj, LOCAL_TIME_SLOT,
                       DOUBLE_TO_JSVAL(cx->runtime->jsNaN));
    return date;
}

static JSBool
Date(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsdouble *date;
    JSString *str;
    jsdouble d;

    
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) {
        int64 us, ms, us2ms;
        jsdouble msec_time;

        


        us = PRMJ_Now();
        JSLL_UI2L(us2ms, PRMJ_USEC_PER_MSEC);
        JSLL_DIV(ms, us, us2ms);
        JSLL_L2D(msec_time, ms);

        return date_format(cx, msec_time, FORMATSPEC_FULL, rval);
    }

    
    if (argc == 0) {
        int64 us, ms, us2ms;
        jsdouble msec_time;

        date = date_constructor(cx, obj);
        if (!date)
            return JS_FALSE;

        us = PRMJ_Now();
        JSLL_UI2L(us2ms, PRMJ_USEC_PER_MSEC);
        JSLL_DIV(ms, us, us2ms);
        JSLL_L2D(msec_time, ms);

        *date = msec_time;
    } else if (argc == 1) {
        if (!JSVAL_IS_STRING(argv[0])) {
            
            if (!js_ValueToNumber(cx, argv[0], &d))
                return JS_FALSE;
            date = date_constructor(cx, obj);
            if (!date)
                return JS_FALSE;
            *date = TIMECLIP(d);
        } else {
            
            date = date_constructor(cx, obj);
            if (!date)
                return JS_FALSE;

            str = js_ValueToString(cx, argv[0]);
            if (!str)
                return JS_FALSE;

            if (!date_parseString(str, date))
                *date = *cx->runtime->jsNaN;
            *date = TIMECLIP(*date);
        }
    } else {
        jsdouble array[MAXARGS];
        uintN loop;
        jsdouble double_arg;
        jsdouble day;
        jsdouble msec_time;

        for (loop = 0; loop < MAXARGS; loop++) {
            if (loop < argc) {
                if (!js_ValueToNumber(cx, argv[loop], &double_arg))
                    return JS_FALSE;
                

                if (!JSDOUBLE_IS_FINITE(double_arg)) {
                    date = date_constructor(cx, obj);
                    if (!date)
                        return JS_FALSE;
                    *date = *cx->runtime->jsNaN;
                    return JS_TRUE;
                }
                array[loop] = js_DoubleToInteger(double_arg);
            } else {
                if (loop == 2) {
                    array[loop] = 1; 
                } else {
                    array[loop] = 0;
                }
            }
        }

        date = date_constructor(cx, obj);
        if (!date)
            return JS_FALSE;

        
        if (array[0] >= 0 && array[0] <= 99)
            array[0] += 1900;

        day = MakeDay(array[0], array[1], array[2]);
        msec_time = MakeTime(array[3], array[4], array[5], array[6]);
        msec_time = MakeDate(day, msec_time);
        msec_time = UTC(msec_time);
        *date = TIMECLIP(msec_time);
    }
    return JS_TRUE;
}

JSObject *
js_InitDateClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto;
    jsdouble *proto_date;

    
    LocalTZA = -(PRMJ_LocalGMTDifference() * msPerSecond);
    proto = JS_InitClass(cx, obj, NULL, &js_DateClass, Date, MAXARGS,
                         NULL, date_methods, NULL, date_static_methods);
    if (!proto)
        return NULL;

    
    if (!JS_AliasProperty(cx, proto, "toUTCString", "toGMTString"))
        return NULL;

    
    proto_date = date_constructor(cx, proto);
    if (!proto_date)
        return NULL;
    *proto_date = *cx->runtime->jsNaN;

    return proto;
}

JS_FRIEND_API(JSObject *)
js_NewDateObjectMsec(JSContext *cx, jsdouble msec_time)
{
    JSObject *obj;
    jsdouble *date;

    obj = js_NewObject(cx, &js_DateClass, NULL, NULL);
    if (!obj)
        return NULL;

    date = date_constructor(cx, obj);
    if (!date)
        return NULL;

    *date = msec_time;
    return obj;
}

JS_FRIEND_API(JSObject *)
js_NewDateObject(JSContext* cx, int year, int mon, int mday,
                 int hour, int min, int sec)
{
    JSObject *obj;
    jsdouble msec_time;

    msec_time = date_msecFromDate(year, mon, mday, hour, min, sec, 0);
    obj = js_NewDateObjectMsec(cx, UTC(msec_time));
    return obj;
}

JS_FRIEND_API(JSBool)
js_DateIsValid(JSContext *cx, JSObject* obj)
{
    jsdouble utctime;
    return GetUTCTime(cx, obj, NULL, &utctime) && !JSDOUBLE_IS_NaN(utctime);
}

JS_FRIEND_API(int)
js_DateGetYear(JSContext *cx, JSObject* obj)
{
    jsdouble localtime;

    
    if (!GetLocalTime(cx, obj, NULL, &localtime) || JSDOUBLE_IS_NaN(localtime))
        return 0;

    return (int) YearFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetMonth(JSContext *cx, JSObject* obj)
{
    jsdouble localtime;

    if (!GetLocalTime(cx, obj, NULL, &localtime) || JSDOUBLE_IS_NaN(localtime))
        return 0;

    return (int) MonthFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetDate(JSContext *cx, JSObject* obj)
{
    jsdouble localtime;

    if (!GetLocalTime(cx, obj, NULL, &localtime) || JSDOUBLE_IS_NaN(localtime))
        return 0;

    return (int) DateFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetHours(JSContext *cx, JSObject* obj)
{
    jsdouble localtime;

    if (!GetLocalTime(cx, obj, NULL, &localtime) || JSDOUBLE_IS_NaN(localtime))
        return 0;

    return (int) HourFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetMinutes(JSContext *cx, JSObject* obj)
{
    jsdouble localtime;

    if (!GetLocalTime(cx, obj, NULL, &localtime) || JSDOUBLE_IS_NaN(localtime))
        return 0;

    return (int) MinFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetSeconds(JSContext *cx, JSObject* obj)
{
    jsdouble utctime;

    if (!GetUTCTime(cx, obj, NULL, &utctime) || JSDOUBLE_IS_NaN(utctime))
        return 0;

    return (int) SecFromTime(utctime);
}

JS_FRIEND_API(void)
js_DateSetYear(JSContext *cx, JSObject *obj, int year)
{
    jsdouble local;

    if (!GetLocalTime(cx, obj, NULL, &local))
        return;

    
    if (JSDOUBLE_IS_NaN(local))
        local = 0;

    local = date_msecFromDate(year,
                              MonthFromTime(local),
                              DateFromTime(local),
                              HourFromTime(local),
                              MinFromTime(local),
                              SecFromTime(local),
                              msFromTime(local));

    
    SetUTCTime(cx, obj, NULL, UTC(local));
}

JS_FRIEND_API(void)
js_DateSetMonth(JSContext *cx, JSObject *obj, int month)
{
    jsdouble local;

    if (!GetLocalTime(cx, obj, NULL, &local))
        return;

    
    if (JSDOUBLE_IS_NaN(local))
        return;

    local = date_msecFromDate(YearFromTime(local),
                              month,
                              DateFromTime(local),
                              HourFromTime(local),
                              MinFromTime(local),
                              SecFromTime(local),
                              msFromTime(local));
    SetUTCTime(cx, obj, NULL, UTC(local));
}

JS_FRIEND_API(void)
js_DateSetDate(JSContext *cx, JSObject *obj, int date)
{
    jsdouble local;

    if (!GetLocalTime(cx, obj, NULL, &local))
        return;

    if (JSDOUBLE_IS_NaN(local))
        return;

    local = date_msecFromDate(YearFromTime(local),
                              MonthFromTime(local),
                              date,
                              HourFromTime(local),
                              MinFromTime(local),
                              SecFromTime(local),
                              msFromTime(local));
    SetUTCTime(cx, obj, NULL, UTC(local));
}

JS_FRIEND_API(void)
js_DateSetHours(JSContext *cx, JSObject *obj, int hours)
{
    jsdouble local;

    if (!GetLocalTime(cx, obj, NULL, &local))
        return;

    if (JSDOUBLE_IS_NaN(local))
        return;
    local = date_msecFromDate(YearFromTime(local),
                              MonthFromTime(local),
                              DateFromTime(local),
                              hours,
                              MinFromTime(local),
                              SecFromTime(local),
                              msFromTime(local));
    SetUTCTime(cx, obj, NULL, UTC(local));
}

JS_FRIEND_API(void)
js_DateSetMinutes(JSContext *cx, JSObject *obj, int minutes)
{
    jsdouble local;

    if (!GetLocalTime(cx, obj, NULL, &local))
        return;

    if (JSDOUBLE_IS_NaN(local))
        return;
    local = date_msecFromDate(YearFromTime(local),
                              MonthFromTime(local),
                              DateFromTime(local),
                              HourFromTime(local),
                              minutes,
                              SecFromTime(local),
                              msFromTime(local));
    SetUTCTime(cx, obj, NULL, UTC(local));
}

JS_FRIEND_API(void)
js_DateSetSeconds(JSContext *cx, JSObject *obj, int seconds)
{
    jsdouble local;

    if (!GetLocalTime(cx, obj, NULL, &local))
        return;

    if (JSDOUBLE_IS_NaN(local))
        return;
    local = date_msecFromDate(YearFromTime(local),
                              MonthFromTime(local),
                              DateFromTime(local),
                              HourFromTime(local),
                              MinFromTime(local),
                              seconds,
                              msFromTime(local));
    SetUTCTime(cx, obj, NULL, UTC(local));
}

JS_FRIEND_API(jsdouble)
js_DateGetMsecSinceEpoch(JSContext *cx, JSObject *obj)
{
    jsdouble utctime;
    if (!GetUTCTime(cx, obj, NULL, &utctime))
        return 0;
    return utctime;
}
