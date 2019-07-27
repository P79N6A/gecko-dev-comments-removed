
















#include "jsdate.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/FloatingPoint.h"

#include <ctype.h>
#include <math.h>
#include <string.h>

#include "jsapi.h"
#include "jscntxt.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsprf.h"
#include "jsstr.h"
#include "jstypes.h"
#include "jsutil.h"
#include "jswrapper.h"
#include "prmjtime.h"

#include "js/Conversions.h"
#include "js/Date.h"
#include "vm/DateTime.h"
#include "vm/GlobalObject.h"
#include "vm/Interpreter.h"
#include "vm/String.h"
#include "vm/StringBuffer.h"

#include "jsobjinlines.h"

using namespace js;

using mozilla::ArrayLength;
using mozilla::IsFinite;
using mozilla::IsNaN;

using JS::AutoCheckCannotGC;
using JS::GenericNaN;
using JS::ToInteger;

















































static inline double
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


static inline bool
IsLeapYear(double year)
{
    MOZ_ASSERT(ToInteger(year) == year);
    return fmod(year, 4) == 0 && (fmod(year, 100) != 0 || fmod(year, 400) == 0);
}

static inline double
DaysInYear(double year)
{
    if (!IsFinite(year))
        return GenericNaN();
    return IsLeapYear(year) ? 366 : 365;
}

static inline double
DayFromYear(double y)
{
    return 365 * (y - 1970) +
           floor((y - 1969) / 4.0) -
           floor((y - 1901) / 100.0) +
           floor((y - 1601) / 400.0);
}

static inline double
TimeFromYear(double y)
{
    return DayFromYear(y) * msPerDay;
}

static double
YearFromTime(double t)
{
    if (!IsFinite(t))
        return GenericNaN();

    MOZ_ASSERT(ToInteger(t) == t);

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

static inline int
DaysInFebruary(double year)
{
    return IsLeapYear(year) ? 29 : 28;
}


static inline double
DayWithinYear(double t, double year)
{
    MOZ_ASSERT_IF(IsFinite(t), YearFromTime(t) == year);
    return Day(t) - DayFromYear(year);
}

static double
MonthFromTime(double t)
{
    if (!IsFinite(t))
        return GenericNaN();

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
    if (!IsFinite(t))
        return GenericNaN();

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
    



    MOZ_ASSERT(ToInteger(t) == t);
    int result = (int(Day(t)) + 4) % 7;
    if (result < 0)
        result += 7;
    return result;
}

static inline int
DayFromMonth(int month, bool isLeapYear)
{
    



    static const int firstDayOfMonth[2][13] = {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
    };

    MOZ_ASSERT(0 <= month && month <= 12);
    return firstDayOfMonth[isLeapYear][month];
}

template<typename T>
static inline int
DayFromMonth(T month, bool isLeapYear) = delete;


static double
MakeDay(double year, double month, double date)
{
    
    if (!IsFinite(year) || !IsFinite(month) || !IsFinite(date))
        return GenericNaN();

    
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


static inline double
MakeDate(double day, double time)
{
    
    if (!IsFinite(day) || !IsFinite(time))
        return GenericNaN();

    
    return day * msPerDay + time;
}

JS_PUBLIC_API(double)
JS::MakeDate(double year, unsigned month, unsigned day)
{
    return TimeClip(::MakeDate(MakeDay(year, month, day), 0));
}

JS_PUBLIC_API(double)
JS::YearFromTime(double time)
{
    return ::YearFromTime(time);
}

JS_PUBLIC_API(double)
JS::MonthFromTime(double time)
{
    return ::MonthFromTime(time);
}

JS_PUBLIC_API(double)
JS::DayFromTime(double time)
{
    return DateFromTime(time);
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
DaylightSavingTA(double t, DateTimeInfo* dtInfo)
{
    if (!IsFinite(t))
        return GenericNaN();

    



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
AdjustTime(double date, DateTimeInfo* dtInfo)
{
    double t = DaylightSavingTA(date, dtInfo) + dtInfo->localTZA();
    t = (dtInfo->localTZA() >= 0) ? fmod(t, msPerDay) : -fmod(msPerDay - t, msPerDay);
    return t;
}


static double
LocalTime(double t, DateTimeInfo* dtInfo)
{
    return t + AdjustTime(t, dtInfo);
}

static double
UTC(double t, DateTimeInfo* dtInfo)
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
    
    if (!IsFinite(hour) ||
        !IsFinite(min) ||
        !IsFinite(sec) ||
        !IsFinite(ms))
    {
        return GenericNaN();
    }

    
    double h = ToInteger(hour);

    
    double m = ToInteger(min);

    
    double s = ToInteger(sec);

    
    double milli = ToInteger(ms);

    
    return h * msPerHour + m * msPerMinute + s * msPerSecond + milli;
}





static bool
date_convert(JSContext* cx, HandleObject obj, JSType hint, MutableHandleValue vp)
{
    MOZ_ASSERT(hint == JSTYPE_NUMBER || hint == JSTYPE_STRING || hint == JSTYPE_VOID);
    MOZ_ASSERT(obj->is<DateObject>());

    return JS::OrdinaryToPrimitive(cx, obj, hint == JSTYPE_VOID ? JSTYPE_STRING : hint, vp);
}



static const char* const wtb[] = {
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

static const int ttb[] = {
    -1, -2, 0, 0, 0, 0, 0, 0, 0,       
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
    10000 + 0, 10000 + 0, 10000 + 0,   
    10000 + 5 * 60, 10000 + 4 * 60,    
    10000 + 6 * 60, 10000 + 5 * 60,    
    10000 + 7 * 60, 10000 + 6 * 60,    
    10000 + 8 * 60, 10000 + 7 * 60     
};

template <typename CharT>
static bool
RegionMatches(const char* s1, int s1off, const CharT* s2, int s2off, int count)
{
    while (count > 0 && s1[s1off] && s2[s2off]) {
        if (unicode::ToLowerCase(s1[s1off]) != unicode::ToLowerCase(s2[s2off]))
            break;

        s1off++;
        s2off++;
        count--;
    }

    return count == 0;
}


static double
date_msecFromDate(double year, double mon, double mday, double hour,
                  double min, double sec, double msec)
{
    return MakeDate(MakeDay(year, mon, mday), MakeTime(hour, min, sec, msec));
}


#define MAXARGS        7

static bool
date_msecFromArgs(JSContext* cx, CallArgs args, double* rval)
{
    unsigned loop;
    double array[MAXARGS];
    double msec_time;

    for (loop = 0; loop < MAXARGS; loop++) {
        if (loop < args.length()) {
            double d;
            if (!ToNumber(cx, args[loop], &d))
                return false;
            
            if (!IsFinite(d)) {
                *rval = GenericNaN();
                return true;
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
    return true;
}




static bool
date_UTC(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    double msec_time;
    if (!date_msecFromArgs(cx, args, &msec_time))
        return false;

    msec_time = TimeClip(msec_time);

    args.rval().setNumber(msec_time);
    return true;
}








template <typename CharT>
static bool
ParseDigits(size_t* result, const CharT* s, size_t* i, size_t limit)
{
    size_t init = *i;
    *result = 0;
    while (*i < limit && ('0' <= s[*i] && s[*i] <= '9')) {
        *result *= 10;
        *result += (s[*i] - '0');
        ++(*i);
    }
    return *i != init;
}









template <typename CharT>
static bool
ParseFractional(double* result, const CharT* s, size_t* i, size_t limit)
{
    double factor = 0.1;
    size_t init = *i;
    *result = 0.0;
    while (*i < limit && ('0' <= s[*i] && s[*i] <= '9')) {
        *result += (s[*i] - '0') * factor;
        factor *= 0.1;
        ++(*i);
    }
    return *i != init;
}








template <typename CharT>
static bool
ParseDigitsN(size_t n, size_t* result, const CharT* s, size_t* i, size_t limit)
{
    size_t init = *i;

    if (ParseDigits(result, s, i, Min(limit, init + n)))
        return (*i - init) == n;

    *i = init;
    return false;
}

static int
DaysInMonth(int year, int month)
{
    bool leap = IsLeapYear(year);
    int result = int(DayFromMonth(month, leap) - DayFromMonth(month - 1, leap));
    return result;
}
























































template <typename CharT>
static bool
ParseISODate(const CharT* s, size_t length, double* result, DateTimeInfo* dtInfo)
{
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
    bool isLocalTime = false;
    size_t tzHour = 0;
    size_t tzMin = 0;

#define PEEK(ch) (i < length && s[i] == ch)

#define NEED(ch)                                                               \
    if (i >= length || s[i] != ch) { return false; } else { ++i; }

#define DONE_DATE_UNLESS(ch)                                                   \
    if (i >= length || s[i] != ch) { goto done_date; } else { ++i; }

#define DONE_UNLESS(ch)                                                        \
    if (i >= length || s[i] != ch) { goto done; } else { ++i; }

#define NEED_NDIGITS(n, field)                                                 \
    if (!ParseDigitsN(n, &field, s, &i, length)) { return false; }

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
            if (!ParseFractional(&frac, s, &i, length))
                return false;
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
        isLocalTime = true;
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
    {
        return false;
    }

    if (i != length)
        return false;

    month -= 1; 

    double msec = date_msecFromDate(dateMul * double(year), month, day,
                                    hour, min, sec, frac * 1000.0);

    if (isLocalTime)
        msec = UTC(msec, dtInfo);
    else
        msec -= tzMul * (tzHour * msPerHour + tzMin * msPerMinute);

    if (msec < -8.64e15 || msec > 8.64e15)
        return false;

    *result = msec;
    return true;

#undef PEEK
#undef NEED
#undef DONE_UNLESS
#undef NEED_NDIGITS
}

template <typename CharT>
static bool
ParseDate(const CharT* s, size_t length, double* result, DateTimeInfo* dtInfo)
{
    if (ParseISODate(s, length, result, dtInfo))
        return true;

    if (length == 0)
        return false;

    int year = -1;
    int mon = -1;
    int mday = -1;
    int hour = -1;
    int min = -1;
    int sec = -1;
    int tzOffset = -1;

    int prevc = 0;

    bool seenPlusMinus = false;
    bool seenMonthName = false;

    size_t i = 0;
    while (i < length) {
        int c = s[i];
        i++;
        if (c <= ' ' || c == ',' || c == '-') {
            if (c == '-' && '0' <= s[i] && s[i] <= '9')
                prevc = c;
            continue;
        }
        if (c == '(') { 
            int depth = 1;
            while (i < length) {
                c = s[i];
                i++;
                if (c == '(') {
                    depth++;
                } else if (c == ')') {
                    if (--depth <= 0)
                        break;
                }
            }
            continue;
        }
        if ('0' <= c && c <= '9') {
            int n = c - '0';
            while (i < length && '0' <= (c = s[i]) && c <= '9') {
                n = n * 10 + c - '0';
                i++;
            }

            







            if ((prevc == '+' || prevc == '-')) {
                
                seenPlusMinus = true;

                
                if (n < 24)
                    n = n * 60; 
                else
                    n = n % 100 + n / 100 * 60; 

                if (prevc == '+')       
                    n = -n;

                if (tzOffset != 0 && tzOffset != -1)
                    return false;

                tzOffset = n;
            } else if (prevc == '/' && mon >= 0 && mday >= 0 && year < 0) {
                if (c <= ' ' || c == ',' || c == '/' || i >= length)
                    year = n;
                else
                    return false;
            } else if (c == ':') {
                if (hour < 0)
                    hour =  n;
                else if (min < 0)
                    min =  n;
                else
                    return false;
            } else if (c == '/') {
                



                if (mon < 0)
                    mon =  n;
                else if (mday < 0)
                    mday =  n;
                else
                    return false;
            } else if (i < length && c != ',' && c > ' ' && c != '-' && c != '(') {
                return false;
            } else if (seenPlusMinus && n < 60) {  
                if (tzOffset < 0)
                    tzOffset -= n;
                else
                    tzOffset += n;
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
                return false;
            }
            prevc = 0;
        } else if (c == '/' || c == ':' || c == '+' || c == '-') {
            prevc = c;
        } else {
            size_t st = i - 1;
            int k;
            while (i < length) {
                c = s[i];
                if (!(('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')))
                    break;
                i++;
            }

            if (i <= st + 1)
                return false;

            for (k = ArrayLength(wtb); --k >= 0;) {
                if (RegionMatches(wtb[k], 0, s, st, i - st)) {
                    int action = ttb[k];
                    if (action != 0) {
                        if (action < 0) {
                            



                            MOZ_ASSERT(action == -1 || action == -2);
                            if (hour > 12 || hour < 0)
                                return false;

                            if (action == -1 && hour == 12) 
                                hour = 0;
                            else if (action == -2 && hour != 12) 
                                hour += 12;
                        } else if (action <= 13) { 
                            



                            if (seenMonthName)
                                return false;

                            seenMonthName = true;
                            int temp =  (action - 2) + 1;

                            if (mon < 0) {
                                mon = temp;
                            } else if (mday < 0) {
                                mday = mon;
                                mon = temp;
                            } else if (year < 0) {
                                year = mon;
                                mon = temp;
                            } else {
                                return false;
                            }
                        } else {
                            tzOffset = action - 10000;
                        }
                    }
                    break;
                }
            }

            if (k < 0)
                return false;

            prevc = 0;
        }
    }

    if (year < 0 || mon < 0 || mday < 0)
        return false;

    

























    if (seenMonthName) {
        if ((mday >= 70 && year >= 70) || (mday < 70 && year < 70))
            return false;

        if (mday > year) {
            int temp = year;
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
            int temp = year;
            year = mon + 1900;
            mon = mday;
            mday = temp;
        } else {
            return false;
        }
    } else { 
        if (mday < 70) {
            int temp = year;
            year = mon;
            mon = mday;
            mday = temp;
        } else {
            return false;
        }
    }

    mon -= 1; 
    if (sec < 0)
        sec = 0;
    if (min < 0)
        min = 0;
    if (hour < 0)
        hour = 0;

    double msec = date_msecFromDate(year, mon, mday, hour, min, sec, 0);

    if (tzOffset == -1) 
        msec = UTC(msec, dtInfo);
    else
        msec += tzOffset * msPerMinute;

    *result = msec;
    return true;
}

static bool
ParseDate(JSLinearString* s, double* result, DateTimeInfo* dtInfo)
{
    AutoCheckCannotGC nogc;
    return s->hasLatin1Chars()
           ? ParseDate(s->latin1Chars(nogc), s->length(), result, dtInfo)
           : ParseDate(s->twoByteChars(nogc), s->length(), result, dtInfo);
}

static bool
date_parse(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() == 0) {
        args.rval().setNaN();
        return true;
    }

    JSString* str = ToString<CanGC>(cx, args[0]);
    if (!str)
        return false;

    JSLinearString* linearStr = str->ensureLinear(cx);
    if (!linearStr)
        return false;

    double result;
    if (!ParseDate(linearStr, &result, &cx->runtime()->dateTimeInfo)) {
        args.rval().setNaN();
        return true;
    }

    result = TimeClip(result);
    args.rval().setNumber(result);
    return true;
}

static inline double
NowAsMillis()
{
    return (double) (PRMJ_Now() / PRMJ_USEC_PER_MSEC);
}

bool
js::date_now(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setDouble(NowAsMillis());
    return true;
}

void
DateObject::setUTCTime(double t)
{
    for (size_t ind = COMPONENTS_START_SLOT; ind < RESERVED_SLOTS; ind++)
        setReservedSlot(ind, UndefinedValue());

    setFixedSlot(UTC_TIME_SLOT, DoubleValue(t));
}

void
DateObject::setUTCTime(double t, MutableHandleValue vp)
{
    setUTCTime(t);
    vp.setDouble(t);
}

void
DateObject::fillLocalTimeSlots(DateTimeInfo* dtInfo)
{
    
    if (!getReservedSlot(LOCAL_TIME_SLOT).isUndefined() &&
        getReservedSlot(TZA_SLOT).toDouble() == dtInfo->localTZA())
    {
        return;
    }

    
    setReservedSlot(TZA_SLOT, DoubleValue(dtInfo->localTZA()));

    double utcTime = UTCTime().toNumber();

    if (!IsFinite(utcTime)) {
        for (size_t ind = COMPONENTS_START_SLOT; ind < RESERVED_SLOTS; ind++)
            setReservedSlot(ind, DoubleValue(utcTime));
        return;
    }

    double localTime = LocalTime(utcTime, dtInfo);

    setReservedSlot(LOCAL_TIME_SLOT, DoubleValue(localTime));

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

    setReservedSlot(LOCAL_YEAR_SLOT, Int32Value(year));

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

    setReservedSlot(LOCAL_MONTH_SLOT, Int32Value(month));
    setReservedSlot(LOCAL_DATE_SLOT, Int32Value(day - step));

    int weekday = WeekDay(localTime);
    setReservedSlot(LOCAL_DAY_SLOT, Int32Value(weekday));

    int seconds = yearSeconds % 60;
    setReservedSlot(LOCAL_SECONDS_SLOT, Int32Value(seconds));

    int minutes = (yearSeconds / 60) % 60;
    setReservedSlot(LOCAL_MINUTES_SLOT, Int32Value(minutes));

    int hours = (yearSeconds / (60 * 60)) % 24;
    setReservedSlot(LOCAL_HOURS_SLOT, Int32Value(hours));
}

inline double
DateObject::cachedLocalTime(DateTimeInfo* dtInfo)
{
    fillLocalTimeSlots(dtInfo);
    return getReservedSlot(LOCAL_TIME_SLOT).toDouble();
}

MOZ_ALWAYS_INLINE bool
IsDate(HandleValue v)
{
    return v.isObject() && v.toObject().is<DateObject>();
}




 MOZ_ALWAYS_INLINE bool
DateObject::getTime_impl(JSContext* cx, CallArgs args)
{
    args.rval().set(args.thisv().toObject().as<DateObject>().UTCTime());
    return true;
}

static bool
date_getTime(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getTime_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getYear_impl(JSContext* cx, CallArgs args)
{
    DateObject* dateObj = &args.thisv().toObject().as<DateObject>();
    dateObj->fillLocalTimeSlots(&cx->runtime()->dateTimeInfo);

    Value yearVal = dateObj->getReservedSlot(LOCAL_YEAR_SLOT);
    if (yearVal.isInt32()) {
        
        int year = yearVal.toInt32() - 1900;
        args.rval().setInt32(year);
    } else {
        args.rval().set(yearVal);
    }

    return true;
}

static bool
date_getYear(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getYear_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getFullYear_impl(JSContext* cx, CallArgs args)
{
    DateObject* dateObj = &args.thisv().toObject().as<DateObject>();
    dateObj->fillLocalTimeSlots(&cx->runtime()->dateTimeInfo);

    args.rval().set(dateObj->getReservedSlot(LOCAL_YEAR_SLOT));
    return true;
}

static bool
date_getFullYear(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getFullYear_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getUTCFullYear_impl(JSContext* cx, CallArgs args)
{
    double result = args.thisv().toObject().as<DateObject>().UTCTime().toNumber();
    if (IsFinite(result))
        result = YearFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static bool
date_getUTCFullYear(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getUTCFullYear_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getMonth_impl(JSContext* cx, CallArgs args)
{
    DateObject* dateObj = &args.thisv().toObject().as<DateObject>();
    dateObj->fillLocalTimeSlots(&cx->runtime()->dateTimeInfo);

    args.rval().set(dateObj->getReservedSlot(LOCAL_MONTH_SLOT));
    return true;
}

static bool
date_getMonth(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getMonth_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getUTCMonth_impl(JSContext* cx, CallArgs args)
{
    double d = args.thisv().toObject().as<DateObject>().UTCTime().toNumber();
    args.rval().setNumber(MonthFromTime(d));
    return true;
}

static bool
date_getUTCMonth(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getUTCMonth_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getDate_impl(JSContext* cx, CallArgs args)
{
    DateObject* dateObj = &args.thisv().toObject().as<DateObject>();
    dateObj->fillLocalTimeSlots(&cx->runtime()->dateTimeInfo);

    args.rval().set(dateObj->getReservedSlot(LOCAL_DATE_SLOT));
    return true;
}

static bool
date_getDate(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getDate_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getUTCDate_impl(JSContext* cx, CallArgs args)
{
    double result = args.thisv().toObject().as<DateObject>().UTCTime().toNumber();
    if (IsFinite(result))
        result = DateFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static bool
date_getUTCDate(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getUTCDate_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getDay_impl(JSContext* cx, CallArgs args)
{
    DateObject* dateObj = &args.thisv().toObject().as<DateObject>();
    dateObj->fillLocalTimeSlots(&cx->runtime()->dateTimeInfo);

    args.rval().set(dateObj->getReservedSlot(LOCAL_DAY_SLOT));
    return true;
}

static bool
date_getDay(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getDay_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getUTCDay_impl(JSContext* cx, CallArgs args)
{
    double result = args.thisv().toObject().as<DateObject>().UTCTime().toNumber();
    if (IsFinite(result))
        result = WeekDay(result);

    args.rval().setNumber(result);
    return true;
}

static bool
date_getUTCDay(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getUTCDay_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getHours_impl(JSContext* cx, CallArgs args)
{
    DateObject* dateObj = &args.thisv().toObject().as<DateObject>();
    dateObj->fillLocalTimeSlots(&cx->runtime()->dateTimeInfo);

    args.rval().set(dateObj->getReservedSlot(LOCAL_HOURS_SLOT));
    return true;
}

static bool
date_getHours(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getHours_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getUTCHours_impl(JSContext* cx, CallArgs args)
{
    double result = args.thisv().toObject().as<DateObject>().UTCTime().toNumber();
    if (IsFinite(result))
        result = HourFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static bool
date_getUTCHours(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getUTCHours_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getMinutes_impl(JSContext* cx, CallArgs args)
{
    DateObject* dateObj = &args.thisv().toObject().as<DateObject>();
    dateObj->fillLocalTimeSlots(&cx->runtime()->dateTimeInfo);

    args.rval().set(dateObj->getReservedSlot(LOCAL_MINUTES_SLOT));
    return true;
}

static bool
date_getMinutes(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getMinutes_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getUTCMinutes_impl(JSContext* cx, CallArgs args)
{
    double result = args.thisv().toObject().as<DateObject>().UTCTime().toNumber();
    if (IsFinite(result))
        result = MinFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static bool
date_getUTCMinutes(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getUTCMinutes_impl>(cx, args);
}



 MOZ_ALWAYS_INLINE bool
DateObject::getUTCSeconds_impl(JSContext* cx, CallArgs args)
{
    DateObject* dateObj = &args.thisv().toObject().as<DateObject>();
    dateObj->fillLocalTimeSlots(&cx->runtime()->dateTimeInfo);

    args.rval().set(dateObj->getReservedSlot(LOCAL_SECONDS_SLOT));
    return true;
}

static bool
date_getUTCSeconds(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getUTCSeconds_impl>(cx, args);
}



 MOZ_ALWAYS_INLINE bool
DateObject::getUTCMilliseconds_impl(JSContext* cx, CallArgs args)
{
    double result = args.thisv().toObject().as<DateObject>().UTCTime().toNumber();
    if (IsFinite(result))
        result = msFromTime(result);

    args.rval().setNumber(result);
    return true;
}

static bool
date_getUTCMilliseconds(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getUTCMilliseconds_impl>(cx, args);
}

 MOZ_ALWAYS_INLINE bool
DateObject::getTimezoneOffset_impl(JSContext* cx, CallArgs args)
{
    DateObject* dateObj = &args.thisv().toObject().as<DateObject>();
    double utctime = dateObj->UTCTime().toNumber();
    double localtime = dateObj->cachedLocalTime(&cx->runtime()->dateTimeInfo);

    




    double result = (utctime - localtime) / msPerMinute;
    args.rval().setNumber(result);
    return true;
}

static bool
date_getTimezoneOffset(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, DateObject::getTimezoneOffset_impl>(cx, args);
}

MOZ_ALWAYS_INLINE bool
date_setTime_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());
    if (args.length() == 0) {
        dateObj->setUTCTime(GenericNaN(), args.rval());
        return true;
    }

    double result;
    if (!ToNumber(cx, args[0], &result))
        return false;

    dateObj->setUTCTime(TimeClip(result), args.rval());
    return true;
}

static bool
date_setTime(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setTime_impl>(cx, args);
}

static bool
GetMsecsOrDefault(JSContext* cx, const CallArgs& args, unsigned i, double t, double* millis)
{
    if (args.length() <= i) {
        *millis = msFromTime(t);
        return true;
    }
    return ToNumber(cx, args[i], millis);
}

static bool
GetSecsOrDefault(JSContext* cx, const CallArgs& args, unsigned i, double t, double* sec)
{
    if (args.length() <= i) {
        *sec = SecFromTime(t);
        return true;
    }
    return ToNumber(cx, args[i], sec);
}

static bool
GetMinsOrDefault(JSContext* cx, const CallArgs& args, unsigned i, double t, double* mins)
{
    if (args.length() <= i) {
        *mins = MinFromTime(t);
        return true;
    }
    return ToNumber(cx, args[i], mins);
}


MOZ_ALWAYS_INLINE bool
date_setMilliseconds_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = LocalTime(dateObj->UTCTime().toNumber(), &cx->runtime()->dateTimeInfo);

    
    double milli;
    if (!ToNumber(cx, args.get(0), &milli))
        return false;
    double time = MakeTime(HourFromTime(t), MinFromTime(t), SecFromTime(t), milli);

    
    double u = TimeClip(UTC(MakeDate(Day(t), time), &cx->runtime()->dateTimeInfo));

    
    dateObj->setUTCTime(u, args.rval());
    return true;
}

static bool
date_setMilliseconds(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setMilliseconds_impl>(cx, args);
}


MOZ_ALWAYS_INLINE bool
date_setUTCMilliseconds_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = dateObj->UTCTime().toNumber();

    
    double milli;
    if (!ToNumber(cx, args.get(0), &milli))
        return false;
    double time = MakeTime(HourFromTime(t), MinFromTime(t), SecFromTime(t), milli);

    
    double v = TimeClip(MakeDate(Day(t), time));

    
    dateObj->setUTCTime(v, args.rval());
    return true;
}

static bool
date_setUTCMilliseconds(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCMilliseconds_impl>(cx, args);
}


MOZ_ALWAYS_INLINE bool
date_setSeconds_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = LocalTime(dateObj->UTCTime().toNumber(), &cx->runtime()->dateTimeInfo);

    
    double s;
    if (!ToNumber(cx, args.get(0), &s))
        return false;

    
    double milli;
    if (!GetMsecsOrDefault(cx, args, 1, t, &milli))
        return false;

    
    double date = MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), s, milli));

    
    double u = TimeClip(UTC(date, &cx->runtime()->dateTimeInfo));

    
    dateObj->setUTCTime(u, args.rval());
    return true;
}


static bool
date_setSeconds(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setSeconds_impl>(cx, args);
}

MOZ_ALWAYS_INLINE bool
date_setUTCSeconds_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = dateObj->UTCTime().toNumber();

    
    double s;
    if (!ToNumber(cx, args.get(0), &s))
        return false;

    
    double milli;
    if (!GetMsecsOrDefault(cx, args, 1, t, &milli))
        return false;

    
    double date = MakeDate(Day(t), MakeTime(HourFromTime(t), MinFromTime(t), s, milli));

    
    double v = TimeClip(date);

    
    dateObj->setUTCTime(v, args.rval());
    return true;
}


static bool
date_setUTCSeconds(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCSeconds_impl>(cx, args);
}

MOZ_ALWAYS_INLINE bool
date_setMinutes_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = LocalTime(dateObj->UTCTime().toNumber(), &cx->runtime()->dateTimeInfo);

    
    double m;
    if (!ToNumber(cx, args.get(0), &m))
        return false;

    
    double s;
    if (!GetSecsOrDefault(cx, args, 1, t, &s))
        return false;

    
    double milli;
    if (!GetMsecsOrDefault(cx, args, 2, t, &milli))
        return false;

    
    double date = MakeDate(Day(t), MakeTime(HourFromTime(t), m, s, milli));

    
    double u = TimeClip(UTC(date, &cx->runtime()->dateTimeInfo));

    
    dateObj->setUTCTime(u, args.rval());
    return true;
}


static bool
date_setMinutes(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setMinutes_impl>(cx, args);
}

MOZ_ALWAYS_INLINE bool
date_setUTCMinutes_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = dateObj->UTCTime().toNumber();

    
    double m;
    if (!ToNumber(cx, args.get(0), &m))
        return false;

    
    double s;
    if (!GetSecsOrDefault(cx, args, 1, t, &s))
        return false;

    
    double milli;
    if (!GetMsecsOrDefault(cx, args, 2, t, &milli))
        return false;

    
    double date = MakeDate(Day(t), MakeTime(HourFromTime(t), m, s, milli));

    
    double v = TimeClip(date);

    
    dateObj->setUTCTime(v, args.rval());
    return true;
}


static bool
date_setUTCMinutes(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCMinutes_impl>(cx, args);
}

MOZ_ALWAYS_INLINE bool
date_setHours_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = LocalTime(dateObj->UTCTime().toNumber(), &cx->runtime()->dateTimeInfo);

    
    double h;
    if (!ToNumber(cx, args.get(0), &h))
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

    
    double u = TimeClip(UTC(date, &cx->runtime()->dateTimeInfo));

    
    dateObj->setUTCTime(u, args.rval());
    return true;
}


static bool
date_setHours(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setHours_impl>(cx, args);
}

MOZ_ALWAYS_INLINE bool
date_setUTCHours_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = dateObj->UTCTime().toNumber();

    
    double h;
    if (!ToNumber(cx, args.get(0), &h))
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

    
    dateObj->setUTCTime(v, args.rval());
    return true;
}


static bool
date_setUTCHours(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCHours_impl>(cx, args);
}

MOZ_ALWAYS_INLINE bool
date_setDate_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = LocalTime(dateObj->UTCTime().toNumber(), &cx->runtime()->dateTimeInfo);

    
    double date;
    if (!ToNumber(cx, args.get(0), &date))
        return false;

    
    double newDate = MakeDate(MakeDay(YearFromTime(t), MonthFromTime(t), date), TimeWithinDay(t));

    
    double u = TimeClip(UTC(newDate, &cx->runtime()->dateTimeInfo));

    
    dateObj->setUTCTime(u, args.rval());
    return true;
}


static bool
date_setDate(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setDate_impl>(cx, args);
}

MOZ_ALWAYS_INLINE bool
date_setUTCDate_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = dateObj->UTCTime().toNumber();

    
    double date;
    if (!ToNumber(cx, args.get(0), &date))
        return false;

    
    double newDate = MakeDate(MakeDay(YearFromTime(t), MonthFromTime(t), date), TimeWithinDay(t));

    
    double v = TimeClip(newDate);

    
    dateObj->setUTCTime(v, args.rval());
    return true;
}

static bool
date_setUTCDate(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCDate_impl>(cx, args);
}

static bool
GetDateOrDefault(JSContext* cx, const CallArgs& args, unsigned i, double t, double* date)
{
    if (args.length() <= i) {
        *date = DateFromTime(t);
        return true;
    }
    return ToNumber(cx, args[i], date);
}

static bool
GetMonthOrDefault(JSContext* cx, const CallArgs& args, unsigned i, double t, double* month)
{
    if (args.length() <= i) {
        *month = MonthFromTime(t);
        return true;
    }
    return ToNumber(cx, args[i], month);
}


MOZ_ALWAYS_INLINE bool
date_setMonth_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = LocalTime(dateObj->UTCTime().toNumber(), &cx->runtime()->dateTimeInfo);

    
    double m;
    if (!ToNumber(cx, args.get(0), &m))
        return false;

    
    double date;
    if (!GetDateOrDefault(cx, args, 1, t, &date))
        return false;

    
    double newDate = MakeDate(MakeDay(YearFromTime(t), m, date), TimeWithinDay(t));

    
    double u = TimeClip(UTC(newDate, &cx->runtime()->dateTimeInfo));

    
    dateObj->setUTCTime(u, args.rval());
    return true;
}

static bool
date_setMonth(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setMonth_impl>(cx, args);
}


MOZ_ALWAYS_INLINE bool
date_setUTCMonth_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = dateObj->UTCTime().toNumber();

    
    double m;
    if (!ToNumber(cx, args.get(0), &m))
        return false;

    
    double date;
    if (!GetDateOrDefault(cx, args, 1, t, &date))
        return false;

    
    double newDate = MakeDate(MakeDay(YearFromTime(t), m, date), TimeWithinDay(t));

    
    double v = TimeClip(newDate);

    
    dateObj->setUTCTime(v, args.rval());
    return true;
}

static bool
date_setUTCMonth(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCMonth_impl>(cx, args);
}

static double
ThisLocalTimeOrZero(Handle<DateObject*> dateObj, DateTimeInfo* dtInfo)
{
    double t = dateObj->UTCTime().toNumber();
    if (IsNaN(t))
        return +0;
    return LocalTime(t, dtInfo);
}

static double
ThisUTCTimeOrZero(Handle<DateObject*> dateObj)
{
    double t = dateObj->as<DateObject>().UTCTime().toNumber();
    return IsNaN(t) ? +0 : t;
}


MOZ_ALWAYS_INLINE bool
date_setFullYear_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = ThisLocalTimeOrZero(dateObj, &cx->runtime()->dateTimeInfo);

    
    double y;
    if (!ToNumber(cx, args.get(0), &y))
        return false;

    
    double m;
    if (!GetMonthOrDefault(cx, args, 1, t, &m))
        return false;

    
    double date;
    if (!GetDateOrDefault(cx, args, 2, t, &date))
        return false;

    
    double newDate = MakeDate(MakeDay(y, m, date), TimeWithinDay(t));

    
    double u = TimeClip(UTC(newDate, &cx->runtime()->dateTimeInfo));

    
    dateObj->setUTCTime(u, args.rval());
    return true;
}

static bool
date_setFullYear(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setFullYear_impl>(cx, args);
}


MOZ_ALWAYS_INLINE bool
date_setUTCFullYear_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = ThisUTCTimeOrZero(dateObj);

    
    double y;
    if (!ToNumber(cx, args.get(0), &y))
        return false;

    
    double m;
    if (!GetMonthOrDefault(cx, args, 1, t, &m))
        return false;

    
    double date;
    if (!GetDateOrDefault(cx, args, 2, t, &date))
        return false;

    
    double newDate = MakeDate(MakeDay(y, m, date), TimeWithinDay(t));

    
    double v = TimeClip(newDate);

    
    dateObj->setUTCTime(v, args.rval());
    return true;
}

static bool
date_setUTCFullYear(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setUTCFullYear_impl>(cx, args);
}


MOZ_ALWAYS_INLINE bool
date_setYear_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    
    double t = ThisLocalTimeOrZero(dateObj, &cx->runtime()->dateTimeInfo);

    
    double y;
    if (!ToNumber(cx, args.get(0), &y))
        return false;

    
    if (IsNaN(y)) {
        dateObj->setUTCTime(GenericNaN(), args.rval());
        return true;
    }

    
    double yint = ToInteger(y);
    if (0 <= yint && yint <= 99)
        yint += 1900;

    
    double day = MakeDay(yint, MonthFromTime(t), DateFromTime(t));

    
    double u = UTC(MakeDate(day, TimeWithinDay(t)), &cx->runtime()->dateTimeInfo);

    
    dateObj->setUTCTime(TimeClip(u), args.rval());
    return true;
}

static bool
date_setYear(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_setYear_impl>(cx, args);
}


static const char js_NaN_date_str[] = "Invalid Date";
static const char * const days[] =
{
   "Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};
static const char * const months[] =
{
   "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};




static void
print_gmt_string(char* buf, size_t size, double utctime)
{
    MOZ_ASSERT(TimeClip(utctime) == utctime);
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
    MOZ_ASSERT(TimeClip(utctime) == utctime);
    JS_snprintf(buf, size, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3dZ",
                int(YearFromTime(utctime)),
                int(MonthFromTime(utctime)) + 1,
                int(DateFromTime(utctime)),
                int(HourFromTime(utctime)),
                int(MinFromTime(utctime)),
                int(SecFromTime(utctime)),
                int(msFromTime(utctime)));
}

static void
print_iso_extended_string(char* buf, size_t size, double utctime)
{
    MOZ_ASSERT(TimeClip(utctime) == utctime);
    JS_snprintf(buf, size, "%+.6d-%.2d-%.2dT%.2d:%.2d:%.2d.%.3dZ",
                int(YearFromTime(utctime)),
                int(MonthFromTime(utctime)) + 1,
                int(DateFromTime(utctime)),
                int(HourFromTime(utctime)),
                int(MinFromTime(utctime)),
                int(SecFromTime(utctime)),
                int(msFromTime(utctime)));
}


MOZ_ALWAYS_INLINE bool
date_toGMTString_impl(JSContext* cx, CallArgs args)
{
    double utctime = args.thisv().toObject().as<DateObject>().UTCTime().toNumber();

    char buf[100];
    if (!IsFinite(utctime))
        JS_snprintf(buf, sizeof buf, js_NaN_date_str);
    else
        print_gmt_string(buf, sizeof buf, utctime);

    JSString* str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static bool
date_toGMTString(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toGMTString_impl>(cx, args);
}


MOZ_ALWAYS_INLINE bool
date_toISOString_impl(JSContext* cx, CallArgs args)
{
    double utctime = args.thisv().toObject().as<DateObject>().UTCTime().toNumber();
    if (!IsFinite(utctime)) {
        JS_ReportErrorNumber(cx, js::GetErrorMessage, nullptr, JSMSG_INVALID_DATE);
        return false;
    }

    char buf[100];
    int year = int(YearFromTime(utctime));
    if (year < 0 || year > 9999)
        print_iso_extended_string(buf, sizeof buf, utctime);
    else
        print_iso_string(buf, sizeof buf, utctime);

    JSString* str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;

}

static bool
date_toISOString(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toISOString_impl>(cx, args);
}


static bool
date_toJSON(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedObject obj(cx, ToObject(cx, args.thisv()));
    if (!obj)
        return false;

    
    RootedValue tv(cx, ObjectValue(*obj));
    if (!ToPrimitive(cx, JSTYPE_NUMBER, &tv))
        return false;

    
    if (tv.isDouble() && !IsFinite(tv.toDouble())) {
        args.rval().setNull();
        return true;
    }

    
    RootedValue toISO(cx);
    if (!GetProperty(cx, obj, obj, cx->names().toISOString, &toISO))
        return false;

    
    if (!IsCallable(toISO)) {
        JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js::GetErrorMessage, nullptr,
                                     JSMSG_BAD_TOISOSTRING_PROP);
        return false;
    }

    
    InvokeArgs args2(cx);
    if (!args2.init(0))
        return false;

    args2.setCallee(toISO);
    args2.setThis(ObjectValue(*obj));

    if (!Invoke(cx, args2))
        return false;
    args.rval().set(args2.rval());
    return true;
}



static void
new_explode(double timeval, PRMJTime* split, DateTimeInfo* dtInfo)
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


static bool
date_format(JSContext* cx, double date, formatspec format, MutableHandleValue rval)
{
    char buf[100];
    char tzbuf[100];
    bool usetz;
    size_t i, tzlen;
    PRMJTime split;

    if (!IsFinite(date)) {
        JS_snprintf(buf, sizeof buf, js_NaN_date_str);
    } else {
        MOZ_ASSERT(TimeClip(date) == date);

        double local = LocalTime(date, &cx->runtime()->dateTimeInfo);

        

        int minutes = (int) floor(AdjustTime(date, &cx->runtime()->dateTimeInfo) / msPerMinute);

        
        int offset = (minutes / 60) * 100 + minutes % 60;

        








        

        new_explode(date, &split, &cx->runtime()->dateTimeInfo);
        if (PRMJ_FormatTime(tzbuf, sizeof tzbuf, "(%Z)", &split) != 0) {

            





            usetz = true;
            tzlen = strlen(tzbuf);
            if (tzlen > 100) {
                usetz = false;
            } else {
                for (i = 0; i < tzlen; i++) {
                    char16_t c = tzbuf[i];
                    if (c > 127 ||
                        !(isalpha(c) || isdigit(c) ||
                          c == ' ' || c == '(' || c == ')' || c == '.')) {
                        usetz = false;
                    }
                }
            }

            
            if (tzbuf[0] != '(' || tzbuf[1] == ')')
                usetz = false;
        } else
            usetz = false;

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

    JSString* str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return false;
    rval.setString(str);
    return true;
}

static bool
ToLocaleFormatHelper(JSContext* cx, HandleObject obj, const char* format, MutableHandleValue rval)
{
    double utctime = obj->as<DateObject>().UTCTime().toNumber();

    char buf[100];
    if (!IsFinite(utctime)) {
        JS_snprintf(buf, sizeof buf, js_NaN_date_str);
    } else {
        int result_len;
        double local = LocalTime(utctime, &cx->runtime()->dateTimeInfo);
        PRMJTime split;
        new_explode(local, &split, &cx->runtime()->dateTimeInfo);

        
        result_len = PRMJ_FormatTime(buf, sizeof buf, format, &split);

        
        if (result_len == 0)
            return date_format(cx, utctime, FORMATSPEC_FULL, rval);

        
        if (strcmp(format, "%x") == 0 && result_len >= 6 &&
            

            !isdigit(buf[result_len - 3]) &&
            isdigit(buf[result_len - 2]) && isdigit(buf[result_len - 1]) &&
            
            !(isdigit(buf[0]) && isdigit(buf[1]) &&
              isdigit(buf[2]) && isdigit(buf[3]))) {
            double localtime = obj->as<DateObject>().cachedLocalTime(&cx->runtime()->dateTimeInfo);
            int year = IsNaN(localtime) ? 0 : (int) YearFromTime(localtime);
            JS_snprintf(buf + (result_len - 2), (sizeof buf) - (result_len - 2),
                        "%d", year);
        }

    }

    if (cx->runtime()->localeCallbacks && cx->runtime()->localeCallbacks->localeToUnicode)
        return cx->runtime()->localeCallbacks->localeToUnicode(cx, buf, rval);

    JSString* str = JS_NewStringCopyZ(cx, buf);
    if (!str)
        return false;
    rval.setString(str);
    return true;
}

#if !EXPOSE_INTL_API
static bool
ToLocaleStringHelper(JSContext* cx, Handle<DateObject*> dateObj, MutableHandleValue rval)
{
    



    return ToLocaleFormatHelper(cx, dateObj,
#if defined(_WIN32) && !defined(__MWERKS__)
                          "%#c"
#else
                          "%c"
#endif
                         , rval);
}


MOZ_ALWAYS_INLINE bool
date_toLocaleString_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());
    return ToLocaleStringHelper(cx, dateObj, args.rval());
}

static bool
date_toLocaleString(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toLocaleString_impl>(cx, args);
}


MOZ_ALWAYS_INLINE bool
date_toLocaleDateString_impl(JSContext* cx, CallArgs args)
{
    



    static const char format[] =
#if defined(_WIN32) && !defined(__MWERKS__)
                                   "%#x"
#else
                                   "%x"
#endif
                                   ;

    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());
    return ToLocaleFormatHelper(cx, dateObj, format, args.rval());
}

static bool
date_toLocaleDateString(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toLocaleDateString_impl>(cx, args);
}


MOZ_ALWAYS_INLINE bool
date_toLocaleTimeString_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());
    return ToLocaleFormatHelper(cx, dateObj, "%X", args.rval());
}

static bool
date_toLocaleTimeString(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toLocaleTimeString_impl>(cx, args);
}
#endif 

MOZ_ALWAYS_INLINE bool
date_toLocaleFormat_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());

    if (args.length() == 0) {
        



        return ToLocaleFormatHelper(cx, dateObj,
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

    return ToLocaleFormatHelper(cx, dateObj, fmtbytes.ptr(), args.rval());
}

static bool
date_toLocaleFormat(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toLocaleFormat_impl>(cx, args);
}


MOZ_ALWAYS_INLINE bool
date_toTimeString_impl(JSContext* cx, CallArgs args)
{
    return date_format(cx, args.thisv().toObject().as<DateObject>().UTCTime().toNumber(),
                       FORMATSPEC_TIME, args.rval());
}

static bool
date_toTimeString(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toTimeString_impl>(cx, args);
}


MOZ_ALWAYS_INLINE bool
date_toDateString_impl(JSContext* cx, CallArgs args)
{
    return date_format(cx, args.thisv().toObject().as<DateObject>().UTCTime().toNumber(),
                       FORMATSPEC_DATE, args.rval());
}

static bool
date_toDateString(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toDateString_impl>(cx, args);
}

#if JS_HAS_TOSOURCE
MOZ_ALWAYS_INLINE bool
date_toSource_impl(JSContext* cx, CallArgs args)
{
    StringBuffer sb(cx);
    if (!sb.append("(new Date(") ||
        !NumberValueToStringBuffer(cx, args.thisv().toObject().as<DateObject>().UTCTime(), sb) ||
        !sb.append("))"))
    {
        return false;
    }

    JSString* str = sb.finishString();
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static bool
date_toSource(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toSource_impl>(cx, args);
}
#endif

MOZ_ALWAYS_INLINE bool
date_toString_impl(JSContext* cx, CallArgs args)
{
    return date_format(cx, args.thisv().toObject().as<DateObject>().UTCTime().toNumber(),
                       FORMATSPEC_FULL, args.rval());
}

static bool
date_toString(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_toString_impl>(cx, args);
}

MOZ_ALWAYS_INLINE bool
date_valueOf_impl(JSContext* cx, CallArgs args)
{
    Rooted<DateObject*> dateObj(cx, &args.thisv().toObject().as<DateObject>());
    args.rval().set(dateObj->UTCTime());
    return true;
}

bool
js::date_valueOf(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsDate, date_valueOf_impl>(cx, args);
}

static const JSFunctionSpec date_static_methods[] = {
    JS_FN("UTC",                 date_UTC,                MAXARGS,0),
    JS_FN("parse",               date_parse,              1,0),
    JS_FN("now",                 date_now,                0,0),
    JS_FS_END
};

static const JSFunctionSpec date_methods[] = {
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
#if EXPOSE_INTL_API
    JS_SELF_HOSTED_FN(js_toLocaleString_str, "Date_toLocaleString", 0,0),
    JS_SELF_HOSTED_FN("toLocaleDateString", "Date_toLocaleDateString", 0,0),
    JS_SELF_HOSTED_FN("toLocaleTimeString", "Date_toLocaleTimeString", 0,0),
#else
    JS_FN(js_toLocaleString_str, date_toLocaleString,     0,0),
    JS_FN("toLocaleDateString",  date_toLocaleDateString, 0,0),
    JS_FN("toLocaleTimeString",  date_toLocaleTimeString, 0,0),
#endif
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

bool
js::DateConstructor(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    if (!args.isConstructing())
        return date_format(cx, NowAsMillis(), FORMATSPEC_FULL, args.rval());

    
    double d;
    if (args.length() == 0) {
        
        d = NowAsMillis();
    } else if (args.length() == 1) {
        

        
        if (!ToPrimitive(cx, args[0]))
            return false;

        if (args[0].isString()) {
            
            JSString* str = args[0].toString();
            if (!str)
                return false;

            JSLinearString* linearStr = str->ensureLinear(cx);
            if (!linearStr)
                return false;

            if (!ParseDate(linearStr, &d, &cx->runtime()->dateTimeInfo))
                d = GenericNaN();
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

        if (IsFinite(msec_time)) {
            msec_time = UTC(msec_time, &cx->runtime()->dateTimeInfo);
            msec_time = TimeClip(msec_time);
        }
        d = msec_time;
    }

    JSObject* obj = NewDateObjectMsec(cx, d);
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

static bool
FinishDateClassInit(JSContext* cx, HandleObject ctor, HandleObject proto)
{
    proto->as<DateObject>().setUTCTime(GenericNaN());

    



    RootedValue toUTCStringFun(cx);
    RootedId toUTCStringId(cx, NameToId(cx->names().toUTCString));
    RootedId toGMTStringId(cx, NameToId(cx->names().toGMTString));
    return NativeGetProperty(cx, proto.as<DateObject>(), toUTCStringId, &toUTCStringFun) &&
           NativeDefineProperty(cx, proto.as<DateObject>(), toGMTStringId, toUTCStringFun,
                                nullptr, nullptr, 0);
}

const Class DateObject::class_ = {
    js_Date_str,
    JSCLASS_HAS_RESERVED_SLOTS(RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Date),
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    date_convert,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    {
        GenericCreateConstructor<DateConstructor, MAXARGS, JSFunction::FinalizeKind>,
        GenericCreatePrototype,
        date_static_methods,
        nullptr,
        date_methods,
        nullptr,
        FinishDateClassInit
    }
};

JS_FRIEND_API(JSObject*)
js::NewDateObjectMsec(JSContext* cx, double msec_time)
{
    JSObject* obj = NewBuiltinClassInstance(cx, &DateObject::class_);
    if (!obj)
        return nullptr;
    obj->as<DateObject>().setUTCTime(msec_time);
    return obj;
}

JS_FRIEND_API(JSObject*)
js::NewDateObject(JSContext* cx, int year, int mon, int mday,
                  int hour, int min, int sec)
{
    MOZ_ASSERT(mon < 12);
    double msec_time = date_msecFromDate(year, mon, mday, hour, min, sec, 0);
    return NewDateObjectMsec(cx, UTC(msec_time, &cx->runtime()->dateTimeInfo));
}

JS_FRIEND_API(bool)
js::DateIsValid(JSContext* cx, JSObject* objArg)
{
    RootedObject obj(cx, objArg);
    if (!ObjectClassIs(obj, ESClass_Date, cx))
        return false;

    RootedValue unboxed(cx);
    if (!Unbox(cx, obj, &unboxed)) {
        
        
        cx->clearPendingException();
        return false;
    }

    return !IsNaN(unboxed.toNumber());
}

JS_FRIEND_API(double)
js::DateGetMsecSinceEpoch(JSContext* cx, JSObject* objArg)
{
    RootedObject obj(cx, objArg);
    if (!ObjectClassIs(obj, ESClass_Date, cx))
        return 0;

    RootedValue unboxed(cx);
    if (!Unbox(cx, obj, &unboxed)) {
        
        
        cx->clearPendingException();
        return 0;
    }

    return unboxed.toNumber();
}
