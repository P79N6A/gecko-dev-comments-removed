














































#include "plgetopt.h"

#include "prinit.h"
#include "prtime.h"
#include "prprf.h"
#include "prlog.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PRINT_DETAILS

int failed_already=0;
PRBool debug_mode = PR_FALSE;

static char *dayOfWeek[] =
	{ "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "???" };
static char *month[] =
	{ "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "???" };

PRLogModuleInfo *lm;

static void PrintExplodedTime(const PRExplodedTime *et) {
    PRInt32 totalOffset;
    PRInt32 hourOffset, minOffset;
    const char *sign;

    
    printf("%s %s %2ld %02ld:%02ld:%02ld ",
	    dayOfWeek[et->tm_wday], month[et->tm_month], et->tm_mday,
	    et->tm_hour, et->tm_min, et->tm_sec);

    
    printf("%hd ", et->tm_year);

    
    totalOffset = et->tm_params.tp_gmt_offset + et->tm_params.tp_dst_offset;
    if (totalOffset == 0) {
	printf("UTC ");
    } else {
        sign = "+";
        if (totalOffset < 0) {
	    totalOffset = -totalOffset;
	    sign = "-";
        }
        hourOffset = totalOffset / 3600;
        minOffset = (totalOffset % 3600) / 60;
        printf("%s%02ld%02ld ", sign, hourOffset, minOffset);
    }
#ifdef PRINT_DETAILS
	printf("{%d, %d, %d, %d, %d, %d, %d, %d, %d, { %d, %d}}\n",et->tm_usec,
											et->tm_sec,
											et->tm_min,
											et->tm_hour,
											et->tm_mday,
											et->tm_month,
											et->tm_year,
											et->tm_wday,
											et->tm_yday,
											et->tm_params.tp_gmt_offset,
											et->tm_params.tp_dst_offset);
#endif
}

static int ExplodedTimeIsEqual(const PRExplodedTime *et1,
	const PRExplodedTime *et2)
{
    if (et1->tm_usec == et2->tm_usec &&
	    et1->tm_sec == et2->tm_sec &&
	    et1->tm_min == et2->tm_min &&
	    et1->tm_hour == et2->tm_hour &&
	    et1->tm_mday == et2->tm_mday &&
	    et1->tm_month == et2->tm_month &&
	    et1->tm_year == et2->tm_year &&
	    et1->tm_wday == et2->tm_wday &&
	    et1->tm_yday == et2->tm_yday &&
	    et1->tm_params.tp_gmt_offset == et2->tm_params.tp_gmt_offset &&
	    et1->tm_params.tp_dst_offset == et2->tm_params.tp_dst_offset) {
        return 1;
    } else {
	return 0;
    }
}



































static PRTime prt[] = {
    LL_INIT(220405, 2133125120),  
    LL_INIT(220425, 2633779200),  
    LL_INIT(221612, 2107598848),  
    LL_INIT(228975, 663398400),  
    LL_INIT(258365, 1974568960),  
    LL_INIT(218132, 1393788928),  
    
    LL_INIT( 213062, 4077979648 ), 
    LL_INIT( 218152, 1894443008 ), 
    LL_INIT( 221592, 1606944768 ), 
    LL_INIT( 227768, 688924672 ), 
    LL_INIT( 227788, 1189578752 ), 
};

static PRExplodedTime gmt[] = {
    { 0, 0, 0, 10, 31, 11, 1999, 5, 364, {0, 0}}, 
    { 0, 0, 0, 10, 1, 0, 2000, 6, 0, {0, 0}}, 
    { 0, 0, 0, 10, 29, 1, 2000, 2, 59, {0, 0}}, 
    { 0, 0, 0, 10, 1, 2, 2001, 4, 59, {0, 0}}, 
    { 0, 0, 0, 10, 1, 2, 2005, 2, 59, {0, 0}}, 
    { 0, 0, 0, 10, 9, 8, 1999, 4, 251, {0, 0}},  
    
    { 0, 0, 0, 10, 31, 11, 1998, 4, 364, {0, 0}},  
    { 0, 0, 0, 10, 10, 8, 1999, 5, 252, {0, 0}},  
    { 0, 0, 0, 10, 28, 1, 2000, 1, 58, {0, 0}},  
    { 0, 0, 0, 10, 31, 11, 2000, 0, 365, {0, 0}},  
    { 0, 0, 0, 10, 1, 0, 2001, 1, 0, {0, 0}}  
};

static PRExplodedTime uspt[] = {
{ 0, 0, 0, 2, 31, 11, 1999, 5, 364, {-28800, 0}}, 
{ 0, 0, 0, 2, 1, 0, 2000, 6, 0, {-28800, 0}}, 
{ 0, 0, 0, 2, 29, 1, 2000, 2, 59, {-28800, 0}}, 
{ 0, 0, 0, 2, 1, 2, 2001, 4, 59, {-28800, 0}}, 
{ 0, 0, 0, 2, 1, 2, 2005, 2, 59, {-28800, 0}}, 
{ 0, 0, 0, 3, 9, 8, 1999, 4, 251, {-28800, 3600}},  
    
    { 0, 0, 0, 2, 31, 11, 1998, 4, 364, {-28800, 0}},  
    { 0, 0, 0, 3, 10, 8, 1999, 5, 252, {-28800, 3600}},  
    { 0, 0, 0, 2, 28, 1, 2000, 1, 58, {-28800, 0}},  
    { 0, 0, 0, 2, 31, 11, 2000, 0, 365, {-28800, 0}},  
    { 0, 0, 0, 2, 1, 0, 2001, 1, 0, {-28800, 0}}  
};








static PRExplodedTime localt[] = {
{ 0, 0, 0, 2, 31, 11, 1999, 5, 364, {-28800, 0}}, 
{ 0, 0, 0, 2, 1, 0, 2000, 6, 0, {-28800, 0}}, 
{ 0, 0, 0, 2, 29, 1, 2000, 2, 59, {-28800, 0}}, 
{ 0, 0, 0, 2, 1, 2, 2001, 4, 59, {-28800, 0}}, 
{ 0, 0, 0, 2, 1, 2, 2005, 2, 59, {-28800, 0}}, 
{ 0, 0, 0, 3, 9, 8, 1999, 4, 251, {-28800, 3600}},  
    
    { 0, 0, 0, 2, 31, 11, 1998, 4, 364, {-28800, 0}},  
    { 0, 0, 0, 3, 10, 8, 1999, 5, 252, {-28800, 3600}},  
    { 0, 0, 0, 2, 28, 1, 2000, 1, 58, {-28800, 0}},  
    { 0, 0, 0, 2, 31, 11, 2000, 0, 365, {-28800, 0}},  
    { 0, 0, 0, 2, 1, 0, 2001, 1, 0, {-28800, 0}}  
};

#ifdef US_EASTERN_TIME
static PRExplodedTime localt[] = {
{ 0, 0, 0, 5, 31, 11, 1999, 5, 364, {-18000, 0}}, 
{ 0, 0, 0, 5, 1, 0, 2000, 6, 0, {-18000, 0}}, 
{ 0, 0, 0, 5, 29, 1, 2000, 2, 59, {-18000, 0}}, 
{ 0, 0, 0, 5, 1, 2, 2001, 4, 59, {-18000, 0}}, 
{ 0, 0, 0, 5, 1, 2, 2005, 2, 59, {-18000, 0}}, 
{ 0, 0, 0, 6, 9, 8, 1999, 4, 251, {-18000, 3600}},  
    
    { 0, 0, 0, 5, 31, 11, 1998, 4, 364, {-18000 0}},  
    { 0, 0, 0, 6, 10, 8, 1999, 5, 252, {-18000 3600}},  
    { 0, 0, 0, 5, 28, 1, 2000, 1, 58, {-18000 0}},  
    { 0, 0, 0, 5, 31, 11, 2000, 0, 365, {-18000 0}},  
    { 0, 0, 0, 5, 1, 0, 2001, 1, 0, {-18000 0}}  
};
#endif

static PRStatus TestExplodeImplodeTime(void)
{
    PRTime prt_tmp;
    PRTime now;
    int idx;
    int array_size = sizeof(prt) / sizeof(PRTime);
    PRExplodedTime et_tmp;
    char buf[1024];

    for (idx = 0; idx < array_size; idx++) {
        PR_snprintf(buf, sizeof(buf), "%lld", prt[idx]);
        if (debug_mode) printf("Time stamp %s\n", buf); 
        PR_ExplodeTime(prt[idx], PR_GMTParameters, &et_tmp);
        if (!ExplodedTimeIsEqual(&et_tmp, &gmt[idx])) {
            fprintf(stderr, "GMT not equal\n");
            PrintExplodedTime(&et_tmp);
            PrintExplodedTime(&gmt[idx]);
            exit(1);
        }
        prt_tmp = PR_ImplodeTime(&et_tmp);
        if (LL_NE(prt_tmp, prt[idx])) {
            fprintf(stderr, "PRTime not equal\n");
            exit(1);
        }
        if (debug_mode) {
            printf("GMT: ");
            PrintExplodedTime(&et_tmp);
            printf("\n");
        }

        PR_ExplodeTime(prt[idx], PR_USPacificTimeParameters, &et_tmp);
        if (!ExplodedTimeIsEqual(&et_tmp, &uspt[idx])) {
            fprintf(stderr, "US Pacific Time not equal\n");
            PrintExplodedTime(&et_tmp);
            PrintExplodedTime(&uspt[idx]);
            exit(1);
        }
        prt_tmp = PR_ImplodeTime(&et_tmp);
        if (LL_NE(prt_tmp, prt[idx])) {
            fprintf(stderr, "PRTime not equal\n");
            exit(1);
        }
        if (debug_mode) {
            printf("US Pacific Time: ");
            PrintExplodedTime(&et_tmp);
            printf("\n");
        }

        PR_ExplodeTime(prt[idx], PR_LocalTimeParameters, &et_tmp);
        if (!ExplodedTimeIsEqual(&et_tmp, &localt[idx])) {
            fprintf(stderr, "not equal\n");
            PrintExplodedTime(&et_tmp);
            PrintExplodedTime(&localt[idx]);
            exit(1);
        }
        prt_tmp = PR_ImplodeTime(&et_tmp);
        if (LL_NE(prt_tmp, prt[idx])) {
            fprintf(stderr, "not equal\n");
            exit(1);
        }
        if (debug_mode) {
            printf("Local time:");
            PrintExplodedTime(&et_tmp);
            printf("\n\n");
        }
    }

    now = PR_Now();
    PR_ExplodeTime(now, PR_GMTParameters, &et_tmp);
    printf("Current GMT is ");
    PrintExplodedTime(&et_tmp);
    printf("\n");
    prt_tmp = PR_ImplodeTime(&et_tmp);
    if (LL_NE(prt_tmp, now)) {
        fprintf(stderr, "not equal\n");
        exit(1);
    }
    PR_ExplodeTime(now, PR_USPacificTimeParameters, &et_tmp);
    printf("Current US Pacific Time is ");
    PrintExplodedTime(&et_tmp);
    printf("\n");
    prt_tmp = PR_ImplodeTime(&et_tmp);
    if (LL_NE(prt_tmp, now)) {
        fprintf(stderr, "not equal\n");
        exit(1);
    }
    PR_ExplodeTime(now, PR_LocalTimeParameters, &et_tmp);
    printf("Current local time is ");
    PrintExplodedTime(&et_tmp);
    printf("\n");
    prt_tmp = PR_ImplodeTime(&et_tmp);
    if (LL_NE(prt_tmp, now)) {
        fprintf(stderr, "not equal\n");
        exit(1);
    }
    printf("Please verify the results\n\n");

    if (debug_mode) printf("Test 1 passed\n");
    return PR_SUCCESS;
}









typedef struct time_increment {
	PRInt32 ti_usec;
	PRInt32 ti_sec;
	PRInt32 ti_min;
	PRInt32 ti_hour;
} time_increment_t;






typedef struct normalize_test_data {
    PRExplodedTime		base_time; 
    time_increment_t  	increment;
    PRExplodedTime		expected_gmt_time;
    PRExplodedTime		expected_uspt_time;
} normalize_test_data_t;







normalize_test_data_t normalize_test_array[] = {
  

	
	{{0, 48, 32, 19, 31, 11, 1999, 5, 364, { -28800, 0}},
    {0, 0, 30, 20},
	{0, 48, 2, 0, 2, 0, 2000, 0, 1, { 0, 0}},	
	{0, 48, 2, 16, 1, 0, 2000, 6, 0, { -28800, 0}},

	},
	
	{{0, 2, 59, 23, 31, 11, 1999, 5, 364, { 0, 0}},
    {0, 0, 45, 0},
	{0, 2, 44, 0, 1, 0, 2000, 6, 0, { 0, 0}},
	{0, 2, 44, 16, 31, 11, 1999, 5, 364, { -28800, 0}}

	},
	
	{{0, 0, 0, 12, 25, 11, 1999, 6, 358, { 0, 0}},
    {0, 0, 0, 364 * 24},
	{0, 0, 0, 12, 23, 11, 2000, 6, 357, { 0, 0}},

	{0, 0, 0, 4, 23, 11, 2000, 6, 357, { -28800, 0}}

	},
	
    {{0, 0, 0, 0, 1, 0, 2000, 6, 0, { -28800, 0}},
    {0, 0, 0, 48},
    {0, 0, 0, 8, 3, 0, 2000, 1, 2, { 0, 0}},
    {0, 0, 0, 0, 3, 0, 2000, 1, 2, { -28800, 0}}

	},
	
    {{0, 0, 0, 12, 10, 0, 2000, 1, 9, { -28800, 0}},
    {0, 0, 0, 364 * 5 * 24},
    {0, 0, 0, 20, 3, 0, 2005, 1, 2, { 0, 0}},
    {0, 0, 0, 12, 3, 0, 2005, 1, 2, { -28800, 0}}

	},
	
	{{0, 0, 39, 15, 28, 1, 2000, 1, 58, { 0, 0}},
    {0,  0, 0, 24},
	{0, 0, 39, 15, 29, 1, 2000, 2, 59, { 0, 0}}, 

	{0, 0, 39, 7, 29, 1, 2000, 2, 59, { -28800, 0}}

	},
	
    {{0, 0, 0, 12, 3, 0, 2001, 3, 2, { -28800, 0}},

    {0, 30, 30,45},
    {0, 30, 30, 17, 5, 0, 2001, 5, 4, { 0, 0}}, 

    {0, 30, 30, 9, 5, 0, 2001, 5, 4, { -28800, 0}} 

	},
	
	{{0, 0, 0, 20, 3, 0, 2001, 3, 2, { 0, 0}},
    {0, 0, 30,0},
	{0, 0, 30, 20, 3, 0, 2001, 3, 2, { 0, 0}}, 
    {0, 0, 30, 12, 3, 0, 2001, 3, 2, { -28800, 0}}

	},
	
	{{0, 0, 0, 0, 9, 8, 1999, 4, 251, { 0, 0}},
    {0, 0, 0, 12},
    {0, 0, 0, 12, 9, 8, 1999, 4, 251, { 0, 0}},
    {0, 0, 0, 5, 9, 8, 1999, 4, 251, { -28800, 3600}}

	}
};

void add_time_increment(PRExplodedTime *et1, time_increment_t *it)
{
	et1->tm_usec += it->ti_usec;
	et1->tm_sec	+= it->ti_sec;
	et1->tm_min += it->ti_min;
	et1->tm_hour += it->ti_hour;
}









PRStatus TestNormalizeTime(void)
{
int idx, count;
normalize_test_data_t *itemp;
time_increment_t *itp;

	count = sizeof(normalize_test_array)/sizeof(normalize_test_array[0]);
	for (idx = 0; idx < count; idx++) {
		itemp = &normalize_test_array[idx];
		if (debug_mode) {
			printf("%2d. %15s",idx +1,"Base time: ");
			PrintExplodedTime(&itemp->base_time);
			printf("\n");
		}
		itp = &itemp->increment;
		if (debug_mode) {
			printf("%20s %2d hrs %2d min %3d sec\n","Add",itp->ti_hour,
												itp->ti_min, itp->ti_sec);
		}
		add_time_increment(&itemp->base_time, &itemp->increment);
		PR_NormalizeTime(&itemp->base_time, PR_LocalTimeParameters);
		if (debug_mode) {
			printf("%19s","PST time: ");
			PrintExplodedTime(&itemp->base_time);
			printf("\n");
		}
		if (!ExplodedTimeIsEqual(&itemp->base_time,
									&itemp->expected_uspt_time)) {
			printf("PR_NormalizeTime failed\n");
			if (debug_mode)
				PrintExplodedTime(&itemp->expected_uspt_time);
			return PR_FAILURE;
		}
		PR_NormalizeTime(&itemp->base_time, PR_GMTParameters);
		if (debug_mode) {
			printf("%19s","GMT time: ");
			PrintExplodedTime(&itemp->base_time);
			printf("\n");
		}

		if (!ExplodedTimeIsEqual(&itemp->base_time,
									&itemp->expected_gmt_time)) {
			printf("PR_NormalizeTime failed\n");
			return PR_FAILURE;
		}
	}
	return PR_SUCCESS;
}






typedef struct ParseTest
{
    char            *sDate;     
    PRExplodedTime  et;         
} ParseTest;

static ParseTest parseArray[] = 
{
    
    
    { "Thursday 1 Jan 1970 00:00:00",   { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "1 Jan 1970 00:00:00",            { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "1-Jan-1970 00:00:00",            { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "01-Jan-1970 00:00:00",           { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "January 1, 1970",                { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "January 1, 1970 00:00",          { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "January 01, 1970 00:00",         { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "January 01 1970 00:00",          { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "January 01 1970 00:00:00",       { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "01-01-1970",                     { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "01/01/1970",                     { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "01/01/70",                       { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "01/01/70 00:00:00",              { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "70/01/01 00:00:00",              { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "70/1/1 00:00:",                  { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "00:00 Thursday, January 1, 1970",{ 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "1-Jan-70 00:00:00",              { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "70-01-01 00:00:00",              { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},
    { "70/01/01 00:00:00",              { 000000,  00, 00, 00,     1,   0, 1970, 4,     0,   {-28800, 0 }}},

    
    { "Wed 31 Dec 1969 00:00:00",       { 000000,  00, 00, 00,    31,  11, 1969, 3,   364,   {-28800, 0 }}},
    { "31 Dec 1969 00:00:00",           { 000000,  00, 00, 00,    31,  11, 1969, 3,   364,   {-28800, 0 }}},
    { "12/31/69    00:00:00",           { 000000,  00, 00, 00,    31,  11, 2069, 2,   364,   {-28800, 0 }}},
    { "12/31/1969  00:00:00",           { 000000,  00, 00, 00,    31,  11, 1969, 3,   364,   {-28800, 0 }}},
    { "12-31-69    00:00:00",           { 000000,  00, 00, 00,    31,  11, 2069, 2,   364,   {-28800, 0 }}},
    { "12-31-1969  00:00:00",           { 000000,  00, 00, 00,    31,  11, 1969, 3,   364,   {-28800, 0 }}},
    { "69-12-31    00:00:00",           { 000000,  00, 00, 00,    31,  11, 2069, 2,   364,   {-28800, 0 }}},
    { "69/12/31    00:00:00",           { 000000,  00, 00, 00,    31,  11, 2069, 2,   364,   {-28800, 0 }}},

     
    { "Thu 31 Dec 1998 00:00:00",        { 00000,  00, 00, 00,    31,  11, 1998, 4,   364,    {-28800, 0 }}},
    { "12/31/98    00:00:00",            { 00000,  00, 00, 00,    31,  11, 1998, 4,   364,    {-28800, 0 }}},
    { "12/31/1998  00:00:00",            { 00000,  00, 00, 00,    31,  11, 1998, 4,   364,    {-28800, 0 }}},
    { "12-31-98    00:00:00",            { 00000,  00, 00, 00,    31,  11, 1998, 4,   364,    {-28800, 0 }}},
    { "12-31-1998  00:00:00",            { 00000,  00, 00, 00,    31,  11, 1998, 4,   364,    {-28800, 0 }}},
    { "98-12-31    00:00:00",            { 00000,  00, 00, 00,    31,  11, 1998, 4,   364,    {-28800, 0 }}},
    { "98/12/31    00:00:00",            { 00000,  00, 00, 00,    31,  11, 1998, 4,   364,    {-28800, 0 }}},

    
    { "09 Sep 1999 00:00:00",           { 000000,  00, 00, 00,     9,   8, 1999, 4,   251,   {-28800, 3600 }}},
    { "9/9/99      00:00:00",           { 000000,  00, 00, 00,     9,   8, 1999, 4,   251,   {-28800, 3600 }}},
    { "9/9/1999    00:00:00",           { 000000,  00, 00, 00,     9,   8, 1999, 4,   251,   {-28800, 3600 }}},
    { "9-9-99      00:00:00",           { 000000,  00, 00, 00,     9,   8, 1999, 4,   251,   {-28800, 3600 }}},
    { "9-9-1999    00:00:00",           { 000000,  00, 00, 00,     9,   8, 1999, 4,   251,   {-28800, 3600 }}},
    { "09-09-99    00:00:00",           { 000000,  00, 00, 00,     9,   8, 1999, 4,   251,   {-28800, 3600 }}},
    { "09-09-1999  00:00:00",           { 000000,  00, 00, 00,     9,   8, 1999, 4,   251,   {-28800, 3600 }}},
    { "99-09-09    00:00:00",           { 000000,  00, 00, 00,     9,   8, 1999, 4,   251,   {-28800, 3600 }}},
    
    
    { "10 Sep 1999 00:00:00",           { 000000,  00, 00, 00,    10,   8, 1999, 5,   252,   {-28800, 3600 }}},
    { "9/10/99     00:00:00",           { 000000,  00, 00, 00,    10,   8, 1999, 5,   252,   {-28800, 3600 }}},
    { "9/10/1999   00:00:00",           { 000000,  00, 00, 00,    10,   8, 1999, 5,   252,   {-28800, 3600 }}},
    { "9-10-99     00:00:00",           { 000000,  00, 00, 00,    10,   8, 1999, 5,   252,   {-28800, 3600 }}},
    { "9-10-1999   00:00:00",           { 000000,  00, 00, 00,    10,   8, 1999, 5,   252,   {-28800, 3600 }}},
    { "09-10-99    00:00:00",           { 000000,  00, 00, 00,    10,   8, 1999, 5,   252,   {-28800, 3600 }}},
    { "09-10-1999  00:00:00",           { 000000,  00, 00, 00,    10,   8, 1999, 5,   252,   {-28800, 3600 }}},
    { "99-09-10    00:00:00",           { 000000,  00, 00, 00,    10,   8, 1999, 5,   252,   {-28800, 3600 }}},

    
    { "31 Dec 1999 00:00:00",           { 000000,  00, 00, 00,    31,  11, 1999, 5,   364,   {-28800, 0 }}},
    { "12/31/99    00:00:00",           { 000000,  00, 00, 00,    31,  11, 1999, 5,   364,   {-28800, 0 }}},
    { "12/31/1999  00:00:00",           { 000000,  00, 00, 00,    31,  11, 1999, 5,   364,   {-28800, 0 }}},
    { "12-31-99    00:00:00",           { 000000,  00, 00, 00,    31,  11, 1999, 5,   364,   {-28800, 0 }}},
    { "12-31-1999  00:00:00",           { 000000,  00, 00, 00,    31,  11, 1999, 5,   364,   {-28800, 0 }}},
    { "99-12-31    00:00:00",           { 000000,  00, 00, 00,    31,  11, 1999, 5,   364,   {-28800, 0 }}},
    { "99/12/31    00:00:00",           { 000000,  00, 00, 00,    31,  11, 1999, 5,   364,   {-28800, 0 }}},

    
    { "01 Jan 2000 00:00:00",           { 000000,  00, 00, 00,     1,   0, 2000, 6,     0,   {-28800, 0 }}},
    { "1/1/00      00:00:00",           { 000000,  00, 00, 00,     1,   0, 2000, 6,     0,   {-28800, 0 }}},
    { "1/1/2000    00:00:00",           { 000000,  00, 00, 00,     1,   0, 2000, 6,     0,   {-28800, 0 }}},
    { "1-1-00      00:00:00",           { 000000,  00, 00, 00,     1,   0, 2000, 6,     0,   {-28800, 0 }}},
    { "1-1-2000    00:00:00",           { 000000,  00, 00, 00,     1,   0, 2000, 6,     0,   {-28800, 0 }}},
    { "01-01-00    00:00:00",           { 000000,  00, 00, 00,     1,   0, 2000, 6,     0,   {-28800, 0 }}},
    { "Saturday 01-01-2000  00:00:00",  { 000000,  00, 00, 00,     1,   0, 2000, 6,     0,   {-28800, 0 }}},

    
    { "28 Feb 2000 00:00:00",           { 000000,  00, 00, 00,    28,   1, 2000, 1,    58,   {-28800, 0 }}},
    { "2/28/00     00:00:00",           { 000000,  00, 00, 00,    28,   1, 2000, 1,    58,   {-28800, 0 }}},
    { "2/28/2000   00:00:00",           { 000000,  00, 00, 00,    28,   1, 2000, 1,    58,   {-28800, 0 }}},
    { "2-28-00     00:00:00",           { 000000,  00, 00, 00,    28,   1, 2000, 1,    58,   {-28800, 0 }}},
    { "2-28-2000   00:00:00",           { 000000,  00, 00, 00,    28,   1, 2000, 1,    58,   {-28800, 0 }}},
    { "02-28-00    00:00:00",           { 000000,  00, 00, 00,    28,   1, 2000, 1,    58,   {-28800, 0 }}},
    { "02-28-2000  00:00:00",           { 000000,  00, 00, 00,    28,   1, 2000, 1,    58,   {-28800, 0 }}},

    
    { "29 Feb 2000 00:00:00",           { 000000,  00, 00, 00,    29,   1, 2000, 2,    59,   {-28800, 0 }}},
    { "2/29/00     00:00:00",           { 000000,  00, 00, 00,    29,   1, 2000, 2,    59,   {-28800, 0 }}},
    { "2/29/2000   00:00:00",           { 000000,  00, 00, 00,    29,   1, 2000, 2,    59,   {-28800, 0 }}},
    { "2-29-00     00:00:00",           { 000000,  00, 00, 00,    29,   1, 2000, 2,    59,   {-28800, 0 }}},
    { "2-29-2000   00:00:00",           { 000000,  00, 00, 00,    29,   1, 2000, 2,    59,   {-28800, 0 }}},
    { "02-29-00    00:00:00",           { 000000,  00, 00, 00,    29,   1, 2000, 2,    59,   {-28800, 0 }}},
    { "02-29-2000  00:00:00",           { 000000,  00, 00, 00,    29,   1, 2000, 2,    59,   {-28800, 0 }}},

    
    { "01 Mar 2000 00:00:00",           { 000000,  00, 00, 00,     1,   2, 2000, 3,    60,   {-28800, 0 }}},
    { "3/1/00      00:00:00",           { 000000,  00, 00, 00,     1,   2, 2000, 3,    60,   {-28800, 0 }}},
    { "3/1/2000    00:00:00",           { 000000,  00, 00, 00,     1,   2, 2000, 3,    60,   {-28800, 0 }}},
    { "3-1-00      00:00:00",           { 000000,  00, 00, 00,     1,   2, 2000, 3,    60,   {-28800, 0 }}},
    { "03-01-00    00:00:00",           { 000000,  00, 00, 00,     1,   2, 2000, 3,    60,   {-28800, 0 }}},
    { "03-01-2000  00:00:00",           { 000000,  00, 00, 00,     1,   2, 2000, 3,    60,   {-28800, 0 }}},

    
    { "31 Dec 2000 00:00:00",           { 000000,  00, 00, 00,    31,  11, 2000, 0,   365,   {-28800, 0 }}},
    { "12/31/00    00:00:00",           { 000000,  00, 00, 00,    31,  11, 2000, 0,   365,   {-28800, 0 }}},
    { "12/31/2000  00:00:00",           { 000000,  00, 00, 00,    31,  11, 2000, 0,   365,   {-28800, 0 }}},
    { "12-31-00    00:00:00",           { 000000,  00, 00, 00,    31,  11, 2000, 0,   365,   {-28800, 0 }}},
    { "12-31-2000  00:00:00",           { 000000,  00, 00, 00,    31,  11, 2000, 0,   365,   {-28800, 0 }}},
    { "00-12-31    00:00:00",           { 000000,  00, 00, 00,    31,  11, 2000, 0,   365,   {-28800, 0 }}},
    { "00/12/31    00:00:00",           { 000000,  00, 00, 00,    31,  11, 2000, 0,   365,   {-28800, 0 }}},

    
    { "01 Jan 2001 00:00:00",           { 000000,  00, 00, 00,     1,   0, 2001, 1,     0,   {-28800, 0 }}},
    { "1/1/01      00:00:00",           { 000000,  00, 00, 00,     1,   0, 2001, 1,     0,   {-28800, 0 }}},
    { "1/1/2001    00:00:00",           { 000000,  00, 00, 00,     1,   0, 2001, 1,     0,   {-28800, 0 }}},
    { "1-1-01      00:00:00",           { 000000,  00, 00, 00,     1,   0, 2001, 1,     0,   {-28800, 0 }}},
    { "1-1-2001    00:00:00",           { 000000,  00, 00, 00,     1,   0, 2001, 1,     0,   {-28800, 0 }}},
    { "01-01-01    00:00:00",           { 000000,  00, 00, 00,     1,   0, 2001, 1,     0,   {-28800, 0 }}},
    { "Saturday 01-01-2001  00:00:00",  { 000000,  00, 00, 00,     1,   0, 2001, 1,     0,   {-28800, 0 }}},

    
    { "01 Mar 2001 00:00:00",           { 000000,  00, 00, 00,     1,   2, 2001, 4,    59,   {-28800, 0 }}},
    { "3/1/01      00:00:00",           { 000000,  00, 00, 00,     1,   2, 2001, 4,    59,   {-28800, 0 }}},
    { "3/1/2001    00:00:00",           { 000000,  00, 00, 00,     1,   2, 2001, 4,    59,   {-28800, 0 }}},
    { "3-1-01      00:00:00",           { 000000,  00, 00, 00,     1,   2, 2001, 4,    59,   {-28800, 0 }}},
    { "3-1-2001    00:00:00",           { 000000,  00, 00, 00,     1,   2, 2001, 4,    59,   {-28800, 0 }}},
    { "03-01-01    00:00:00",           { 000000,  00, 00, 00,     1,   2, 2001, 4,    59,   {-28800, 0 }}},
    { "03-01-2001  00:00:00",           { 000000,  00, 00, 00,     1,   2, 2001, 4,    59,   {-28800, 0 }}},

    
    { "29 Feb 2004 00:00:00",           { 000000,  00, 00, 00,    29,   1, 2004, 0,    59,   {-28800, 0 }}},
    { "2/29/04     00:00:00",           { 000000,  00, 00, 00,    29,   1, 2004, 0,    59,   {-28800, 0 }}},
    { "2/29/2004   00:00:00",           { 000000,  00, 00, 00,    29,   1, 2004, 0,    59,   {-28800, 0 }}},
    { "2-29-04     00:00:00",           { 000000,  00, 00, 00,    29,   1, 2004, 0,    59,   {-28800, 0 }}},
    { "2-29-2004   00:00:00",           { 000000,  00, 00, 00,    29,   1, 2004, 0,    59,   {-28800, 0 }}},

    
    { "01 Mar 2004 00:00:00",           { 000000,  00, 00, 00,     1,   2, 2004, 1,    60,   {-28800, 0 }}},
    { "3/1/04      00:00:00",           { 000000,  00, 00, 00,     1,   2, 2004, 1,    60,   {-28800, 0 }}},
    { "3/1/2004    00:00:00",           { 000000,  00, 00, 00,     1,   2, 2004, 1,    60,   {-28800, 0 }}},
    { "3-1-04      00:00:00",           { 000000,  00, 00, 00,     1,   2, 2004, 1,    60,   {-28800, 0 }}},
    { "3-1-2004    00:00:00",           { 000000,  00, 00, 00,     1,   2, 2004, 1,    60,   {-28800, 0 }}},
    { "03-01-04    00:00:00",           { 000000,  00, 00, 00,     1,   2, 2004, 1,    60,   {-28800, 0 }}},
    { "03-01-2004  00:00:00",           { 000000,  00, 00, 00,     1,   2, 2004, 1,    60,   {-28800, 0 }}},

    
    { "01 Mar 2005 00:00:00",           { 000000,  00, 00, 00,     1,   2, 2005, 2,    59,   {-28800, 0 }}},
    { "3/1/05      00:00:00",           { 000000,  00, 00, 00,     1,   2, 2005, 2,    59,   {-28800, 0 }}},
    { "3/1/2005    00:00:00",           { 000000,  00, 00, 00,     1,   2, 2005, 2,    59,   {-28800, 0 }}},
    { "3-1-05      00:00:00",           { 000000,  00, 00, 00,     1,   2, 2005, 2,    59,   {-28800, 0 }}},
    { "3-1-2005    00:00:00",           { 000000,  00, 00, 00,     1,   2, 2005, 2,    59,   {-28800, 0 }}},
    { "03-01-05    00:00:00",           { 000000,  00, 00, 00,     1,   2, 2005, 2,    59,   {-28800, 0 }}},
    { "03-01-2005  00:00:00",           { 000000,  00, 00, 00,     1,   2, 2005, 2,    59,   {-28800, 0 }}},

    
    { NULL }        
}; 















static PRStatus TestParseTime( void )
{
    ParseTest       *ptp = parseArray;
    PRTime          ct;
    PRExplodedTime  cet;
    char            *sp = ptp->sDate;
    PRStatus        rc;
    PRStatus        rv = PR_SUCCESS;

    while ( sp != NULL)
    {
        rc = PR_ParseTimeString( sp, PR_FALSE, &ct );
        if ( PR_FAILURE == rc )
        {
            printf("TestParseTime(): PR_ParseTimeString() failed to convert: %s\n", sp );
            rv = PR_FAILURE;
            failed_already = 1;
        }
        else
        {
            PR_ExplodeTime( ct, PR_LocalTimeParameters , &cet );

            if ( !ExplodedTimeIsEqual( &cet, &ptp->et ))
            {
                printf("TestParseTime(): Exploded time compare failed: %s\n", sp );
                if ( debug_mode )
                {
                    PrintExplodedTime( &cet );
                    printf("\n");
                    PrintExplodedTime( &ptp->et );
                    printf("\n");
                }
                
                rv = PR_FAILURE;
                failed_already = 1;
            }
        }
                
        
        ptp++;
        sp = ptp->sDate;
    } 

    return( rv );
} 

int main(int argc, char** argv)
{
	





	PLOptStatus os;
	PLOptState *opt;
    
    PR_STDIO_INIT();
	opt = PL_CreateOptState(argc, argv, "d");
	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
		if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'd':  
			debug_mode = PR_TRUE;
            break;
         default:
            break;
        }
    }
	PL_DestroyOptState(opt);

 
	
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    lm = PR_NewLogModule("test");

    if ( PR_FAILURE == TestExplodeImplodeTime())
    {
        PR_LOG( lm, PR_LOG_ERROR,
            ("TestExplodeImplodeTime() failed"));
    }
    else
    	printf("Test 1: Calendar Time Test passed\n");

    if ( PR_FAILURE == TestNormalizeTime())
    {
        PR_LOG( lm, PR_LOG_ERROR,
            ("TestNormalizeTime() failed"));
    }
    else
    	printf("Test 2: Normalize Time Test passed\n");

    if ( PR_FAILURE == TestParseTime())
    {
        PR_LOG( lm, PR_LOG_ERROR,
            ("TestParseTime() failed"));
    }
    else
    	printf("Test 3: Parse Time Test passed\n");

	if (failed_already) 
	    return 1;
	else 
	    return 0;
} 

