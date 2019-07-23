






































#include "rclock.h"
#include "rcthread.h"
#include "rcinrval.h"
#include "rccv.h"

#include <prio.h>
#include <prlog.h>
#include <prprf.h>

#define DEFAULT_ITERATIONS 100

PRIntn main(PRIntn argc, char **argv)
{
    RCLock ml;
    PRStatus rv;
    RCCondition cv(&ml);

    RCInterval now, timeout, epoch, elapsed;
    PRFileDesc *output = PR_GetSpecialFD(PR_StandardOutput);
    PRIntn msecs, seconds, loops, iterations = DEFAULT_ITERATIONS;

    
    for (seconds = 0; seconds < 10; ++seconds)
    {
        timeout = RCInterval::FromSeconds(seconds);
        cv.SetTimeout(timeout);
        {
            RCEnter lock(&ml);

            epoch.SetToNow();

            rv = cv.Wait();
            PR_ASSERT(PR_SUCCESS == rv);

            now = RCInterval(RCInterval::now);
            elapsed = now - epoch;
        }

        PR_fprintf(
            output, "Waiting %u seconds took %s%u milliseconds\n",
            seconds, ((elapsed < timeout)? "**" : ""),
            elapsed.ToMilliseconds());
    }

    
    for (seconds = 0; seconds < 10; ++seconds)
    {
        timeout = RCInterval::FromSeconds(seconds);
        {
            epoch.SetToNow();

            rv = RCThread::Sleep(timeout);
            PR_ASSERT(PR_SUCCESS == rv);

            now = RCInterval(RCInterval::now);
            elapsed = now - epoch;
        }

        PR_fprintf(
            output, "Sleeping %u seconds took %s%u milliseconds\n",
            seconds, ((elapsed < timeout)? "**" : ""),
            elapsed.ToMilliseconds());
    }

    
    for (msecs = 10; msecs < 100; msecs += 10)
    {
        timeout = RCInterval::FromMilliseconds(msecs);
        cv.SetTimeout(timeout);
        {
            RCEnter lock(&ml);

            epoch.SetToNow();

            for (loops = 0; loops < iterations; ++loops)
            {
                rv = cv.Wait();
                PR_ASSERT(PR_SUCCESS == rv);
            }

            now = RCInterval(RCInterval::now);
            elapsed = now - epoch;
        }
        elapsed /= iterations;

        PR_fprintf(
            output, "Waiting %u msecs took %s%u milliseconds average\n",
            msecs, ((elapsed < timeout)? "**" : ""), elapsed.ToMilliseconds());
    }
    return 0;
}  



