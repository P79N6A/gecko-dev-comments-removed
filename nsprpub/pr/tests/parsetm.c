










































#include "prtime.h"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PRBool debug_mode = PR_TRUE;

static char *dayOfWeek[] =
	{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "???" };
static char *month[] =
	{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "???" };

static void PrintExplodedTime(const PRExplodedTime *et) {
    PRInt32 totalOffset;
    PRInt32 hourOffset, minOffset;
    const char *sign;

    
    if (debug_mode) printf("%s %s %ld %02ld:%02ld:%02ld ",
	    dayOfWeek[et->tm_wday], month[et->tm_month], et->tm_mday,
	    et->tm_hour, et->tm_min, et->tm_sec);

    
    totalOffset = et->tm_params.tp_gmt_offset + et->tm_params.tp_dst_offset;
    if (totalOffset == 0) {
	if (debug_mode) printf("UTC ");
    } else {
        sign = "+";
        if (totalOffset < 0) {
	    totalOffset = -totalOffset;
	    sign = "-";
        }
        hourOffset = totalOffset / 3600;
        minOffset = (totalOffset % 3600) / 60;
        if (debug_mode) 
            printf("%s%02ld%02ld ", sign, hourOffset, minOffset);
    }

    
    if (debug_mode) printf("%hd", et->tm_year);
}

int main(int argc, char **argv)
{
    PRTime ct;
    PRExplodedTime et;
    PRStatus rv;
    char *sp1 = "Sat, 1 Jan 3001 00:00:00";  
    char *sp2 = "Fri, 31 Dec 3000 23:59:60";  

#if _MSC_VER >= 1400 && !defined(WINCE)
    
    _putenv_s("TZ", "PST8PDT");
    _tzset();
#endif

    rv = PR_ParseTimeString(sp1, PR_FALSE, &ct);
    printf("rv = %d\n", rv);
    PR_ExplodeTime(ct, PR_GMTParameters, &et);
    PrintExplodedTime(&et);
    printf("\n");

    rv = PR_ParseTimeString(sp2, PR_FALSE, &ct);
    printf("rv = %d\n", rv);
    PR_ExplodeTime(ct, PR_GMTParameters, &et);
    PrintExplodedTime(&et);
    printf("\n");

    return 0;
}
