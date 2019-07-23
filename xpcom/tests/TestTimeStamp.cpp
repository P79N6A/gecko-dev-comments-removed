




































#include "mozilla/TimeStamp.h"

#include "TestHarness.h"

#include "prinrval.h"
#include "prthread.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;

static void Assert(PRBool aCond, const char* aMsg)
{
    if (aCond) {
        passed(aMsg);
    } else {
        fail("%s", aMsg);
    }
}

int main(int argc, char** argv)
{
    ScopedXPCOM xpcom("nsTimeStamp");
    if (xpcom.failed())
        return 1;

    TimeDuration td;
    Assert(td.ToSeconds() == 0.0, "TimeDuration default value 0");
    Assert(TimeDuration::FromSeconds(5).ToSeconds() == 5.0,
           "TimeDuration FromSeconds/ToSeconds round-trip");
    Assert(TimeDuration::FromMilliseconds(5000).ToSeconds() == 5.0,
           "TimeDuration FromMilliseconds/ToSeconds round-trip");
    Assert(TimeDuration::FromSeconds(1) < TimeDuration::FromSeconds(2),
           "TimeDuration < operator works");
    Assert(!(TimeDuration::FromSeconds(1) < TimeDuration::FromSeconds(1)),
           "TimeDuration < operator works");
    Assert(TimeDuration::FromSeconds(2) > TimeDuration::FromSeconds(1),
           "TimeDuration > operator works");
    Assert(!(TimeDuration::FromSeconds(1) > TimeDuration::FromSeconds(1)),
           "TimeDuration > operator works");
    Assert(TimeDuration::FromSeconds(1) <= TimeDuration::FromSeconds(2),
           "TimeDuration <= operator works");
    Assert(TimeDuration::FromSeconds(1) <= TimeDuration::FromSeconds(1),
           "TimeDuration <= operator works");
    Assert(!(TimeDuration::FromSeconds(2) <= TimeDuration::FromSeconds(1)),
           "TimeDuration <= operator works");
    Assert(TimeDuration::FromSeconds(2) >= TimeDuration::FromSeconds(1),
           "TimeDuration >= operator works");
    Assert(TimeDuration::FromSeconds(1) >= TimeDuration::FromSeconds(1),
           "TimeDuration >= operator works");
    Assert(!(TimeDuration::FromSeconds(1) >= TimeDuration::FromSeconds(2)),
           "TimeDuration >= operator works");

    TimeStamp ts;
    Assert(ts.IsNull(), "Default TimeStamp value null");
    
    ts = TimeStamp::Now();
    Assert(!ts.IsNull(), "TimeStamp time value non-null");
    Assert((ts - ts).ToSeconds() == 0.0, "TimeStamp zero-length duration");

    PR_Sleep(PR_SecondsToInterval(2));

    TimeStamp ts2(TimeStamp::Now());
    Assert(ts2 > ts, "TimeStamp > comparison");
    Assert(!(ts > ts), "TimeStamp > comparison");
    Assert(ts < ts2, "TimeStamp < comparison");
    Assert(!(ts < ts), "TimeStamp < comparison");
    Assert(ts <= ts2, "TimeStamp <= comparison");
    Assert(ts <= ts, "TimeStamp <= comparison");
    Assert(!(ts2 <= ts), "TimeStamp <= comparison");
    Assert(ts2 >= ts, "TimeStamp >= comparison");
    Assert(ts2 >= ts, "TimeStamp >= comparison");
    Assert(!(ts >= ts2), "TimeStamp >= comparison");

    
    
    
    td = ts2 - ts;
    Assert(td.ToSeconds() > 1.0, "TimeStamp difference lower bound");
    Assert(td.ToSeconds() < 20.0, "TimeStamp difference upper bound");
    td = ts - ts2;
    Assert(td.ToSeconds() < -1.0, "TimeStamp negative difference lower bound");
    Assert(td.ToSeconds() > -20.0, "TimeStamp negative difference upper bound");

    return gFailCount > 0;
}
