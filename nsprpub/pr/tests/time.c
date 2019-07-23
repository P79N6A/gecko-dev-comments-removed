


















































#include "plgetopt.h"

#include "nspr.h"
#include "prpriv.h"
#include "prinrval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define DEFAULT_COUNT 100000
PRInt32 count;

time_t itime;
hrtime_t ihrtime;

void
ftime_init()
{
   itime = time(NULL);
   ihrtime = gethrtime();
}

time_t
ftime()
{
        hrtime_t now = gethrtime();

        return itime + ((now - ihrtime) / 1000000000ll);
}

static void timeTime(void)
{
    PRInt32 index = count;
    time_t rv;
 
    for (;index--;)
        rv = time(NULL);
}

static void timeGethrtime(void)
{
    PRInt32 index = count;
    time_t rv;
 
    for (;index--;)
        rv = ftime();
}

static void timeGettimeofday(void)
{
    PRInt32 index = count;
    time_t rv;
    struct timeval tp;
 
    for (;index--;)
        rv = gettimeofday(&tp, NULL);
}

static void timePRTime32(void)
{
    PRInt32 index = count;
    PRInt32 rv32;
    PRTime q;
    PRTime rv;

    LL_I2L(q, 1000000);
 
    for (;index--;) {
        rv = PR_Now();
        LL_DIV(rv, rv, q);
        LL_L2I(rv32, rv);
    }
}

static void timePRTime64(void)
{
    PRInt32 index = count;
    PRTime rv;
 
    for (;index--;)
        rv = PR_Now();
}



static void Measure(void (*func)(void), const char *msg)
{
    PRIntervalTime start, stop;
    double d;
    PRInt32 tot;

    start = PR_IntervalNow();
    (*func)();
    stop = PR_IntervalNow();

    d = (double)PR_IntervalToMicroseconds(stop - start);
    tot = PR_IntervalToMilliseconds(stop-start);

    if (debug_mode) printf("%40s: %6.2f usec avg, %d msec total\n", msg, d / count, tot);
}

int main(int argc, char **argv)
{
	





	PLOptStatus os;
	PLOptState *opt = PL_CreateOptState(argc, argv, "d:");
	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
		if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'd':  
			debug_mode = 1;
            break;
         default:
            break;
        }
    }
	PL_DestroyOptState(opt);

    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();

    if (argc > 1) {
	count = atoi(argv[1]);
    } else {
	count = DEFAULT_COUNT;
    }

    ftime_init();

    Measure(timeTime, "time to get time with time()");
    Measure(timeGethrtime, "time to get time with gethrtime()");
    Measure(timeGettimeofday, "time to get time with gettimeofday()");
    Measure(timePRTime32, "time to get time with PR_Time() (32bit)");
    Measure(timePRTime64, "time to get time with PR_Time() (64bit)");

	PR_Cleanup();
	return 0;
}



