





#include "vm/DateTime.h"

#include <time.h>

#include "jsutil.h"

using mozilla::UnspecifiedNaN;

static bool
ComputeLocalTime(time_t local, struct tm* ptm)
{
#if defined(_WIN32)
    return localtime_s(ptm, &local) == 0;
#elif defined(HAVE_LOCALTIME_R)
    return localtime_r(&local, ptm);
#else
    struct tm* otm = localtime(&local);
    if (!otm)
        return false;
    *ptm = *otm;
    return true;
#endif
}

static bool
ComputeUTCTime(time_t t, struct tm* ptm)
{
#if defined(_WIN32)
    return gmtime_s(ptm, &t) == 0;
#elif defined(HAVE_GMTIME_R)
    return gmtime_r(&t, ptm);
#else
    struct tm* otm = gmtime(&t);
    if (!otm)
        return false;
    *ptm = *otm;
    return true;
#endif
}















static int32_t
UTCToLocalStandardOffsetSeconds()
{
    using js::SecondsPerDay;
    using js::SecondsPerHour;
    using js::SecondsPerMinute;

#if defined(XP_WIN)
    
    
    
    _tzset();
#endif

    
    time_t currentMaybeWithDST = time(nullptr);
    if (currentMaybeWithDST == time_t(-1))
        return 0;

    
    
    struct tm local;
    if (!ComputeLocalTime(currentMaybeWithDST, &local))
        return 0;

    
    time_t currentNoDST;
    if (local.tm_isdst == 0) {
        
        currentNoDST = currentMaybeWithDST;
    } else {
        
        
        local.tm_isdst = 0;

        
        
        
        
        
        
        currentNoDST = mktime(&local);
        if (currentNoDST == time_t(-1))
            return 0;
    }

    
    
    struct tm utc;
    if (!ComputeUTCTime(currentNoDST, &utc))
        return 0;

    
    
    
    int utc_secs = utc.tm_hour * SecondsPerHour + utc.tm_min * SecondsPerMinute;
    int local_secs = local.tm_hour * SecondsPerHour + local.tm_min * SecondsPerMinute;

    
    if (utc.tm_mday == local.tm_mday)
        return local_secs - utc_secs;

    
    
    if (utc_secs > local_secs)
        return (SecondsPerDay + local_secs) - utc_secs;

    
    
    return local_secs - (utc_secs + SecondsPerDay);
}

void
js::DateTimeInfo::updateTimeZoneAdjustment()
{
    



    utcToLocalStandardOffsetSeconds = UTCToLocalStandardOffsetSeconds();

    double newTZA = utcToLocalStandardOffsetSeconds * msPerSecond;
    if (newTZA == localTZA_)
        return;

    localTZA_ = newTZA;

    




    offsetMilliseconds = 0;
    rangeStartSeconds = rangeEndSeconds = INT64_MIN;
    oldOffsetMilliseconds = 0;
    oldRangeStartSeconds = oldRangeEndSeconds = INT64_MIN;

    sanityCheck();
}







js::DateTimeInfo::DateTimeInfo()
{
    
    
    localTZA_ = UnspecifiedNaN<double>();
    updateTimeZoneAdjustment();
}

int64_t
js::DateTimeInfo::computeDSTOffsetMilliseconds(int64_t utcSeconds)
{
    MOZ_ASSERT(utcSeconds >= 0);
    MOZ_ASSERT(utcSeconds <= MaxUnixTimeT);

#if defined(XP_WIN)
    
    
    
    _tzset();
#endif

    struct tm tm;
    if (!ComputeLocalTime(static_cast<time_t>(utcSeconds), &tm))
        return 0;

    int32_t dayoff = int32_t((utcSeconds + utcToLocalStandardOffsetSeconds) % SecondsPerDay);
    int32_t tmoff = tm.tm_sec + (tm.tm_min * SecondsPerMinute) + (tm.tm_hour * SecondsPerHour);

    int32_t diff = tmoff - dayoff;

    if (diff < 0)
        diff += SecondsPerDay;

    return diff * msPerSecond;
}

int64_t
js::DateTimeInfo::getDSTOffsetMilliseconds(int64_t utcMilliseconds)
{
    sanityCheck();

    int64_t utcSeconds = utcMilliseconds / msPerSecond;

    if (utcSeconds > MaxUnixTimeT) {
        utcSeconds = MaxUnixTimeT;
    } else if (utcSeconds < 0) {
        
        utcSeconds = SecondsPerDay;
    }

    





    if (rangeStartSeconds <= utcSeconds && utcSeconds <= rangeEndSeconds)
        return offsetMilliseconds;

    if (oldRangeStartSeconds <= utcSeconds && utcSeconds <= oldRangeEndSeconds)
        return oldOffsetMilliseconds;

    oldOffsetMilliseconds = offsetMilliseconds;
    oldRangeStartSeconds = rangeStartSeconds;
    oldRangeEndSeconds = rangeEndSeconds;

    if (rangeStartSeconds <= utcSeconds) {
        int64_t newEndSeconds = Min(rangeEndSeconds + RangeExpansionAmount, MaxUnixTimeT);
        if (newEndSeconds >= utcSeconds) {
            int64_t endOffsetMilliseconds = computeDSTOffsetMilliseconds(newEndSeconds);
            if (endOffsetMilliseconds == offsetMilliseconds) {
                rangeEndSeconds = newEndSeconds;
                return offsetMilliseconds;
            }

            offsetMilliseconds = computeDSTOffsetMilliseconds(utcSeconds);
            if (offsetMilliseconds == endOffsetMilliseconds) {
                rangeStartSeconds = utcSeconds;
                rangeEndSeconds = newEndSeconds;
            } else {
                rangeEndSeconds = utcSeconds;
            }
            return offsetMilliseconds;
        }

        offsetMilliseconds = computeDSTOffsetMilliseconds(utcSeconds);
        rangeStartSeconds = rangeEndSeconds = utcSeconds;
        return offsetMilliseconds;
    }

    int64_t newStartSeconds = Max<int64_t>(rangeStartSeconds - RangeExpansionAmount, 0);
    if (newStartSeconds <= utcSeconds) {
        int64_t startOffsetMilliseconds = computeDSTOffsetMilliseconds(newStartSeconds);
        if (startOffsetMilliseconds == offsetMilliseconds) {
            rangeStartSeconds = newStartSeconds;
            return offsetMilliseconds;
        }

        offsetMilliseconds = computeDSTOffsetMilliseconds(utcSeconds);
        if (offsetMilliseconds == startOffsetMilliseconds) {
            rangeStartSeconds = newStartSeconds;
            rangeEndSeconds = utcSeconds;
        } else {
            rangeStartSeconds = utcSeconds;
        }
        return offsetMilliseconds;
    }

    rangeStartSeconds = rangeEndSeconds = utcSeconds;
    offsetMilliseconds = computeDSTOffsetMilliseconds(utcSeconds);
    return offsetMilliseconds;
}

void
js::DateTimeInfo::sanityCheck()
{
    MOZ_ASSERT(rangeStartSeconds <= rangeEndSeconds);
    MOZ_ASSERT_IF(rangeStartSeconds == INT64_MIN, rangeEndSeconds == INT64_MIN);
    MOZ_ASSERT_IF(rangeEndSeconds == INT64_MIN, rangeStartSeconds == INT64_MIN);
    MOZ_ASSERT_IF(rangeStartSeconds != INT64_MIN,
                  rangeStartSeconds >= 0 && rangeEndSeconds >= 0);
    MOZ_ASSERT_IF(rangeStartSeconds != INT64_MIN,
                  rangeStartSeconds <= MaxUnixTimeT && rangeEndSeconds <= MaxUnixTimeT);
}
