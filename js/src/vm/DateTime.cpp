





#include "DateTime.h"

#include <time.h>

#include "jsutil.h"

static bool
ComputeLocalTime(time_t local, struct tm *ptm)
{
#ifdef HAVE_LOCALTIME_R
    return localtime_r(&local, ptm);
#else
    struct tm *otm = localtime(&local);
    if (!otm)
        return false;
    *ptm = *otm;
    return true;
#endif
}

static int32_t
LocalUTCDifferenceSeconds()
{
    using js::SecondsPerDay;

#if defined(XP_WIN)
    
    
    
    _tzset();
#endif

    




    int day = 0;
    struct tm tm;

    if (!ComputeLocalTime(0, &tm))
        return 0;
    if (tm.tm_isdst > 0) {
        day = 180;
        if (!ComputeLocalTime(SecondsPerDay * day, &tm))
            return 0;
    }

    int32_t time = (tm.tm_hour * 3600) + (tm.tm_min * 60) + tm.tm_sec;
    time = SecondsPerDay - time;

    if (tm.tm_yday == day)
        time -= SecondsPerDay;

    return time;
}

void
js::DateTimeInfo::updateTimeZoneAdjustment()
{
    double newTZA = -(LocalUTCDifferenceSeconds() * msPerSecond);
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
    
    
    localTZA_ = MOZ_DOUBLE_NaN();
    updateTimeZoneAdjustment();
}

int64_t
js::DateTimeInfo::computeDSTOffsetMilliseconds(int64_t localTimeSeconds)
{
    MOZ_ASSERT(localTimeSeconds >= 0);
    MOZ_ASSERT(localTimeSeconds <= MaxUnixTimeT);

#if defined(XP_WIN)
    




    _tzset();
#endif

    struct tm tm;
    if (!ComputeLocalTime(static_cast<time_t>(localTimeSeconds), &tm))
        return 0;

    int32_t base = LocalUTCDifferenceSeconds();

    int32_t dayoff = int32_t((localTimeSeconds - base) % (SecondsPerHour * 24));
    int32_t tmoff = tm.tm_sec + (tm.tm_min * SecondsPerMinute) +
        (tm.tm_hour * SecondsPerHour);

    int32_t diff = tmoff - dayoff;

    if (diff < 0)
        diff += SecondsPerDay;

    return diff * msPerSecond;
}

int64_t
js::DateTimeInfo::getDSTOffsetMilliseconds(int64_t localTimeMilliseconds)
{
    sanityCheck();

    int64_t localTimeSeconds = localTimeMilliseconds / msPerSecond;

    if (localTimeSeconds > MaxUnixTimeT) {
        localTimeSeconds = MaxUnixTimeT;
    } else if (localTimeSeconds < 0) {
        
        localTimeSeconds = SecondsPerDay;
    }

    





    if (rangeStartSeconds <= localTimeSeconds &&
        localTimeSeconds <= rangeEndSeconds) {
        return offsetMilliseconds;
    }

    if (oldRangeStartSeconds <= localTimeSeconds &&
        localTimeSeconds <= oldRangeEndSeconds) {
        return oldOffsetMilliseconds;
    }

    oldOffsetMilliseconds = offsetMilliseconds;
    oldRangeStartSeconds = rangeStartSeconds;
    oldRangeEndSeconds = rangeEndSeconds;

    if (rangeStartSeconds <= localTimeSeconds) {
        int64_t newEndSeconds = Min(rangeEndSeconds + RangeExpansionAmount, MaxUnixTimeT);
        if (newEndSeconds >= localTimeSeconds) {
            int64_t endOffsetMilliseconds = computeDSTOffsetMilliseconds(newEndSeconds);
            if (endOffsetMilliseconds == offsetMilliseconds) {
                rangeEndSeconds = newEndSeconds;
                return offsetMilliseconds;
            }

            offsetMilliseconds = computeDSTOffsetMilliseconds(localTimeSeconds);
            if (offsetMilliseconds == endOffsetMilliseconds) {
                rangeStartSeconds = localTimeSeconds;
                rangeEndSeconds = newEndSeconds;
            } else {
                rangeEndSeconds = localTimeSeconds;
            }
            return offsetMilliseconds;
        }

        offsetMilliseconds = computeDSTOffsetMilliseconds(localTimeSeconds);
        rangeStartSeconds = rangeEndSeconds = localTimeSeconds;
        return offsetMilliseconds;
    }

    int64_t newStartSeconds = Max<int64_t>(rangeStartSeconds - RangeExpansionAmount, 0);
    if (newStartSeconds <= localTimeSeconds) {
        int64_t startOffsetMilliseconds = computeDSTOffsetMilliseconds(newStartSeconds);
        if (startOffsetMilliseconds == offsetMilliseconds) {
            rangeStartSeconds = newStartSeconds;
            return offsetMilliseconds;
        }

        offsetMilliseconds = computeDSTOffsetMilliseconds(localTimeSeconds);
        if (offsetMilliseconds == startOffsetMilliseconds) {
            rangeStartSeconds = newStartSeconds;
            rangeEndSeconds = localTimeSeconds;
        } else {
            rangeStartSeconds = localTimeSeconds;
        }
        return offsetMilliseconds;
    }

    rangeStartSeconds = rangeEndSeconds = localTimeSeconds;
    offsetMilliseconds = computeDSTOffsetMilliseconds(localTimeSeconds);
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
