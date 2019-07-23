






































#include "rctime.h"

#include <prlog.h>
#include <prprf.h>

#define DEFAULT_ITERATIONS 100

PRIntn main(PRIntn argc, char **argv)
{
    RCTime unitialized;
    RCTime now(PR_Now());
    RCTime current(RCTime::now);
    PRTime time = current;

    unitialized = now;
    now.Now();

    return 0;
}  



