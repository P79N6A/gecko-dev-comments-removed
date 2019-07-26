










#include "mozilla/FloatingPoint.h"
#include "mozilla/Util.h"










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
#include "jsversion.h"
#include "jscntxt.h"
#include "jsdate.h"
#include "jsinterp.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstr.h"
#include "jslibmath.h"

#include "vm/DateTime.h"
#include "vm/GlobalObject.h"
#include "vm/NumericConversions.h"
#include "vm/String.h"
#include "vm/StringBuffer.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"

#include "vm/Stack-inl.h"

using namespace js;
using namespace js::types;

using mozilla::ArrayLength;

















































inline double
Day(double t)
{
    return floor(t / msPerDay);
}

static double
TimeWithinDay(double t)
{
    double result = fmod(t, msPerDay);
    if (result < 0)
        result += msPerDay;
    return result;
}


inline bool
IsLeapYear(double year)
{
    JS_ASSERT(ToInteger(year) == year);
    return fmod(year, 4) == 0 && (fmod(year, 100) != 0 || fmod(year, 400) == 0);
}

inline double
DaysInYear(double year)
{
    if (!MOZ_DOUBLE_IS_FINITE(year))
        return js_NaN;
    return IsLeapYear(year) ? 366 : 365;
}

inline double
DayFromYear(double y)
{
    return 365 * (y - 1970) +
           floor((y - 1969) / 4.0) -
           floor((y - 1901) / 100.0) +
           floor((y - 1601) / 400.0);
}

inline double
TimeFromYear(double y)
{
    return DayFromYear(y) * msPerDay;
}

static double
YearFromTime(double t)
{
    if (!MOZ_DOUBLE_IS_FINITE(t))
        return js_NaN;

    JS_ASSERT(ToInteger(t) == t);

    double y = floor(t / (msPerDay * 365.2425)) + 1970;
    double t2 = TimeFromYear(y);

    




    if (t2 > t) {
        y--;
    } else {
        if (t2 + msPerDay * DaysInYear(y) <= t)
            y++;
    }
    return y;
}

inline int
DaysInFebruary(double year)
{
    return IsLeapYear(year) ? 29 : 28;
}


inline double
DayWithinYear(double t, double year)
{
    JS_ASSERT_IF(MOZ_DOUBLE_IS_FINITE(t), YearFromTime(t) == year);
    return Day(t) - DayFromYear(year);
}

static double
MonthFromTime(double t)
{
    if (!MOZ_DOUBLE_IS_FINITE(t))
        return js_NaN;

    double year = YearFromTime(t);
    double d = DayWithinYear(t, year);

    int step;
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


static double
DateFromTime(double t)
{
    if (!MOZ_DOUBLE_IS_FINITE(t))
        return js_NaN;

    double year = YearFromTime(t);
    double d = DayWithinYear(t, year);

    int next;
    if (d <= (next = 30))
        return d + 1;
    int step = next;
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
    



    JS_ASSERT(ToInteger(t) == t);
    int result = (int(Day(t)) + 4) % 7;
    if (result < 0)
        result += 7;
    return result;
}

inline int
DayFromMonth(int month, bool isLeapYear)
{
    



    static const int firstDayOfMonth[2][13] = {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
    };

    JS_ASSERT(0 <= month && month <= 12);
    return firstDayOfMonth[isLeapYear][month];
}

template<typename T>
inline int
DayFromMonth(T month, bool isLeapYear) MOZ_DELETE;


static double
MakeDay(double year, double month, double date)
{
    
    if (!MOZ_DOUBLE_IS_FINITE(year) || !MOZ_DOUBLE_IS_FINITE(month) || !MOZ_DOUBLE_IS_FINITE(date))
        return js_NaN;

    
    double y = ToInteger(year);
    double m = ToInteger(month);
    double dt = ToInteger(date);

    
    double ym = y + floor(m / 12);

    
    int mn = int(fmod(m, 12.0));
    if (mn < 0)
        mn += 12;

    
    bool leap = IsLeapYear(ym);

    double yearday = floor(TimeFromYear(ym) / msPerDay);
    double monthday = DayFromMonth(mn, leap);

    return yearday + monthday + dt - 1;
}


inline double
MakeDate(double day, double time)
{
    
    if (!MOZ_DOUBLE_IS_FINITE(day) || !MOZ_DOUBLE_IS_FINITE(time))
        return js_NaN;

    
    return day * msPerDay + time;
}








static int
EquivalentYearForDST(int year)
{
    








    static const int yearStartingWith[2][7] = {
        {1978, 1973, 1974, 1975, 1981, 1971, 1977},
        {1984, 1996, 1980, 1992, 1976, 1988, 1972}
    };

    int day = int(DayFromYear(year) + 4) % 7;
    if (day < 0)
        day += 7;

    return yearStartingWith[IsLeapYear(year)][day];
}


static double
DaylightSavingTA(double t, DateTimeInfo *dtInfo)
{
    if (!MOZ_DOUBLE_IS_FINITE(t))
        return js_NaN;

    



    if (t < 0.0 || t > 2145916800000.0) {
        int year = EquivalentYearForDST(int(YearFromTime(t)));
        double day = MakeDay(year, MonthFromTime(t), DateFromTime(t));
        t = MakeDate(day, TimeWithinDay(t));
    }

    int64_t utcMilliseconds = static_cast<int64_t>(t);
    int64_t offsetMilliseconds = dtInfo->getDSTOffsetMilliseconds(utcMilliseconds);
    return static_cast<double>(offsetMilliseconds);
}

static double
AdjustTime(double date, DateTimeInfo *dtInfo)
{
    double t = DaylightSavingTA(date, dtInfo) + dtInfo->localTZA();
    t = (dtInfo->localTZA() >= 0) ? fmod(t, msPerDay) : -fmod(msPerDay - t, msPerDay);
    return t;
}


static double
LocalTime(double t, DateTimeInfo *dtInfo)
{
    return t + AdjustTime(t, dtInfo);
}

static double
UTC(double t, DateTimeInfo *dtInfo)
{
    return t - AdjustTime(t - dtInfo->localTZA(), dtInfo);
}


static double
HourFromTime(double t)
{
    double result = fmod(floor(t/msPerHour), HoursPerDay);
    if (result < 0)
        result += HoursPerDay;
    return result;
}

static double
MinFromTime(double t)
{
    double result = fmod(floor(t / msPerMinute), MinutesPerHour);
    if (result < 0)
        result += MinutesPerHour;
    return result;
}

static double
SecFromTime(double t)
{
    double result = fmod(floor(t / msPerSecond), SecondsPerMinute);
    if (result < 0)
        result += SecondsPerMinute;
    return result;
}

static double
msFromTime(double t)
{
    double result = fmod(t, msPerSecond);
    if (result < 0)
        result += msPerSecond;
    return result;
}


static double
MakeTime(double hour, double min, double sec, double ms)
{
    
    if (!MOZ_DOUBLE_IS_FINITE(hour) ||
        !MOZ_DOUBLE_IS_FINITE(min) ||
        !MOZ_DOUBLE_IS_FINITE(sec) ||
        !MOZ_DOUBLE_IS_FINITE(ms))
    {
        return js_NaN;
    }

    
    double h = ToInteger(hour);

    
    double m = ToInteger(min);

    
    double s = ToInteger(sec);

    
    double milli = ToInteger(ms);

    
    return h * msPerHour + m * msPerMinute + s * msPerSecond + milli;
}





static JSBool
date_convert(JSContext *cx, HandleObject obj, JSType hint, MutableHandleValue vp)
{
    JS_ASSERT(hint == JSTYPE_NUMBER || hint == JSTYPE_STRING || hint == JSTYPE_VOID);
    JS_ASSERT(obj->isDate());

    return DefaultValue(cx, obj, (hint == JSTYPE_VOID) ? JSTYPE_STRING : hint, vp);
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
    return MakeDate(MakeDay(year, mon, mday), MakeTime(hour, min, sec, msec));
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
            
            if (!MOZ_DOUBLE_IS_FINITE(d)) {
                *rval = js_NaN;
                return JS_TRUE;
            }
            array[loop] = ToInteger(d);
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

    msec_time = TimeClip(msec_time);

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

    if (digits(result, s, i, Min(limit, init+n)))
        return ((*i - init) == n);

    *i = init;
    return JS_FALSE;
}

static int
DaysInMonth(int year, int month)
{
    bool leap = IsLeapYear(year);
    int result = int(DayFromMonth(month, leap) - DayFromMonth(month - 1, leap));
    return result;
}

























































static JSBool
date_parseISOString(RawLinearString str, double *result, DateTimeInfo *dtInfo)
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
        msec = UTC(msec, dtInfo);
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
date_parseString(RawLinearString str, double *result, DateTimeInfo *dtInfo)
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

    if (date_parseISOString(str, result, dtInfo))
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
        msec = UTC(msec, dtInfo);
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
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() == 0) {
        vp->setDouble(js_NaN);
        return true;
    }

    RawString str = ToString<CanGC>(cx, args[0]);
    if (!str)
        return false;

    RawLinearString linearStr = str->ensureLinear(cx);
    if (!linearStr)
        return false;

    double result;
    if (!date_parseString(linearStr, &result, &cx->runtime->dateTimeInfo)) {
        vp->setDouble(js_NaN);
        return true;
    }

    result = TimeClip(result);
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




static void
SetUTCTime(RawObject obj, double t, Value *vp = NULL)
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
}






static void
FillLocalTimeSlots(DateTimeInfo *dtInfo, RawObject obj)
{
    JS_ASSERT(obj->isDate());

    
    if (!obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_TIME).isUndefined() &&
        obj->getSlot(JSObject::JSSLOT_DATE_TZA).toDouble() == dtInfo->localTZA())
    {
        return;
    }

    
    obj->setSlot(JSObject::JSSLOT_DATE_TZA, DoubleValue(dtInfo->localTZA()));

    double utcTime = obj->getDateUTCTime().toNumber();

    if (!MOZ_DOUBLE_IS_FINITE(utcTime)) {
        for (size_t ind = JSObject::JSSLOT_DATE_COMPONENTS_START;
             ind < JSObject::DATE_CLASS_RESERVED_SLOTS;
             ind++) {
            obj->setSlot(ind, DoubleValue(utcTime));
        }
        return;
    }

    double localTime = LocalTime(utcTime, dtInfo);

    obj->setSlot(JSObject::JSSLOT_DATE_LOCAL_TIME, DoubleValue(localTime));

    int year = (int) floor(localTime /(msPerDay * 365.2425)) + 1970;
    double yearStartTime = TimeFromYear(year);

    
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
}

inline double
GetCachedLocalTime(DateTimeInfo *dtInfo, RawObject obj)
{
    JS_ASSERT(obj);
    FillLocalTimeSlots(dtInfo, obj);
    return obj->getSlot(JSObject::JSSLOT_DATE_LOCAL_TIME).toDouble();
}

JS_ALWAYS_INLINE bool
IsDate(const Value &v)
{
    return v.isObject() && v.toObject().hasClass(&DateClass);
}




JS_ALWAYS_INLINE bool
date_getTime_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));
    args.rval().set(args.thisv().toObject().getDateUTCTime());
    return true;
}

static JSBool
date_getTime(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getTime_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getYear_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RawObject thisObj = &args.thisv().toObject();
    FillLocalTimeSlots(&cx->runtime->dateTimeInfo, thisObj);

    Value yearVal = thisObj->getSlot(JSObject::JSSLOT_DATE_LOCAL_YEAR);
    if (yearVal.isInt32()) {
        
        int year = yearVal.toInt32() - 1900;
        args.rval().setInt32(year);
    } else {
        args.rval().set(yearVal);
    }

    return true;
}

static JSBool
date_getYear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getYear_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getFullYear_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RawObject thisObj = &args.thisv().toObject();
    FillLocalTimeSlots(&cx->runtime->dateTimeInfo, thisObj);

    args.rval().set(thisObj->getSlot(JSObject::JSSLOT_DATE_LOCAL_YEAR));
    return true;
}

static JSBool
date_getFullYear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getFullYear_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getUTCFullYear_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    double result = args.thisv().toObject().getDateUTCTime().toNumber();
    if (MOZ_DOUBLE_IS_FINITE(result))
        result = YearFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static JSBool
date_getUTCFullYear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getUTCFullYear_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getMonth_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RawObject thisObj = &args.thisv().toObject();
    FillLocalTimeSlots(&cx->runtime->dateTimeInfo, thisObj);

    args.rval().set(thisObj->getSlot(JSObject::JSSLOT_DATE_LOCAL_MONTH));
    return true;
}

static JSBool
date_getMonth(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getMonth_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getUTCMonth_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    double d = args.thisv().toObject().getDateUTCTime().toNumber();
    args.rval().setNumber(MonthFromTime(d));
    return true;
}

static JSBool
date_getUTCMonth(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getUTCMonth_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getDate_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RawObject thisObj = &args.thisv().toObject();
    FillLocalTimeSlots(&cx->runtime->dateTimeInfo, thisObj);

    args.rval().set(thisObj->getSlot(JSObject::JSSLOT_DATE_LOCAL_DATE));
    return true;
}

static JSBool
date_getDate(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getDate_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getUTCDate_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    double result = args.thisv().toObject().getDateUTCTime().toNumber();
    if (MOZ_DOUBLE_IS_FINITE(result))
        result = DateFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static JSBool
date_getUTCDate(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getUTCDate_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getDay_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RawObject thisObj = &args.thisv().toObject();
    FillLocalTimeSlots(&cx->runtime->dateTimeInfo, thisObj);

    args.rval().set(thisObj->getSlot(JSObject::JSSLOT_DATE_LOCAL_DAY));
    return true;
}

static JSBool
date_getDay(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getDay_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getUTCDay_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    double result = args.thisv().toObject().getDateUTCTime().toNumber();
    if (MOZ_DOUBLE_IS_FINITE(result))
        result = WeekDay(result);

    args.rval().setNumber(result);
    return true;
}

static JSBool
date_getUTCDay(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getUTCDay_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getHours_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RawObject thisObj = &args.thisv().toObject();
    FillLocalTimeSlots(&cx->runtime->dateTimeInfo, thisObj);

    args.rval().set(thisObj->getSlot(JSObject::JSSLOT_DATE_LOCAL_HOURS));
    return true;
}

static JSBool
date_getHours(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getHours_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getUTCHours_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    double result = args.thisv().toObject().getDateUTCTime().toNumber();
    if (MOZ_DOUBLE_IS_FINITE(result))
        result = HourFromTime(result);

    args.rval().setNumber(result);
    return JS_TRUE;
}

static JSBool
date_getUTCHours(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getUTCHours_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getMinutes_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RawObject thisObj = &args.thisv().toObject();
    FillLocalTimeSlots(&cx->runtime->dateTimeInfo, thisObj);

    args.rval().set(thisObj->getSlot(JSObject::JSSLOT_DATE_LOCAL_MINUTES));
    return true;
}

static JSBool
date_getMinutes(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getMinutes_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getUTCMinutes_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    double result = args.thisv().toObject().getDateUTCTime().toNumber();
    if (MOZ_DOUBLE_IS_FINITE(result))
        result = MinFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static JSBool
date_getUTCMinutes(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getUTCMinutes_impl>(cx, args);
}



JS_ALWAYS_INLINE bool
date_getUTCSeconds_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RawObject thisObj = &args.thisv().toObject();
    FillLocalTimeSlots(&cx->runtime->dateTimeInfo, thisObj);

    args.rval().set(thisObj->getSlot(JSObject::JSSLOT_DATE_LOCAL_SECONDS));
    return true;
}

static JSBool
date_getUTCSeconds(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getUTCSeconds_impl>(cx, args);
}



JS_ALWAYS_INLINE bool
date_getUTCMilliseconds_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    double result = args.thisv().toObject().getDateUTCTime().toNumber();
    if (MOZ_DOUBLE_IS_FINITE(result))
        result = msFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static JSBool
date_getUTCMilliseconds(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getUTCMilliseconds_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_getTimezoneOffset_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RawObject thisObj = &args.thisv().toObject();
    double utctime = thisObj->getDateUTCTime().toNumber();
    double localtime = GetCachedLocalTime(&cx->runtime->dateTimeInfo, thisObj);

    




    double result = (utctime - localtime) / msPerMinute;
    args.rval().setNumber(result);
    return true;
}

static JSBool
date_getTimezoneOffset(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_getTimezoneOffset_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_setTime_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());
    if (args.length() == 0) {
        SetUTCTime(thisObj, js_NaN, args.rval().address());
        return true;
    }

    double result;
    if (!ToNumber(cx, args[0], &result))
        return false;

    SetUTCTime(thisObj, TimeClip(result), args.rval().address());
    return true;
}

static JSBool
date_setTime(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setTime_impl>(cx, args);
}

static bool
GetMsecsOrDefault(JSContext *cx, const CallArgs &args, unsigned i, double t, double *millis)
{
    if (args.length() <= i) {
        *millis = msFromTime(t);
        return true;
    }
    return ToNumber(cx, args[i], millis);
}

static bool
GetSecsOrDefault(JSContext *cx, const CallArgs &args, unsigned i, double t, double *sec)
{
    if (args.length() <= i) {
        *sec = SecFromTime(t);
        return true;
    }
    return ToNumber(cx, args[i], sec);
}

static bool
GetMinsOrDefault(JSContext *cx, const CallArgs &args, unsigned i, double t, double *mins)
{
    if (args.length() <= i) {
        *mins = MinFromTime(t);
        return true;
    }
    return ToNumber(cx, args[i], mins);
}


JS_ALWAYS_INLINE bool
date_setMilliseconds_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = LocalTime(thisObj->getDateUTCTime().toNumber(), &cx->runtime->dateTimeInfo);

    
    double milli;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &milli))
        return false;
    double time = MakeTime(HourFromTime(t), MinFromTime(t), SecFromTime(t), milli);

    
    double u = TimeClip(UTC(MakeDate(Day(t), time), &cx->runtime->dateTimeInfo));

    
    SetUTCTime(thisObj, u, args.rval().address());
    return true;
}

static JSBool
date_setMilliseconds(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setMilliseconds_impl>(cx, args);
}


JS_ALWAYS_INLINE bool
date_setUTCMilliseconds_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = thisObj->getDateUTCTime().toNumber();

    
    double milli;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &milli))
        return false;
    double time = MakeTime(HourFromTime(t), MinFromTime(t), SecFromTime(t), milli);

    
    double v = TimeClip(MakeDate(Day(t), time));

    
    SetUTCTime(thisObj, v, args.rval().address());
    return true;
}

static JSBool
date_setUTCMilliseconds(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCMilliseconds_impl>(cx, args);
}


JS_ALWAYS_INLINE bool
date_setSeconds_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = LocalTime(thisObj->getDateUTCTime().toNumber(), &cx->runtime->dateTimeInfo);

    
    double s;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &s))
        return false;

    
    double milli;
    if (!GetMsecsOrDefault(cx, args, 1, t, &milli))
        return false;

    
    double date = MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), s, milli));

    
    double u = TimeClip(UTC(date, &cx->runtime->dateTimeInfo));

    
    SetUTCTime(thisObj, u, args.rval().address());
    return true;
}


static JSBool
date_setSeconds(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setSeconds_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_setUTCSeconds_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = thisObj->getDateUTCTime().toNumber();

    
    double s;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &s))
        return false;

    
    double milli;
    if (!GetMsecsOrDefault(cx, args, 1, t, &milli))
        return false;

    
    double date = MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), s, milli));

    
    double v = TimeClip(date);

    
    SetUTCTime(thisObj, v, args.rval().address());
    return true;
}


static JSBool
date_setUTCSeconds(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCSeconds_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_setMinutes_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = LocalTime(thisObj->getDateUTCTime().toNumber(), &cx->runtime->dateTimeInfo);

    
    double m;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &m))
        return false;

    
    double s;
    if (!GetSecsOrDefault(cx, args, 1, t, &s))
        return false;

    
    double milli;
    if (!GetMsecsOrDefault(cx, args, 2, t, &milli))
        return false;

    
    double date = MakeDate(Day(t), MakeTime(HourFromTime(t), m, s, milli));

    
    double u = TimeClip(UTC(date, &cx->runtime->dateTimeInfo));

    
    SetUTCTime(thisObj, u, args.rval().address());
    return true;
}


static JSBool
date_setMinutes(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setMinutes_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_setUTCMinutes_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = thisObj->getDateUTCTime().toNumber();

    
    double m;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &m))
        return false;

    
    double s;
    if (!GetSecsOrDefault(cx, args, 1, t, &s))
        return false;

    
    double milli;
    if (!GetMsecsOrDefault(cx, args, 2, t, &milli))
        return false;

    
    double date = MakeDate(Day(t), MakeTime(HourFromTime(t), m, s, milli));

    
    double v = TimeClip(date);

    
    SetUTCTime(thisObj, v, args.rval().address());
    return true;
}


static JSBool
date_setUTCMinutes(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCMinutes_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_setHours_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = LocalTime(thisObj->getDateUTCTime().toNumber(), &cx->runtime->dateTimeInfo);

    
    double h;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &h))
        return false;

    
    double m;
    if (!GetMinsOrDefault(cx, args, 1, t, &m))
        return false;

    
    double s;
    if (!GetSecsOrDefault(cx, args, 2, t, &s))
        return false;

    
    double milli;
    if (!GetMsecsOrDefault(cx, args, 3, t, &milli))
        return false;

    
    double date = MakeDate(Day(t), MakeTime(h, m, s, milli));

    
    double u = TimeClip(UTC(date, &cx->runtime->dateTimeInfo));

    
    SetUTCTime(thisObj, u, args.rval().address());
    return true;
}


static JSBool
date_setHours(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setHours_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_setUTCHours_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = thisObj->getDateUTCTime().toNumber();

    
    double h;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &h))
        return false;

    
    double m;
    if (!GetMinsOrDefault(cx, args, 1, t, &m))
        return false;

    
    double s;
    if (!GetSecsOrDefault(cx, args, 2, t, &s))
        return false;

    
    double milli;
    if (!GetMsecsOrDefault(cx, args, 3, t, &milli))
        return false;

    
    double newDate = MakeDate(Day(t), MakeTime(h, m, s, milli));

    
    double v = TimeClip(newDate);

    
    SetUTCTime(thisObj, v, args.rval().address());
    return true;
}


static JSBool
date_setUTCHours(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCHours_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_setDate_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = LocalTime(thisObj->getDateUTCTime().toNumber(), &cx->runtime->dateTimeInfo);

    
    double dt;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &dt))
        return false;

    
    double newDate = MakeDate(MakeDay(YearFromTime(t), MonthFromTime(t), dt), TimeWithinDay(t));

    
    double u = TimeClip(UTC(newDate, &cx->runtime->dateTimeInfo));

    
    SetUTCTime(thisObj, u, args.rval().address());
    return true;
}


static JSBool
date_setDate(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setDate_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_setUTCDate_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = thisObj->getDateUTCTime().toNumber();

    
    double dt;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &dt))
        return false;

    
    double newDate = MakeDate(MakeDay(YearFromTime(t), MonthFromTime(t), dt), TimeWithinDay(t));

    
    double v = TimeClip(newDate);

    
    SetUTCTime(thisObj, v, args.rval().address());
    return true;
}

static JSBool
date_setUTCDate(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCDate_impl>(cx, args);
}

static bool
GetDateOrDefault(JSContext *cx, const CallArgs &args, unsigned i, double t, double *date)
{
    if (args.length() <= i) {
        *date = DateFromTime(t);
        return true;
    }
    return ToNumber(cx, args[i], date);
}

static bool
GetMonthOrDefault(JSContext *cx, const CallArgs &args, unsigned i, double t, double *month)
{
    if (args.length() <= i) {
        *month = MonthFromTime(t);
        return true;
    }
    return ToNumber(cx, args[i], month);
}


JS_ALWAYS_INLINE bool
date_setMonth_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = LocalTime(thisObj->getDateUTCTime().toNumber(), &cx->runtime->dateTimeInfo);

    
    double m;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &m))
        return false;

    
    double dt;
    if (!GetDateOrDefault(cx, args, 1, t, &dt))
        return false;

    
    double newDate = MakeDate(MakeDay(YearFromTime(t), m, dt), TimeWithinDay(t));

    
    double u = TimeClip(UTC(newDate, &cx->runtime->dateTimeInfo));

    
    SetUTCTime(thisObj, u, args.rval().address());
    return true;
}

static JSBool
date_setMonth(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setMonth_impl>(cx, args);
}


JS_ALWAYS_INLINE bool
date_setUTCMonth_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = thisObj->getDateUTCTime().toNumber();

    
    double m;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &m))
        return false;

    
    double dt;
    if (!GetDateOrDefault(cx, args, 1, t, &dt))
        return false;

    
    double newDate = MakeDate(MakeDay(YearFromTime(t), m, dt), TimeWithinDay(t));

    
    double v = TimeClip(newDate);

    
    SetUTCTime(thisObj, v, args.rval().address());
    return true;
}

static JSBool
date_setUTCMonth(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCMonth_impl>(cx, args);
}

static double
ThisLocalTimeOrZero(HandleObject date, DateTimeInfo *dtInfo)
{
    double t = date->getDateUTCTime().toNumber();
    if (MOZ_DOUBLE_IS_NaN(t))
        return +0;
    return LocalTime(t, dtInfo);
}

static double
ThisUTCTimeOrZero(HandleObject date)
{
    double t = date->getDateUTCTime().toNumber();
    return MOZ_DOUBLE_IS_NaN(t) ? +0 : t;
}


JS_ALWAYS_INLINE bool
date_setFullYear_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = ThisLocalTimeOrZero(thisObj, &cx->runtime->dateTimeInfo);

    
    double y;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &y))
        return false;

    
    double m;
    if (!GetMonthOrDefault(cx, args, 1, t, &m))
        return false;

    
    double dt;
    if (!GetDateOrDefault(cx, args, 2, t, &dt))
        return false;

    
    double newDate = MakeDate(MakeDay(y, m, dt), TimeWithinDay(t));

    
    double u = TimeClip(UTC(newDate, &cx->runtime->dateTimeInfo));

    
    SetUTCTime(thisObj, u, args.rval().address());
    return true;
}

static JSBool
date_setFullYear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setFullYear_impl>(cx, args);
}


JS_ALWAYS_INLINE bool
date_setUTCFullYear_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = ThisUTCTimeOrZero(thisObj);

    
    double y;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &y))
        return false;

    
    double m;
    if (!GetMonthOrDefault(cx, args, 1, t, &m))
        return false;

    
    double dt;
    if (!GetDateOrDefault(cx, args, 2, t, &dt))
        return false;

    
    double newDate = MakeDate(MakeDay(y, m, dt), TimeWithinDay(t));

    
    double v = TimeClip(newDate);

    
    SetUTCTime(thisObj, v, args.rval().address());
    return true;
}

static JSBool
date_setUTCFullYear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCFullYear_impl>(cx, args);
}


JS_ALWAYS_INLINE bool
date_setYear_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    
    double t = ThisLocalTimeOrZero(thisObj, &cx->runtime->dateTimeInfo);

    
    double y;
    if (!ToNumber(cx, args.length() > 0 ? args[0] : UndefinedValue(), &y))
        return false;

    
    if (MOZ_DOUBLE_IS_NaN(y)) {
        SetUTCTime(thisObj, js_NaN, args.rval().address());
        return true;
    }

    
    double yint = ToInteger(y);
    if (0 <= yint && yint <= 99)
        yint += 1900;

    
    double day = MakeDay(yint, MonthFromTime(t), DateFromTime(t));

    
    double u = UTC(MakeDate(day, TimeWithinDay(t)), &cx->runtime->dateTimeInfo);

    
    SetUTCTime(thisObj, TimeClip(u), args.rval().address());
    return true;
}

static JSBool
date_setYear(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setYear_impl>(cx, args);
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
    JS_ASSERT(TimeClip(utctime) == utctime);
    JS_snprintf(buf, size, "%s, %.2d %s %.4d %.2d:%.2d:%.2d GMT",
                days[int(WeekDay(utctime))],
                int(DateFromTime(utctime)),
                months[int(MonthFromTime(utctime))],
                int(YearFromTime(utctime)),
                int(HourFromTime(utctime)),
                int(MinFromTime(utctime)),
                int(SecFromTime(utctime)));
}

static void
print_iso_string(char* buf, size_t size, double utctime)
{
    JS_ASSERT(TimeClip(utctime) == utctime);
    JS_snprintf(buf, size, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3dZ",
                int(YearFromTime(utctime)),
                int(MonthFromTime(utctime)) + 1,
                int(DateFromTime(utctime)),
                int(HourFromTime(utctime)),
                int(MinFromTime(utctime)),
                int(SecFromTime(utctime)),
                int(msFromTime(utctime)));
}


JS_ALWAYS_INLINE bool
date_toGMTString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    double utctime = args.thisv().toObject().getDateUTCTime().toNumber();

    char buf[100];
    if (!MOZ_DOUBLE_IS_FINITE(utctime))
        JS_snprintf(buf, sizeof buf, js_NaN_date_str);
    else
        print_gmt_string(buf, sizeof buf, utctime);

    RawString str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}


static JSBool
date_toGMTString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toGMTString_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_toISOString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    double utctime = args.thisv().toObject().getDateUTCTime().toNumber();
    if (!MOZ_DOUBLE_IS_FINITE(utctime)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_INVALID_DATE);
        return false;
    }

    char buf[100];
    print_iso_string(buf, sizeof buf, utctime);

    RawString str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;

}

static JSBool
date_toISOString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toISOString_impl>(cx, args);
}


static JSBool
date_toJSON(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    
    RootedValue tv(cx, ObjectValue(*obj));
    if (!ToPrimitive(cx, JSTYPE_NUMBER, &tv))
        return false;

    
    if (tv.isDouble() && !MOZ_DOUBLE_IS_FINITE(tv.toDouble())) {
        args.rval().setNull();
        return true;
    }

    
    RootedValue toISO(cx);
    if (!JSObject::getProperty(cx, obj, obj, cx->names().toISOString, &toISO))
        return false;

    
    if (!js_IsCallable(toISO)) {
        JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_TOISOSTRING_PROP);
        return false;
    }

    
    InvokeArgsGuard ag;
    if (!cx->stack.pushInvokeArgs(cx, 0, &ag))
        return false;

    ag.setCallee(toISO);
    ag.setThis(ObjectValue(*obj));

    if (!Invoke(cx, ag))
        return false;
    args.rval().set(ag.rval());
    return true;
}



static void
new_explode(double timeval, PRMJTime *split, DateTimeInfo *dtInfo)
{
    double year = YearFromTime(timeval);

    split->tm_usec = int32_t(msFromTime(timeval)) * 1000;
    split->tm_sec = int8_t(SecFromTime(timeval));
    split->tm_min = int8_t(MinFromTime(timeval));
    split->tm_hour = int8_t(HourFromTime(timeval));
    split->tm_mday = int8_t(DateFromTime(timeval));
    split->tm_mon = int8_t(MonthFromTime(timeval));
    split->tm_wday = int8_t(WeekDay(timeval));
    split->tm_year = year;
    split->tm_yday = int16_t(DayWithinYear(timeval, year));

    

    split->tm_isdst = (DaylightSavingTA(timeval, dtInfo) != 0);
}

typedef enum formatspec {
    FORMATSPEC_FULL, FORMATSPEC_DATE, FORMATSPEC_TIME
} formatspec;


static JSBool
date_format(JSContext *cx, double date, formatspec format, MutableHandleValue rval)
{
    char buf[100];
    char tzbuf[100];
    JSBool usetz;
    size_t i, tzlen;
    PRMJTime split;

    if (!MOZ_DOUBLE_IS_FINITE(date)) {
        JS_snprintf(buf, sizeof buf, js_NaN_date_str);
    } else {
        JS_ASSERT(TimeClip(date) == date);

        double local = LocalTime(date, &cx->runtime->dateTimeInfo);

        

        int minutes = (int) floor(AdjustTime(date, &cx->runtime->dateTimeInfo) / msPerMinute);

        
        int offset = (minutes / 60) * 100 + minutes % 60;

        








        

        new_explode(date, &split, &cx->runtime->dateTimeInfo);
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
                        days[int(WeekDay(local))],
                        months[int(MonthFromTime(local))],
                        int(DateFromTime(local)),
                        int(YearFromTime(local)),
                        int(HourFromTime(local)),
                        int(MinFromTime(local)),
                        int(SecFromTime(local)),
                        offset,
                        usetz ? " " : "",
                        usetz ? tzbuf : "");
            break;
          case FORMATSPEC_DATE:
            
            JS_snprintf(buf, sizeof buf,
                        "%s %s %.2d %.4d",
                        days[int(WeekDay(local))],
                        months[int(MonthFromTime(local))],
                        int(DateFromTime(local)),
                        int(YearFromTime(local)));
            break;
          case FORMATSPEC_TIME:
            
            JS_snprintf(buf, sizeof buf,
                        "%.2d:%.2d:%.2d GMT%+.4d%s%s",
                        int(HourFromTime(local)),
                        int(MinFromTime(local)),
                        int(SecFromTime(local)),
                        offset,
                        usetz ? " " : "",
                        usetz ? tzbuf : "");
            break;
        }
    }

    RawString str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return false;
    rval.setString(str);
    return true;
}

static bool
ToLocaleFormatHelper(JSContext *cx, HandleObject obj, const char *format, MutableHandleValue rval)
{
    double utctime = obj->getDateUTCTime().toNumber();

    char buf[100];
    if (!MOZ_DOUBLE_IS_FINITE(utctime)) {
        JS_snprintf(buf, sizeof buf, js_NaN_date_str);
    } else {
        int result_len;
        double local = LocalTime(utctime, &cx->runtime->dateTimeInfo);
        PRMJTime split;
        new_explode(local, &split, &cx->runtime->dateTimeInfo);

        
        result_len = PRMJ_FormatTime(buf, sizeof buf, format, &split);

        
        if (result_len == 0)
            return date_format(cx, utctime, FORMATSPEC_FULL, rval);

        
        if (strcmp(format, "%x") == 0 && result_len >= 6 &&
            

            !isdigit(buf[result_len - 3]) &&
            isdigit(buf[result_len - 2]) && isdigit(buf[result_len - 1]) &&
            
            !(isdigit(buf[0]) && isdigit(buf[1]) &&
              isdigit(buf[2]) && isdigit(buf[3]))) {
            JS_snprintf(buf + (result_len - 2), (sizeof buf) - (result_len - 2),
                        "%d", js_DateGetYear(cx, obj));
        }

    }

    if (cx->runtime->localeCallbacks && cx->runtime->localeCallbacks->localeToUnicode)
        return cx->runtime->localeCallbacks->localeToUnicode(cx, buf, rval.address());

    RawString str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return false;
    rval.setString(str);
    return true;
}

#if !ENABLE_INTL_API
static bool
ToLocaleStringHelper(JSContext *cx, HandleObject thisObj, MutableHandleValue rval)
{
    



    return ToLocaleFormatHelper(cx, thisObj,
#if defined(_WIN32) && !defined(__MWERKS__)
                          "%#c"
#else
                          "%c"
#endif
                         , rval);
}


JS_ALWAYS_INLINE bool
date_toLocaleString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());
    return ToLocaleStringHelper(cx, thisObj, args.rval());
}

static JSBool
date_toLocaleString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toLocaleString_impl>(cx, args);
}


JS_ALWAYS_INLINE bool
date_toLocaleDateString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    



    static const char format[] =
#if defined(_WIN32) && !defined(__MWERKS__)
                                   "%#x"
#else
                                   "%x"
#endif
                                   ;

    RootedObject thisObj(cx, &args.thisv().toObject());
    return ToLocaleFormatHelper(cx, thisObj, format, args.rval());
}

static JSBool
date_toLocaleDateString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toLocaleDateString_impl>(cx, args);
}


JS_ALWAYS_INLINE bool
date_toLocaleTimeString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());
    return ToLocaleFormatHelper(cx, thisObj, "%X", args.rval());
}

static JSBool
date_toLocaleTimeString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toLocaleTimeString_impl>(cx, args);
}
#endif

JS_ALWAYS_INLINE bool
date_toLocaleFormat_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RootedObject thisObj(cx, &args.thisv().toObject());

    if (args.length() == 0) {
        



        return ToLocaleFormatHelper(cx, thisObj,
#if defined(_WIN32) && !defined(__MWERKS__)
                              "%#c"
#else
                              "%c"
#endif
                             , args.rval());
    }

    RootedString fmt(cx, ToString<CanGC>(cx, args[0]));
    if (!fmt)
        return false;

    JSAutoByteString fmtbytes(cx, fmt);
    if (!fmtbytes)
        return false;

    return ToLocaleFormatHelper(cx, thisObj, fmtbytes.ptr(), args.rval());
}

static JSBool
date_toLocaleFormat(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toLocaleFormat_impl>(cx, args);
}


JS_ALWAYS_INLINE bool
date_toTimeString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    return date_format(cx, args.thisv().toObject().getDateUTCTime().toNumber(),
                       FORMATSPEC_TIME, args.rval());
}

static JSBool
date_toTimeString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toTimeString_impl>(cx, args);
}


JS_ALWAYS_INLINE bool
date_toDateString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    return date_format(cx, args.thisv().toObject().getDateUTCTime().toNumber(),
                       FORMATSPEC_DATE, args.rval());
}

static JSBool
date_toDateString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toDateString_impl>(cx, args);
}

#if JS_HAS_TOSOURCE
JS_ALWAYS_INLINE bool
date_toSource_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    StringBuffer sb(cx);
    if (!sb.append("(new Date(") ||
        !NumberValueToStringBuffer(cx, args.thisv().toObject().getDateUTCTime(), sb) ||
        !sb.append("))"))
    {
        return false;
    }

    RawString str = sb.finishString();
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static JSBool
date_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toSource_impl>(cx, args);
}
#endif

JS_ALWAYS_INLINE bool
date_toString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));
    return date_format(cx, args.thisv().toObject().getDateUTCTime().toNumber(),
                       FORMATSPEC_FULL, args.rval());
}

static JSBool
date_toString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toString_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
date_valueOf_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsDate(args.thisv()));

    RawObject thisObj = &args.thisv().toObject();

    args.rval().set(thisObj->getDateUTCTime());
    return true;
}

static JSBool
date_valueOf(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_valueOf_impl>(cx, args);
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

    
    
#if ENABLE_INTL_API
         {js_toLocaleString_str, {NULL, NULL},            0,0, "Date_toLocaleString"},
         {"toLocaleDateString",  {NULL, NULL},            0,0, "Date_toLocaleDateString"},
         {"toLocaleTimeString",  {NULL, NULL},            0,0, "Date_toLocaleTimeString"},
#else
    JS_FN(js_toLocaleString_str, date_toLocaleString,     0,0),
    JS_FN("toLocaleDateString",  date_toLocaleDateString, 0,0),
    JS_FN("toLocaleTimeString",  date_toLocaleTimeString, 0,0),
#endif

    JS_FS_END
};

JSBool
js_Date(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    if (!IsConstructing(args))
        return date_format(cx, NowAsMillis(), FORMATSPEC_FULL, args.rval());

    
    double d;
    if (args.length() == 0) {
        
        d = NowAsMillis();
    } else if (args.length() == 1) {
        

        
        if (!ToPrimitive(cx, args.handleAt(0)))
            return false;

        if (args[0].isString()) {
            
            RawString str = args[0].toString();
            if (!str)
                return false;

            RawLinearString linearStr = str->ensureLinear(cx);
            if (!linearStr)
                return false;

            if (!date_parseString(linearStr, &d, &cx->runtime->dateTimeInfo))
                d = js_NaN;
            else
                d = TimeClip(d);
        } else {
            
            if (!ToNumber(cx, args[0], &d))
                return false;
            d = TimeClip(d);
        }
    } else {
        double msec_time;
        if (!date_msecFromArgs(cx, args, &msec_time))
            return false;

        if (MOZ_DOUBLE_IS_FINITE(msec_time)) {
            msec_time = UTC(msec_time, &cx->runtime->dateTimeInfo);
            msec_time = TimeClip(msec_time);
        }
        d = msec_time;
    }

    RawObject obj = js_NewDateObjectMsec(cx, d);
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

JSObject *
js_InitDateClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());

    Rooted<GlobalObject*> global(cx, &obj->asGlobal());

    RootedObject dateProto(cx, global->createBlankPrototype(cx, &DateClass));
    if (!dateProto)
        return NULL;
    SetUTCTime(dateProto, js_NaN);

    RootedFunction ctor(cx);
    ctor = global->createConstructor(cx, js_Date, cx->names().Date, MAXARGS);
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, dateProto))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, ctor, NULL, date_static_methods))
        return NULL;

    




    if (!JS_DefineFunctions(cx, dateProto, date_methods))
        return NULL;
    RootedValue toUTCStringFun(cx);
    RootedId toUTCStringId(cx, NameToId(cx->names().toUTCString));
    RootedId toGMTStringId(cx, NameToId(cx->names().toGMTString));
    if (!baseops::GetProperty(cx, dateProto, toUTCStringId, &toUTCStringFun) ||
        !baseops::DefineGeneric(cx, dateProto, toGMTStringId, toUTCStringFun,
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
    RawObject obj = NewBuiltinClassInstance(cx, &DateClass);
    if (!obj)
        return NULL;
    SetUTCTime(obj, msec_time);
    return obj;
}

JS_FRIEND_API(JSObject *)
js_NewDateObject(JSContext *cx, int year, int mon, int mday,
                 int hour, int min, int sec)
{
    JS_ASSERT(mon < 12);
    double msec_time = date_msecFromDate(year, mon, mday, hour, min, sec, 0);
    return js_NewDateObjectMsec(cx, UTC(msec_time, &cx->runtime->dateTimeInfo));
}

JS_FRIEND_API(JSBool)
js_DateIsValid(RawObject obj)
{
    return obj->isDate() && !MOZ_DOUBLE_IS_NaN(obj->getDateUTCTime().toNumber());
}

JS_FRIEND_API(int)
js_DateGetYear(JSContext *cx, RawObject obj)
{
    
    JS_ASSERT(obj);
    double localtime = GetCachedLocalTime(&cx->runtime->dateTimeInfo, obj);
    if (MOZ_DOUBLE_IS_NaN(localtime))
        return 0;

    return (int) YearFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetMonth(JSContext *cx, RawObject obj)
{
    JS_ASSERT(obj);
    double localtime = GetCachedLocalTime(&cx->runtime->dateTimeInfo, obj);
    if (MOZ_DOUBLE_IS_NaN(localtime))
        return 0;

    return (int) MonthFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetDate(JSContext *cx, RawObject obj)
{
    JS_ASSERT(obj);
    double localtime = GetCachedLocalTime(&cx->runtime->dateTimeInfo, obj);
    if (MOZ_DOUBLE_IS_NaN(localtime))
        return 0;

    return (int) DateFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetHours(JSContext *cx, RawObject obj)
{
    JS_ASSERT(obj);
    double localtime = GetCachedLocalTime(&cx->runtime->dateTimeInfo, obj);
    if (MOZ_DOUBLE_IS_NaN(localtime))
        return 0;

    return (int) HourFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetMinutes(JSContext *cx, RawObject obj)
{
    JS_ASSERT(obj);
    double localtime = GetCachedLocalTime(&cx->runtime->dateTimeInfo, obj);
    if (MOZ_DOUBLE_IS_NaN(localtime))
        return 0;

    return (int) MinFromTime(localtime);
}

JS_FRIEND_API(int)
js_DateGetSeconds(RawObject obj)
{
    if (!obj->isDate())
        return 0;

    double utctime = obj->getDateUTCTime().toNumber();
    if (MOZ_DOUBLE_IS_NaN(utctime))
        return 0;
    return (int) SecFromTime(utctime);
}

JS_FRIEND_API(double)
js_DateGetMsecSinceEpoch(RawObject obj)
{
    return obj->isDate() ? obj->getDateUTCTime().toNumber() : 0;
}


static const NativeImpl sReadOnlyDateMethods[] = {
    date_getTime_impl,
    date_getYear_impl,
    date_getFullYear_impl,
    date_getUTCFullYear_impl,
    date_getMonth_impl,
    date_getUTCMonth_impl,
    date_getDate_impl,
    date_getUTCDate_impl,
    date_getDay_impl,
    date_getUTCDay_impl,
    date_getHours_impl,
    date_getUTCHours_impl,
    date_getMinutes_impl,
    date_getUTCMinutes_impl,
    date_getUTCSeconds_impl,
    date_getUTCMilliseconds_impl,
    date_getTimezoneOffset_impl,
    date_toGMTString_impl,
    date_toISOString_impl,
    date_toLocaleFormat_impl,
    date_toTimeString_impl,
    date_toDateString_impl,
    date_toSource_impl,
    date_toString_impl,
    date_valueOf_impl
};

JS_FRIEND_API(bool)
js::IsReadOnlyDateMethod(IsAcceptableThis test, NativeImpl method)
{
    
    if (test != IsDate)
        return false;

    
    unsigned max = sizeof(sReadOnlyDateMethods) / sizeof(sReadOnlyDateMethods[0]);
    for (unsigned i = 0; i < max; ++i) {
        if (method == sReadOnlyDateMethods[i])
            return true;
    }
    return false;
}
