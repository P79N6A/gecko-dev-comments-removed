













































#include "rcinrval.h"

RCInterval::~RCInterval() { }

RCInterval::RCInterval(RCInterval::RCReservedInterval special): RCBase()
{
    switch (special)
    {
    case RCInterval::now:
        interval = PR_IntervalNow();
        break;
    case RCInterval::no_timeout:
        interval = PR_INTERVAL_NO_TIMEOUT;
        break;
    case RCInterval::no_wait:
        interval = PR_INTERVAL_NO_WAIT;
        break;
    default:
        break;
    }
}  


