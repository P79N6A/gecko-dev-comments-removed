




















































#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "mozilla/Util.h"

#include "jstypes.h"
#include "jsprf.h"
#include "prmjtime.h"
#include "jsutil.h"
#include "jsapi.h"
#include "jsversion.h"
#include "jscntxt.h"
#include "jsdate.h"
#include "jsinterp.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstr.h"
#include "jslibmath.h"

#include "vm/GlobalObject.h"
#include "vm/StringBuffer.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"

#include "vm/MethodGuard-inl.h"
#include "vm/Stack-inl.h"

using namespace mozilla;
using namespace js;
using namespace js::types;














































































#define HoursPerDay     24.0
#define MinutesPerDay   (HoursPerDay * MinutesPerHour)
#define MinutesPerHour  60.0
#define SecondsPerDay   (MinutesPerDay * SecondsPerMinute)
#define SecondsPerHour  (MinutesPerHour * SecondsPerMinute)
#define SecondsPerMinute 60.0

#if defined(XP_WIN) || defined(XP_OS2)





static double msPerSecond = 1000.0;
static double msPerDay = SecondsPerDay * 1000.0;
static double msPerHour = SecondsPerHour * 1000.0;
static double msPerMinute = SecondsPerMinute * 1000.0;
#else
#define msPerDay        (SecondsPerDay * msPerSecond)
#define msPerHour       (SecondsPerHour * msPerSecond)
#define msPerMinute     (SecondsPerMinute * msPerSecond)
#define msPerSecond     1000.0
#endif

#define Day(t)          floor((t) / msPerDay)

static double
TimeWithinDay(double t)
{
    double result;
    result = fmod(t, msPerDay);
    if (result < 0)
        result += msPerDay;
    return result;
}

static inline bool
IsLeapYear(int year)
{
    return year % 4 == 0 && (year % 100 || (year % 400 == 0));
}

static inline int
DaysInYear(int year)
{
    return IsLeapYear(year) ? 366 : 365;
}

static inline int
DaysInFebruary(int year)
{
    return IsLeapYear(year) ? 29 : 28;
}




#define DayFromYear(y)  (365 * ((y)-1970) + floor(((y)-1969)/4.0)            \
                         - floor(((y)-1901)/100.0) + floor(((y)-1601)/400.0))
#define TimeFromYear(y) (DayFromYear(y) * msPerDay)

static int
YearFromTime(double t)
{
    int y = (int) floor(t /(msPerDay*365.2425)) + 1970;
    double t2 = (double) TimeFromYear(y);

    




    if (t2 > t) {
        y--;
    } else {
        if (t2 + msPerDay * DaysInYear(y) <= t)
            y++;
    }
    return y;
}

#define DayWithinYear(t, year) ((int) (Day(t) - DayFromYear(year)))





static double firstDayOfMonth[2][13] = {
    {0.0, 31.0, 59.0, 90.0, 120.0, 151.0, 181.0, 212.0, 243.0, 273.0, 304.0, 334.0, 365.0},
    {0.0, 31.0, 60.0, 91.0, 121.0, 152.0, 182.0, 213.0, 244.0, 274.0, 305.0, 335.0, 366.0}
};

#define DayFromMonth(m, leap) firstDayOfMonth[leap][(int)m]

static int
DaysInMonth(int year, int month)
{
    JSBool leap = IsLeapYear(year);
    int result = int(DayFromMonth(month, leap) - DayFromMonth(month-1, leap));
    return result;
}

static int
MonthFromTime(double t)
{
    int d, step;
    int year = YearFromTime(t);
    d = DayWithinYear(t, year);

    if (d < (step = 31))
        return 0;
    if (d < (step += DaysInFebruary(year)))
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

static int
DateFromTime(double t)
{
    int d, step, next;
    int year = YearFromTime(t);
    d = DayWithinYear(t, year);

    if (d <= (next = 30))
        return d + 1;
    step = next;
    if (d <= (next += DaysInFebruary(year)))
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

static int
WeekDay(double t)
{
    int result;
    result = (int) Day(t) + 4;
    result = result % 7;
    if (result < 0)
        result += 7;
    return (int) result;
}

#define MakeTime(hour, min, sec, ms) \
((((hour) * MinutesPerHour + (min)) * SecondsPerMinute + (sec)) * msPerSecond + (ms))

static double
MakeDay(double year, double month, double date)
{
    JSBool leap;
    double yearday;
    double monthday;

    year += floor(month / 12);

    month = fmod(month, 12.0);
    if (month < 0)
        month += 12;

    leap = IsLeapYear((int) year);

    yearday = floor(TimeFromYear(year) / msPerDay);
    monthday = DayFromMonth(month, leap);

    return yearday + monthday + date - 1;
}

#define MakeDate(day, time) ((day) * msPerDay + (time))










static int yearStartingWith[2][7] = {
    {1978, 1973, 1974, 1975, 1981, 1971, 1977},
    {1984, 1996, 1980, 1992, 1976, 1988, 1972}
};








static int
EquivalentYearForDST(int year)
{
    int day;

    day = (int) DayFromYear(year) + 4;
    day = day % 7;
    if (day < 0)
        day += 7;

    return yearStartingWith[IsLeapYear(year)][day];
}


static double LocalTZA;

static double
DaylightSavingTA(double t, JSContext *cx)
{
    
    if (JSDOUBLE_IS_NaN(t))
        return t;

    



    if (t < 0.0 || t > 2145916800000.0) {
        int year = EquivalentYearForDST(YearFromTime(t));
        double day = MakeDay(year, MonthFromTime(t), DateFromTime(t));
        t = MakeDate(day, TimeWithinDay(t));
    }

    int64_t timeMilliseconds = static_cast<int64_t>(t);
    int64_t offsetMilliseconds = cx->dstOffsetCache.getDSTOffsetMilliseconds(timeMilliseconds, cx);
    return static_cast<double>(offsetMilliseconds);
}

static double
AdjustTime(double date, JSContext *cx)
{
    double t = DaylightSavingTA(date, cx) + LocalTZA;
    t = (LocalTZA >= 0) ? fmod(t, msPerDay) : -fmod(msPerDay - t, msPerDay);
    return t;
}

static double
LocalTime(double t, JSContext *cx)
{
    return t + AdjustTime(t, cx);
}

static double
UTC(double t, JSContext *cx)
{
    return t - AdjustTime(t - LocalTZA, cx);
}

static int
HourFromTime(double t)
{
    int result = (int) fmod(floor(t/msPerHour), HoursPerDay);
    if (result < 0)
        result += (int)HoursPerDay;
    return result;
}

static int
MinFromTime(double t)
{
    int result = (int) fmod(floor(t / msPerMinute), MinutesPerHour);
    if (result < 0)
        result += (int)MinutesPerHour;
    return result;
}

static int
SecFromTime(double t)
{
    int result = (int) fmod(floor(t / msPerSecond), SecondsPerMinute);
    if (result < 0)
        result += (int)SecondsPerMinute;
    return result;
}

static int
msFromTime(double t)
{
    int result = (int) fmod(t, msPerSecond);
    if (result < 0)
        result += (int)msPerSecond;
    return result;
}





static JSBool
date_convert(JSContext *cx, JSObject *obj, JSType hint, Value *vp)
{
    JS_ASSERT(hint == JSTYPE_NUMBER || hint == JSTYPE_STRING || hint == JSTYPE_VOID);
    JS_ASSERT(obj->isDate());

    return DefaultValue(cx, RootedVarObject(cx, obj),
                        (hint == JSTYPE_VOID) ? JSTYPE_STRING : hint, vp);
}





Class js::DateClass = {
    js_Date_str,
    JSCLASS_HAS_RESERVED_SLOTS(JSObject::DATE_CLASS_RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Date),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    date_convert
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
            if (unicode::ToLowerCase(s1[s1off]) != unicode::ToLowerCase(s2[s2off]))
                break;
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


static double
date_msecFromDate(double year, double mon, double mday, double hour,
                  double min, double sec, double msec)
{
    double day;
    double msec_time;
    double result;

    day = MakeDay(year, mon, mday);
    msec_time = MakeTime(hour, min, sec, msec);
    result = MakeDate(day, msec_time);
    return result;
}


#define MAXARGS        7

static JSBool
date_msecFromArgs(JSContext *cx, CallArgs args, double *rval)
{
    unsigned loop;
    double array[MAXARGS];
    double msec_time;

    for (loop = 0; loop < MAXARGS; loop++) {
        if (loop < args.length()) {
            double d;
            if (!ToNumber(cx, args[loop], &d))
                return JS_FALSE;
            
            if (!JSDOUBLE_IS_FINITE(d)) {
                *rval = js_NaN;
                return JS_TRUE;
            }
            array[loop] = js_DoubleToInteger(d);
        } else {
            if (loop == 2) {
                array[loop] = 1; 
            } else {
                array[loop] = 0;
            }
        }
    }

    
    if (array[0] >= 0 && array[0] <= 99)
        array[0] += 1900;

    msec_time = date_msecFromDate(array[0], array[1], array[2],
                                  array[3], array[4], array[5], array[6]);
    *rval = msec_time;
    return JS_TRUE;
}




static JSBool
date_UTC(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    double msec_time;
    if (!date_msecFromArgs(cx, args, &msec_time))
        return JS_FALSE;

    msec_time = TIMECLIP(msec_time);

    args.rval().setNumber(msec_time);
    return JS_TRUE;
}








static JSBool
digits(size_t *result, const jschar *s, size_t *i, size_t limit)
{
    size_t init = *i;
    *result = 0;
    while (*i < limit &&
           ('0' <= s[*i] && s[*i] <= '9')) {
        *result *= 10;
        *result += (s[*i] - '0');
        ++(*i);
    }
    return (*i != init);
}









static JSBool
fractional(double *result, const jschar *s, size_t *i, size_t limit)
{
    double factor = 0.1;
    size_t init = *i;
    *result = 0.0;
    while (*i < limit &&
           ('0' <= s[*i] && s[*i] <= '9')) {
        *result += (s[*i] - '0') * factor;
        factor *= 0.1;
        ++(*i);
    }
    return (*i != init);
}








static JSBool
ndigits(size_t n, size_t *result, const jschar *s, size_t* i, size_t limit)
{
    size_t init = *i;

    if (digits(result, s, i, JS_MIN(limit, init+n)))
        return ((*i - init) == n);

    *i = init;
    return JS_FALSE;
}

























































static JSBool
date_parseISOString(JSLinearString *str, double *result, JSContext *cx)
{
    double msec;

    const jschar *s;
    size_t limit;
    size_t i = 0;
    int tzMul = 1;
    int dateMul = 1;
    size_t year = 1970;
    size_t month = 1;
    size_t day = 1;
    size_t hour = 0;
    size_t min = 0;
    size_t sec = 0;
    double frac = 0;
    bool isLocalTime = JS_FALSE;
    size_t tzHour = 0;
    size_t tzMin = 0;

#define PEEK(ch) (i < limit && s[i] == ch)

#define NEED(ch)                                                     \
    JS_BEGIN_MACRO                                                   \
        if (i >= limit || s[i] != ch) { goto syntax; } else { ++i; } \
    JS_END_MACRO

#define DONE_DATE_UNLESS(ch)                                            \
    JS_BEGIN_MACRO                                                      \
        if (i >= limit || s[i] != ch) { goto done_date; } else { ++i; } \
    JS_END_MACRO

#define DONE_UNLESS(ch)                                            \
    JS_BEGIN_MACRO                                                 \
        if (i >= limit || s[i] != ch) { goto done; } else { ++i; } \
    JS_END_MACRO

#define NEED_NDIGITS(n, field)                                      \
    JS_BEGIN_MACRO                                                  \
        if (!ndigits(n, &field, s, &i, limit)) { goto syntax; }     \
    JS_END_MACRO

    s = str->chars();
    limit = str->length();

    if (PEEK('+') || PEEK('-')) {
        if (PEEK('-'))
            dateMul = -1;
        ++i;
        NEED_NDIGITS(6, year);
    } else if (!PEEK('T')) {
        NEED_NDIGITS(4, year);
    }
    DONE_DATE_UNLESS('-');
    NEED_NDIGITS(2, month);
    DONE_DATE_UNLESS('-');
    NEED_NDIGITS(2, day);

 done_date:
    DONE_UNLESS('T');
    NEED_NDIGITS(2, hour);
    NEED(':');
    NEED_NDIGITS(2, min);

    if (PEEK(':')) {
        ++i;
        NEED_NDIGITS(2, sec);
        if (PEEK('.')) {
            ++i;
            if (!fractional(&frac, s, &i, limit))
                goto syntax;
        }
    }

    if (PEEK('Z')) {
        ++i;
    } else if (PEEK('+') || PEEK('-')) {
        if (PEEK('-'))
            tzMul = -1;
        ++i;
        NEED_NDIGITS(2, tzHour);
        



        if (PEEK(':'))
          ++i;
        NEED_NDIGITS(2, tzMin);
    } else {
        isLocalTime = JS_TRUE;
    }

 done:
    if (year > 275943 
        || (month == 0 || month > 12)
        || (day == 0 || day > size_t(DaysInMonth(year,month)))
        || hour > 24
        || ((hour == 24) && (min > 0 || sec > 0))
        || min > 59
        || sec > 59
        || tzHour > 23
        || tzMin > 59)
        goto syntax;

    if (i != limit)
        goto syntax;

    month -= 1; 

    msec = date_msecFromDate(dateMul * (double)year, month, day,
                             hour, min, sec,
                             frac * 1000.0);;

    if (isLocalTime) {
        msec = UTC(msec, cx);
    } else {
        msec -= ((tzMul) * ((tzHour * msPerHour)
                            + (tzMin * msPerMinute)));
    }

    if (msec < -8.64e15 || msec > 8.64e15)
        goto syntax;

    *result = msec;

    return JS_TRUE;

 syntax:
    
    *result = 0;
    return JS_FALSE;

#undef PEEK
#undef NEED
#undef DONE_UNLESS
#undef NEED_NDIGITS
}

static JSBool
date_parseString(JSLinearString *str, double *result, JSContext *cx)
{
    double msec;

    const jschar *s;
    size_t limit;
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

    if (date_parseISOString(str, result, cx))
        return JS_TRUE;

    s = str->chars();
    limit = str->length();
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
            for (k = ArrayLength(wtb); --k >= 0;)
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

    msec = date_msecFromDate(year, mon, mday, hour, min, sec, 0);

    if (tzoffset == -1) { 
        msec = UTC(msec, cx);
    } else {
        msec += tzoffset * msPerMinute;
    }

    *result = msec;
    return JS_TRUE;

syntax:
    
    *result = 0;
    return JS_FALSE;
}

static JSBool
date_parse(JSContext *cx, unsigned argc, Value *vp)
{
    JSString *str;
    double result;

    if (argc == 0) {
        vp->setDouble(js_NaN);
        return true;
    }
    str = ToString(cx, vp[2]);
    if (!str)
        return JS_FALSE;
    vp[2].setString(str);
    JSLinearString *linearStr = str->ensureLinear(cx);
    if (!linearStr)
        return false;

    if (!date_parseString(linearStr, &result, cx)) {
        vp->setDouble(js_NaN);
        return true;
    }

    result = TIMECLIP(result);
    vp->setNumber(result);
    return true;
}

static inline double
NowAsMillis()
{
    return (double) (PRMJ_Now() / PRMJ_USEC_PER_MSEC);
}

static JSBool
date_now(JSContext *cx, unsigned argc, Value *vp)
{
    vp->setDouble(NowAsMillis());
    return JS_TRUE;
}




static JSBool
SetUTCTime(JSContext *cx, JSObject *obj, double t, Value *vp = NULL)
{
    JS_ASSERT(obj->isDate());

    for (size_t ind = JSObject::JSSLOT_DATE_COMPONENTS_START;
         ind < JSObject::DATE_CLASS_RESERVED_SLOTS;
         ind++) {
        obj->setSlot(ind, UndefinedValue());
    }

    obj->setDateUTCTime(DoubleValue(t));
    if (vp)
        vp->setDouble(t);
    return true;
}

static void
SetDateToNaN(JSContext *cx, JSObject *obj, Value *vp = NULL)
{
    double NaN = cx->runtime->NaNValue.getDoubleRef();
    SetUTCTime(cx, obj, NaN, vp);
}






static bool
FillLocalTimes(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isDate());

    double utcTime = obj->getDateUTCTime().toNumber();

    if (!JSDOUBLE_IS_FINITE(utcTime)) {
        for (size_t ind = JSObject::JSSLOT_DATE_COMPONENTS_START;
             ind < JSObject::DATE_CLASS_RESERVED_SLOTS;
             ind++) {
            obj->setSlot(ind, DoubleValue(utcTime));
        }
        return true;
    }

    double localTime = LocalTime(utcTime, cx);

    obj->setSlot(JSObject::JSSLOT_DATE_LOCAL_TIME, DoubleValue(localTime));

    int year = (int) floor(localTime /(msPerDay*365.2425)) + 1970;
    double yearStartTime = (double) TimeFromYear(year);

    
    int yearDays;
    if (yearStartTime > localTime) {
        year--;
        yearStartTime -= (msPerDay * DaysInYear(year));
        yearDays = DaysInYear(year);
    } else {
        yearDays = DaysInYear(year);
        double nextStart = yearStartTime + (msPerDay * yearDays);
        if (nextStart <= localTime) {
            year++;
            yearStartTime = nextStart;
            yearDays = DaysInYear(year);
        }
    }

    obj->setSlot(JSObject::JSSLOT_DATE_LOCAL_YEAR, Int32Value(year));

    uint64_t yearTime = uint64_t(localTime - yearStartTime);
    int yearSeconds = uint32_t(yearTime / 1000);

    int day = yearSeconds / int(SecondsPerDay);

    int step = -1, next = 30;
    int month;

    do {
        if (day <= next) {
            month = 0;
            break;
        }
        step = next;
        next += ((yearDays == 366) ? 29 : 28);
        if (day <= next) {
            month = 1;
            break;
        }
        step = next;
        if (day <= (next += 31)) {
            month = 2;
            break;
        }
        step = next;
        if (day <= (next += 30)) {
            month = 3;
            break;
        }
        step = next;
        if (day <= (next += 31)) {
            month = 4;
            break;
        }
        step = next;
        if (day <= (next += 30)) {
            month = 5;
            break;
        }
        step = next;
        if (day <= (next += 31)) {
            month = 6;
            break;
        }
        step = next;
        if (day <= (next += 31)) {
            month = 7;
            break;
        }
        step = next;
        if (day <= (next += 30)) {
            month = 8;
            break;
        }
        step = next;
        if (day <= (next += 31)) {
            month = 9;
            break;
        }
        step = next;
        if (day <= (next += 30)) {
            month = 10;
            break;
        }
        step = next;
        month = 11;
    } while (0);

    obj->setSlot(JSObject::JSSLOT_DATE_LOCAL_MONTH, Int32Value(month));
    obj->setSlot(JSObject::JSSLOT_DATE_LOCAL_DATE, Int32Value(day - step));

    int weekday = WeekDay(localTime);

    obj->setSlot(JSObject::JSSLOT_DATE_LOCAL_DAY, Int32Value(weekday));

    int seconds = yearSeconds % 60;

    obj->setSlot(JSObject::JSSLOT_DATE_LOCAL_SECONDS, Int32Value(seconds));

    int minutes = (yearSeconds / 60) % 60;

    obj->setSlot(JSObject::JSSLOT_DATE_LOCAL_MINUTES, Int32Value(minutes));

    int hours = (yearSeconds / (60 * 60)) % 24;

    obj->setSlot(JSObject::JSSLOT_DATE_LOCAL_HOURS, Int32Value(hours));

    return true;
}


static inline bool
GetAndCacheLocalTime(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isDate());

    
    if (obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_TIME).isUndefined()) {
        if (!FillLocalTimes(cx, obj))
            return false;
    }
    return true;
}

static inline bool
GetAndCacheLocalTime(JSContext *cx, JSObject *obj, double *time)
{
    if (!obj || !GetAndCacheLocalTime(cx, obj))
        return false;

    *time = obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_TIME).toDouble();
    return true;
}




static JSBool
date_getTime(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getTime, &DateClass, &ok);
    if (!obj)
        return ok;

    args.rval() = obj->getDateUTCTime();
    return true;
}

static JSBool
date_getYear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getYear, &DateClass, &ok);
    if (!obj)
        return ok;

    if (!GetAndCacheLocalTime(cx, obj))
        return false;

    Value yearVal = obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_YEAR);
    if (yearVal.isInt32()) {
        
        int year = yearVal.toInt32() - 1900;
        args.rval().setInt32(year);
    } else {
        args.rval() = yearVal;
    }

    return true;
}

static JSBool
date_getFullYear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getFullYear, &DateClass, &ok);
    if (!obj)
        return ok;

    if (!GetAndCacheLocalTime(cx, obj))
        return false;

    args.rval() = obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_YEAR);
    return true;
}

static JSBool
date_getUTCFullYear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getUTCFullYear, &DateClass, &ok);
    if (!obj)
        return ok;

    double result = obj->getDateUTCTime().toNumber();
    if (JSDOUBLE_IS_FINITE(result))
        result = YearFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static JSBool
date_getMonth(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getMonth, &DateClass, &ok);
    if (!obj)
        return ok;

    if (!GetAndCacheLocalTime(cx, obj))
        return false;

    args.rval() = obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_MONTH);
    return true;
}

static JSBool
date_getUTCMonth(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getUTCMonth, &DateClass, &ok);
    if (!obj)
        return ok;

    double result = obj->getDateUTCTime().toNumber();
    if (JSDOUBLE_IS_FINITE(result))
        result = MonthFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static JSBool
date_getDate(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getDate, &DateClass, &ok);
    if (!obj)
        return ok;

    if (!GetAndCacheLocalTime(cx, obj))
        return false;

    args.rval() = obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_DATE);
    return true;
}

static JSBool
date_getUTCDate(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getUTCDate, &DateClass, &ok);
    if (!obj)
        return ok;

    double result = obj->getDateUTCTime().toNumber();
    if (JSDOUBLE_IS_FINITE(result))
        result = DateFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static JSBool
date_getDay(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getDay, &DateClass, &ok);
    if (!obj)
        return ok;

    if (!GetAndCacheLocalTime(cx, obj))
        return false;

    args.rval() = obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_DAY);
    return true;
}

static JSBool
date_getUTCDay(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getUTCDay, &DateClass, &ok);
    if (!obj)
        return ok;

    double result = obj->getDateUTCTime().toNumber();
    if (JSDOUBLE_IS_FINITE(result))
        result = WeekDay(result);

    args.rval().setNumber(result);
    return true;
}

static JSBool
date_getHours(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getHours, &DateClass, &ok);
    if (!obj)
        return ok;

    if (!GetAndCacheLocalTime(cx, obj))
        return false;

    args.rval() = obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_HOURS);
    return true;
}

static JSBool
date_getUTCHours(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getUTCHours, &DateClass, &ok);
    if (!obj)
        return ok;

    double result = obj->getDateUTCTime().toNumber();
    if (JSDOUBLE_IS_FINITE(result))
        result = HourFromTime(result);

    args.rval().setNumber(result);
    return JS_TRUE;
}

static JSBool
date_getMinutes(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getMinutes, &DateClass, &ok);
    if (!obj)
        return ok;

    if (!GetAndCacheLocalTime(cx, obj))
        return false;

    args.rval() = obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_MINUTES);
    return true;
}

static JSBool
date_getUTCMinutes(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getUTCMinutes, &DateClass, &ok);
    if (!obj)
        return ok;

    double result = obj->getDateUTCTime().toNumber();
    if (JSDOUBLE_IS_FINITE(result))
        result = MinFromTime(result);

    args.rval().setNumber(result);
    return true;
}



static JSBool
date_getUTCSeconds(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getUTCSeconds, &DateClass, &ok);
    if (!obj)
        return ok;

    if (!GetAndCacheLocalTime(cx, obj))
        return false;

    args.rval() = obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_SECONDS);
    return true;
}



static JSBool
date_getUTCMilliseconds(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getUTCMilliseconds, &DateClass, &ok);
    if (!obj)
        return ok;

    double result = obj->getDateUTCTime().toNumber();
    if (JSDOUBLE_IS_FINITE(result))
        result = msFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static JSBool
date_getTimezoneOffset(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_getTimezoneOffset, &DateClass, &ok);
    if (!obj)
        return ok;

    double utctime = obj->getDateUTCTime().toNumber();

    double localtime;
    if (!GetAndCacheLocalTime(cx, obj, &localtime))
        return false;

    




    double result = (utctime - localtime) / msPerMinute;
    args.rval().setNumber(result);
    return true;
}

static JSBool
date_setTime(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_setTime, &DateClass, &ok);
    if (!obj)
        return ok;

    if (args.length() == 0) {
        SetDateToNaN(cx, obj, &args.rval());
        return true;
    }

    double result;
    if (!ToNumber(cx, args[0], &result))
        return false;

    return SetUTCTime(cx, obj, TIMECLIP(result), &args.rval());
}

static JSBool
date_makeTime(JSContext *cx, Native native, unsigned maxargs, JSBool local, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, native, &DateClass, &ok);
    if (!obj)
        return ok;

    double result = obj->getDateUTCTime().toNumber();

    








    if (args.length() == 0) {
        SetDateToNaN(cx, obj, &args.rval());
        return true;
    }

    unsigned numNums = Min(args.length(), maxargs);
    JS_ASSERT(numNums <= 4);
    double nums[4];
    bool argIsNotFinite = false;
    for (unsigned i = 0; i < numNums; i++) {
        if (!ToNumber(cx, args[i], &nums[i]))
            return false;
        if (!JSDOUBLE_IS_FINITE(nums[i])) {
            argIsNotFinite = true;
        } else {
            nums[i] = js_DoubleToInteger(nums[i]);
        }
    }

    



    if (!JSDOUBLE_IS_FINITE(result)) {
        args.rval().setNumber(result);
        return true;
    }

    
    if (argIsNotFinite) {
        SetDateToNaN(cx, obj, &args.rval());
        return true;
    }

    double lorutime;  
    if (local)
        lorutime = LocalTime(result, cx);
    else
        lorutime = result;

    double *argp = nums;
    double *stop = argp + numNums;
    double hour;
    if (maxargs >= 4 && argp < stop)
        hour = *argp++;
    else
        hour = HourFromTime(lorutime);

    double min;
    if (maxargs >= 3 && argp < stop)
        min = *argp++;
    else
        min = MinFromTime(lorutime);

    double sec;
    if (maxargs >= 2 && argp < stop)
        sec = *argp++;
    else
        sec = SecFromTime(lorutime);

    double msec;
    if (maxargs >= 1 && argp < stop)
        msec = *argp;
    else
        msec = msFromTime(lorutime);

    double msec_time = MakeTime(hour, min, sec, msec);
    result = MakeDate(Day(lorutime), msec_time);

    if (local)
        result = UTC(result, cx);

    return SetUTCTime(cx, obj, TIMECLIP(result), &args.rval());
}

static JSBool
date_setMilliseconds(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeTime(cx, date_setMilliseconds, 1, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCMilliseconds(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeTime(cx, date_setUTCMilliseconds, 1, JS_FALSE, argc, vp);
}

static JSBool
date_setSeconds(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeTime(cx, date_setSeconds, 2, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCSeconds(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeTime(cx, date_setUTCSeconds, 2, JS_FALSE, argc, vp);
}

static JSBool
date_setMinutes(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeTime(cx, date_setMinutes, 3, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCMinutes(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeTime(cx, date_setUTCMinutes, 3, JS_FALSE, argc, vp);
}

static JSBool
date_setHours(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeTime(cx, date_setHours, 4, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCHours(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeTime(cx, date_setUTCHours, 4, JS_FALSE, argc, vp);
}

static JSBool
date_makeDate(JSContext *cx, Native native, unsigned maxargs, JSBool local, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, native, &DateClass, &ok);
    if (!obj)
        return ok;

    double result = obj->getDateUTCTime().toNumber();

    
    if (args.length() == 0) {
        SetDateToNaN(cx, obj, &args.rval());
        return true;
    }

    unsigned numNums = Min(args.length(), maxargs);
    JS_ASSERT(1 <= numNums && numNums <= 3);
    double nums[3];
    bool argIsNotFinite = false;
    for (unsigned i = 0; i < numNums; i++) {
        if (!ToNumber(cx, args[i], &nums[i]))
            return JS_FALSE;
        if (!JSDOUBLE_IS_FINITE(nums[i])) {
            argIsNotFinite = true;
        } else {
            nums[i] = js_DoubleToInteger(nums[i]);
        }
    }

    
    if (argIsNotFinite) {
        SetDateToNaN(cx, obj, &args.rval());
        return true;
    }

    



    double lorutime; 
    if (!JSDOUBLE_IS_FINITE(result)) {
        if (maxargs < 3) {
            args.rval().setDouble(result);
            return true;
        }
        lorutime = +0.;
    } else {
        lorutime = local ? LocalTime(result, cx) : result;
    }

    double *argp = nums;
    double *stop = argp + numNums;
    double year;
    if (maxargs >= 3 && argp < stop)
        year = *argp++;
    else
        year = YearFromTime(lorutime);

    double month;
    if (maxargs >= 2 && argp < stop)
        month = *argp++;
    else
        month = MonthFromTime(lorutime);

    double day;
    if (maxargs >= 1 && argp < stop)
        day = *argp++;
    else
        day = DateFromTime(lorutime);

    day = MakeDay(year, month, day); 
    result = MakeDate(day, TimeWithinDay(lorutime));

    if (local)
        result = UTC(result, cx);

    return SetUTCTime(cx, obj, TIMECLIP(result), &args.rval());
}

static JSBool
date_setDate(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeDate(cx, date_setDate, 1, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCDate(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeDate(cx, date_setUTCDate, 1, JS_FALSE, argc, vp);
}

static JSBool
date_setMonth(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeDate(cx, date_setMonth, 2, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCMonth(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeDate(cx, date_setUTCMonth, 2, JS_FALSE, argc, vp);
}

static JSBool
date_setFullYear(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeDate(cx, date_setFullYear, 3, JS_TRUE, argc, vp);
}

static JSBool
date_setUTCFullYear(JSContext *cx, unsigned argc, Value *vp)
{
    return date_makeDate(cx, date_setUTCFullYear, 3, JS_FALSE, argc, vp);
}

static JSBool
date_setYear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_setYear, &DateClass, &ok);
    if (!obj)
        return ok;

    if (args.length() == 0) {
        
        SetDateToNaN(cx, obj, &args.rval());
        return true;
    }

    double result = obj->getDateUTCTime().toNumber();

    double year;
    if (!ToNumber(cx, args[0], &year))
        return false;
    if (!JSDOUBLE_IS_FINITE(year)) {
        SetDateToNaN(cx, obj, &args.rval());
        return true;
    }
    year = js_DoubleToInteger(year);
    if (year >= 0 && year <= 99)
        year += 1900;

    double t = JSDOUBLE_IS_FINITE(result) ? LocalTime(result, cx) : +0.0;
    double day = MakeDay(year, MonthFromTime(t), DateFromTime(t));
    result = MakeDate(day, TimeWithinDay(t));
    result = UTC(result, cx);

    return SetUTCTime(cx, obj, TIMECLIP(result), &args.rval());
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




static void
print_gmt_string(char* buf, size_t size, double utctime)
{
    JS_snprintf(buf, size, "%s, %.2d %s %.4d %.2d:%.2d:%.2d GMT",
                days[WeekDay(utctime)],
                DateFromTime(utctime),
                months[MonthFromTime(utctime)],
                YearFromTime(utctime),
                HourFromTime(utctime),
                MinFromTime(utctime),
                SecFromTime(utctime));
}

static void
print_iso_string(char* buf, size_t size, double utctime)
{
    JS_snprintf(buf, size, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3dZ",
                YearFromTime(utctime),
                MonthFromTime(utctime) + 1,
                DateFromTime(utctime),
                HourFromTime(utctime),
                MinFromTime(utctime),
                SecFromTime(utctime),
                msFromTime(utctime));
}

static JSBool
date_utc_format(JSContext *cx, Native native, CallArgs args,
                void (*printFunc)(char*, size_t, double))
{
    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, native, &DateClass, &ok);
    if (!obj)
        return ok;

    double utctime = obj->getDateUTCTime().toNumber();

    char buf[100];
    if (!JSDOUBLE_IS_FINITE(utctime)) {
        if (printFunc == print_iso_string) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INVALID_DATE);
            return false;
        }

        JS_snprintf(buf, sizeof buf, js_NaN_date_str);
    } else {
        (*printFunc)(buf, sizeof buf, utctime);
    }

    JSString *str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static JSBool
date_toGMTString(JSContext *cx, unsigned argc, Value *vp)
{
    return date_utc_format(cx, date_toGMTString, CallArgsFromVp(argc, vp), print_gmt_string);
}

static JSBool
date_toISOString(JSContext *cx, unsigned argc, Value *vp)
{
    return date_utc_format(cx, date_toISOString, CallArgsFromVp(argc, vp), print_iso_string);
}


static JSBool
date_toJSON(JSContext *cx, unsigned argc, Value *vp)
{
    
    JSObject *obj = ToObject(cx, &vp[1]);
    if (!obj)
        return false;

    
    Value tv = ObjectValue(*obj);
    if (!ToPrimitive(cx, JSTYPE_NUMBER, &tv))
        return false;

    
    if (tv.isDouble() && !JSDOUBLE_IS_FINITE(tv.toDouble())) {
        vp->setNull();
        return true;
    }

    
    Value &toISO = vp[0];
    if (!obj->getProperty(cx, cx->runtime->atomState.toISOStringAtom, &toISO))
        return false;

    
    if (!js_IsCallable(toISO)) {
        JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_TOISOSTRING_PROP);
        return false;
    }

    
    InvokeArgsGuard args;
    if (!cx->stack.pushInvokeArgs(cx, 0, &args))
        return false;

    args.calleev() = toISO;
    args.thisv().setObject(*obj);

    if (!Invoke(cx, args))
        return false;
    *vp = args.rval();
    return true;
}



static void
new_explode(double timeval, PRMJTime *split, JSContext *cx)
{
    int year = YearFromTime(timeval);

    split->tm_usec = int32_t(msFromTime(timeval)) * 1000;
    split->tm_sec = int8_t(SecFromTime(timeval));
    split->tm_min = int8_t(MinFromTime(timeval));
    split->tm_hour = int8_t(HourFromTime(timeval));
    split->tm_mday = int8_t(DateFromTime(timeval));
    split->tm_mon = int8_t(MonthFromTime(timeval));
    split->tm_wday = int8_t(WeekDay(timeval));
    split->tm_year = year;
    split->tm_yday = int16_t(DayWithinYear(timeval, year));

    

    split->tm_isdst = (DaylightSavingTA(timeval, cx) != 0);
}

typedef enum formatspec {
    FORMATSPEC_FULL, FORMATSPEC_DATE, FORMATSPEC_TIME
} formatspec;


static JSBool
date_format(JSContext *cx, double date, formatspec format, CallReceiver call)
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
        double local = LocalTime(date, cx);

        

        int minutes = (int) floor(AdjustTime(date, cx) / msPerMinute);

        
        int offset = (minutes / 60) * 100 + minutes % 60;

        








        

        new_explode(date, &split, cx);
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
    call.rval().setString(str);
    return JS_TRUE;
}

static bool
ToLocaleHelper(JSContext *cx, CallReceiver call, JSObject *obj, const char *format)
{
    double utctime = obj->getDateUTCTime().toNumber();

    char buf[100];
    if (!JSDOUBLE_IS_FINITE(utctime)) {
        JS_snprintf(buf, sizeof buf, js_NaN_date_str);
    } else {
        int result_len;
        double local = LocalTime(utctime, cx);
        PRMJTime split;
        new_explode(local, &split, cx);

        
        result_len = PRMJ_FormatTime(buf, sizeof buf, format, &split);

        
        if (result_len == 0)
            return date_format(cx, utctime, FORMATSPEC_FULL, call);

        
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
        return cx->localeCallbacks->localeToUnicode(cx, buf, &call.rval());

    JSString *str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return false;
    call.rval().setString(str);
    return true;
}





static JSBool
date_toLocaleHelper(JSContext *cx, unsigned argc, Value *vp, Native native, const char *format)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, native, &DateClass, &ok);
    if (!obj)
        return ok;

    return ToLocaleHelper(cx, args, obj, format);
}

static JSBool
date_toLocaleStringHelper(JSContext *cx, Native native, unsigned argc, Value *vp)
{
    



    return date_toLocaleHelper(cx, argc, vp, native,
#if defined(_WIN32) && !defined(__MWERKS__)
                                   "%#c"
#else
                                   "%c"
#endif
                               );
}

static JSBool
date_toLocaleString(JSContext *cx, unsigned argc, Value *vp)
{
    return date_toLocaleStringHelper(cx, date_toLocaleString, argc, vp);
}

static JSBool
date_toLocaleDateString(JSContext *cx, unsigned argc, Value *vp)
{
    



    return date_toLocaleHelper(cx, argc, vp, date_toLocaleDateString,
#if defined(_WIN32) && !defined(__MWERKS__)
                                   "%#x"
#else
                                   "%x"
#endif
                               );
}

static JSBool
date_toLocaleTimeString(JSContext *cx, unsigned argc, Value *vp)
{
    return date_toLocaleHelper(cx, argc, vp, date_toLocaleTimeString, "%X");
}

static JSBool
date_toLocaleFormat(JSContext *cx, unsigned argc, Value *vp)
{
    if (argc == 0)
        return date_toLocaleStringHelper(cx, date_toLocaleFormat, argc, vp);

    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_toLocaleFormat, &DateClass, &ok);
    if (!obj)
        return ok;

    JSString *fmt = ToString(cx, args[0]);
    if (!fmt)
        return false;

    args[0].setString(fmt);
    JSAutoByteString fmtbytes(cx, fmt);
    if (!fmtbytes)
        return false;

    return ToLocaleHelper(cx, args, obj, fmtbytes.ptr());
}

static JSBool
date_toTimeString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_toTimeString, &DateClass, &ok);
    if (!obj)
        return ok;

    return date_format(cx, obj->getDateUTCTime().toNumber(), FORMATSPEC_TIME, args);
}

static JSBool
date_toDateString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_toDateString, &DateClass, &ok);
    if (!obj)
        return ok;

    return date_format(cx, obj->getDateUTCTime().toNumber(), FORMATSPEC_DATE, args);
}

#if JS_HAS_TOSOURCE
static JSBool
date_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_toSource, &DateClass, &ok);
    if (!obj)
        return ok;

    StringBuffer sb(cx);
    if (!sb.append("(new Date(") || !NumberValueToStringBuffer(cx, obj->getDateUTCTime(), sb) ||
        !sb.append("))"))
    {
        return false;
    }

    JSString *str = sb.finishString();
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}
#endif

static JSBool
date_toString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_toString, &DateClass, &ok);
    if (!obj)
        return ok;

    return date_format(cx, obj->getDateUTCTime().toNumber(), FORMATSPEC_FULL, args);
}

static JSBool
date_valueOf(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    bool ok;
    JSObject *obj = NonGenericMethodGuard(cx, args, date_valueOf, &DateClass, &ok);
    if (!obj)
        return ok;

    
    if (argc == 0) {
        args.rval() = obj->getDateUTCTime();
        return true;
    }

    
    JSString *str = ToString(cx, args[0]);
    if (!str)
        return false;
    JSLinearString *linear_str = str->ensureLinear(cx);
    if (!linear_str)
        return false;
    JSAtom *number_str = cx->runtime->atomState.typeAtoms[JSTYPE_NUMBER];
    if (EqualStrings(linear_str, number_str)) {
        args.rval() = obj->getDateUTCTime();
        return true;
    }
    return date_format(cx, obj->getDateUTCTime().toNumber(), FORMATSPEC_FULL, args);
}

static JSFunctionSpec date_static_methods[] = {
    JS_FN("UTC",                 date_UTC,                MAXARGS,0),
    JS_FN("parse",               date_parse,              1,0),
    JS_FN("now",                 date_now,                0,0),
    JS_FS_END
};

static JSFunctionSpec date_methods[] = {
    JS_FN("getTime",             date_getTime,            0,0),
    JS_FN("getTimezoneOffset",   date_getTimezoneOffset,  0,0),
    JS_FN("getYear",             date_getYear,            0,0),
    JS_FN("getFullYear",         date_getFullYear,        0,0),
    JS_FN("getUTCFullYear",      date_getUTCFullYear,     0,0),
    JS_FN("getMonth",            date_getMonth,           0,0),
    JS_FN("getUTCMonth",         date_getUTCMonth,        0,0),
    JS_FN("getDate",             date_getDate,            0,0),
    JS_FN("getUTCDate",          date_getUTCDate,         0,0),
    JS_FN("getDay",              date_getDay,             0,0),
    JS_FN("getUTCDay",           date_getUTCDay,          0,0),
    JS_FN("getHours",            date_getHours,           0,0),
    JS_FN("getUTCHours",         date_getUTCHours,        0,0),
    JS_FN("getMinutes",          date_getMinutes,         0,0),
    JS_FN("getUTCMinutes",       date_getUTCMinutes,      0,0),
    JS_FN("getSeconds",          date_getUTCSeconds,      0,0),
    JS_FN("getUTCSeconds",       date_getUTCSeconds,      0,0),
    JS_FN("getMilliseconds",     date_getUTCMilliseconds, 0,0),
    JS_FN("getUTCMilliseconds",  date_getUTCMilliseconds, 0,0),
    JS_FN("setTime",             date_setTime,            1,0),
    JS_FN("setYear",             date_setYear,            1,0),
    JS_FN("setFullYear",         date_setFullYear,        3,0),
    JS_FN("setUTCFullYear",      date_setUTCFullYear,     3,0),
    JS_FN("setMonth",            date_setMonth,           2,0),
    JS_FN("setUTCMonth",         date_setUTCMonth,        2,0),
    JS_FN("setDate",             date_setDate,            1,0),
    JS_FN("setUTCDate",          date_setUTCDate,         1,0),
    JS_FN("setHours",            date_setHours,           4,0),
    JS_FN("setUTCHours",         date_setUTCHours,        4,0),
    JS_FN("setMinutes",          date_setMinutes,         3,0),
    JS_FN("setUTCMinutes",       date_setUTCMinutes,      3,0),
    JS_FN("setSeconds",          date_setSeconds,         2,0),
    JS_FN("setUTCSeconds",       date_setUTCSeconds,      2,0),
    JS_FN("setMilliseconds",     date_setMilliseconds,    1,0),
    JS_FN("setUTCMilliseconds",  date_setUTCMilliseconds, 1,0),
    JS_FN("toUTCString",         date_toGMTString,        0,0),
    JS_FN(js_toLocaleString_str, date_toLocaleString,     0,0),
    JS_FN("toLocaleDateString",  date_toLocaleDateString, 0,0),
    JS_FN("toLocaleTimeString",  date_toLocaleTimeString, 0,0),
    JS_FN("toLocaleFormat",      date_toLocaleFormat,     0,0),
    JS_FN("toDateString",        date_toDateString,       0,0),
    JS_FN("toTimeString",        date_toTimeString,       0,0),
    JS_FN("toISOString",         date_toISOString,        0,0),
    JS_FN(js_toJSON_str,         date_toJSON,             1,0),
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,       date_toSource,           0,0),
#endif
    JS_FN(js_toString_str,       date_toString,           0,0),
    JS_FN(js_valueOf_str,        date_valueOf,            0,0),
    JS_FS_END
};

JSBool
js_Date(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    if (!IsConstructing(args))
        return date_format(cx, NowAsMillis(), FORMATSPEC_FULL, args);

    
    double d;
    if (args.length() == 0) {
        d = NowAsMillis();
    } else if (args.length() == 1) {
        if (!args[0].isString()) {
            
            if (!ToNumber(cx, args[0], &d))
                return false;
            d = TIMECLIP(d);
        } else {
            
            JSString *str = ToString(cx, args[0]);
            if (!str)
                return false;
            args[0].setString(str);
            JSLinearString *linearStr = str->ensureLinear(cx);
            if (!linearStr)
                return false;

            if (!date_parseString(linearStr, &d, cx))
                d = js_NaN;
            else
                d = TIMECLIP(d);
        }
    } else {
        double msec_time;
        if (!date_msecFromArgs(cx, args, &msec_time))
            return false;

        if (JSDOUBLE_IS_FINITE(msec_time)) {
            msec_time = UTC(msec_time, cx);
            msec_time = TIMECLIP(msec_time);
        }
        d = msec_time;
    }

    JSObject *obj = js_NewDateObjectMsec(cx, d);
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

JSObject *
js_InitDateClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    
    LocalTZA = -(PRMJ_LocalGMTDifference() * msPerSecond);

    RootedVar<GlobalObject*> global(cx, &obj->asGlobal());

    RootedVarObject dateProto(cx, global->createBlankPrototype(cx, &DateClass));
    if (!dateProto)
        return NULL;
    SetDateToNaN(cx, dateProto);

    RootedVarFunction ctor(cx);
    ctor = global->createConstructor(cx, js_Date, CLASS_ATOM(cx, Date), MAXARGS);
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, dateProto))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, ctor, NULL, date_static_methods))
        return NULL;

    




    if (!JS_DefineFunctions(cx, dateProto, date_methods))
        return NULL;
    Value toUTCStringFun;
    jsid toUTCStringId = ATOM_TO_JSID(cx->runtime->atomState.toUTCStringAtom);
    jsid toGMTStringId = ATOM_TO_JSID(cx->runtime->atomState.toGMTStringAtom);
    if (!js_GetProperty(cx, dateProto, toUTCStringId, &toUTCStringFun) ||
        !js_DefineProperty(cx, dateProto, toGMTStringId, &toUTCStringFun,
                           JS_PropertyStub, JS_StrictPropertyStub, 0))
    {
        return NULL;
    }

    if (!DefineConstructorAndPrototype(cx, global, JSProto_Date, ctor, dateProto))
        return NULL;

    return dateProto;
}

JS_FRIEND_API(JSObject *)
js_NewDateObjectMsec(JSContext *cx, double msec_time)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &DateClass);
    if (!obj)
        return NULL;
    if (!SetUTCTime(cx, obj, msec_time))
        return NULL;
    return obj;
}

JS_FRIEND_API(JSObject *)
js_NewDateObject(JSContext* cx, int year, int mon, int mday,
                 int hour, int min, int sec)
{
    JSObject *obj;
    double msec_time;

    JS_ASSERT(mon < 12);
    msec_time = date_msecFromDate(year, mon, mday, hour, min, sec, 0);
    obj = js_NewDateObjectMsec(cx, UTC(msec_time, cx));
    return obj;
}

JS_FRIEND_API(JSBool)
js_DateIsValid(JSContext *cx, JSObject* obj)
{
    return obj->isDate() && !JSDOUBLE_IS_NaN(obj->getDateUTCTime().toNumber());
}

JS_FRIEND_API(int)
js_DateGetYear(JSContext *cx, JSObject* obj)
{
    double localtime;

    
    if (!GetAndCacheLocalTime(cx, obj, &localtime) ||
        JSDOUBLE_IS_NaN(localtime)) {
        return 0;
    }

    return (int) YearFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetMonth(JSContext *cx, JSObject* obj)
{
    double localtime;

    if (!GetAndCacheLocalTime(cx, obj, &localtime) ||
        JSDOUBLE_IS_NaN(localtime)) {
        return 0;
    }

    return (int) MonthFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetDate(JSContext *cx, JSObject* obj)
{
    double localtime;

    if (!GetAndCacheLocalTime(cx, obj, &localtime) ||
        JSDOUBLE_IS_NaN(localtime)) {
        return 0;
    }

    return (int) DateFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetHours(JSContext *cx, JSObject* obj)
{
    double localtime;

    if (!GetAndCacheLocalTime(cx, obj, &localtime) ||
        JSDOUBLE_IS_NaN(localtime)) {
        return 0;
    }

    return (int) HourFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetMinutes(JSContext *cx, JSObject* obj)
{
    double localtime;

    if (!GetAndCacheLocalTime(cx, obj, &localtime) ||
        JSDOUBLE_IS_NaN(localtime)) {
        return 0;
    }

    return (int) MinFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetSeconds(JSContext *cx, JSObject* obj)
{
    if (!obj->isDate()) 
        return 0;
    
    double utctime = obj->getDateUTCTime().toNumber();
    if (JSDOUBLE_IS_NaN(utctime))
        return 0;
    return (int) SecFromTime(utctime);
}

JS_FRIEND_API(double)
js_DateGetMsecSinceEpoch(JSContext *cx, JSObject *obj)
{
    return obj->isDate() ? obj->getDateUTCTime().toNumber() : 0;
}

#ifdef JS_THREADSAFE
#include "prinrval.h"

JS_FRIEND_API(uint32_t)
js_IntervalNow()
{
    return uint32_t(PR_IntervalToMilliseconds(PR_IntervalNow()));
}

#else 

JS_FRIEND_API(uint32_t)
js_IntervalNow()
{
    return uint32_t(PRMJ_Now() / PRMJ_USEC_PER_MSEC);
}
#endif
