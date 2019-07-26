






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "debug.h"
#include "SigProc_FIX.h"

#if SILK_TIC_TOC

#ifdef _WIN32

#if (defined(_WIN32) || defined(_WINCE))
#include <windows.h>    
#else   
#include <sys/time.h>
#endif

unsigned long silk_GetHighResolutionTime(void) 
{
    
    
    
    LARGE_INTEGER lpPerformanceCount;
    LARGE_INTEGER lpFrequency;
    QueryPerformanceCounter(&lpPerformanceCount);
    QueryPerformanceFrequency(&lpFrequency);
    return (unsigned long)((1000000*(lpPerformanceCount.QuadPart)) / lpFrequency.QuadPart);
}
#else   
unsigned long GetHighResolutionTime(void) 
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return((tv.tv_sec*1000000)+(tv.tv_usec));
}
#endif

int           silk_Timer_nTimers = 0;
int           silk_Timer_depth_ctr = 0;
char          silk_Timer_tags[silk_NUM_TIMERS_MAX][silk_NUM_TIMERS_MAX_TAG_LEN];
#ifdef WIN32
LARGE_INTEGER silk_Timer_start[silk_NUM_TIMERS_MAX];
#else
unsigned long silk_Timer_start[silk_NUM_TIMERS_MAX];
#endif
unsigned int  silk_Timer_cnt[silk_NUM_TIMERS_MAX];
opus_int64     silk_Timer_min[silk_NUM_TIMERS_MAX];
opus_int64     silk_Timer_sum[silk_NUM_TIMERS_MAX];
opus_int64     silk_Timer_max[silk_NUM_TIMERS_MAX];
opus_int64     silk_Timer_depth[silk_NUM_TIMERS_MAX];

#ifdef WIN32
void silk_TimerSave(char *file_name)
{
    if( silk_Timer_nTimers > 0 )
    {
        int k;
        FILE *fp;
        LARGE_INTEGER lpFrequency;
        LARGE_INTEGER lpPerformanceCount1, lpPerformanceCount2;
        int del = 0x7FFFFFFF;
        double avg, sum_avg;
        
        for( k = 0; k < 1000; k++ ) {
            QueryPerformanceCounter(&lpPerformanceCount1);
            QueryPerformanceCounter(&lpPerformanceCount2);
            lpPerformanceCount2.QuadPart -= lpPerformanceCount1.QuadPart;
            if( (int)lpPerformanceCount2.LowPart < del )
                del = lpPerformanceCount2.LowPart;
        }
        QueryPerformanceFrequency(&lpFrequency);
        
        sum_avg = 0.0f;
        for( k = 0; k < silk_Timer_nTimers; k++ ) {
            if (silk_Timer_depth[k] == 0) {
                sum_avg += (1e6 * silk_Timer_sum[k] / silk_Timer_cnt[k] - del) / lpFrequency.QuadPart * silk_Timer_cnt[k];
            }
        }
        fp = fopen(file_name, "w");
        fprintf(fp, "                                min         avg     %%         max      count\n");
        for( k = 0; k < silk_Timer_nTimers; k++ ) {
            if (silk_Timer_depth[k] == 0) {
                fprintf(fp, "%-28s", silk_Timer_tags[k]);
            } else if (silk_Timer_depth[k] == 1) {
                fprintf(fp, " %-27s", silk_Timer_tags[k]);
            } else if (silk_Timer_depth[k] == 2) {
                fprintf(fp, "  %-26s", silk_Timer_tags[k]);
            } else if (silk_Timer_depth[k] == 3) {
                fprintf(fp, "   %-25s", silk_Timer_tags[k]);
            } else {
                fprintf(fp, "    %-24s", silk_Timer_tags[k]);
            }
            avg = (1e6 * silk_Timer_sum[k] / silk_Timer_cnt[k] - del) / lpFrequency.QuadPart;
            fprintf(fp, "%8.2f", (1e6 * (silk_max_64(silk_Timer_min[k] - del, 0))) / lpFrequency.QuadPart);
            fprintf(fp, "%12.2f %6.2f", avg, 100.0 * avg / sum_avg * silk_Timer_cnt[k]);
            fprintf(fp, "%12.2f", (1e6 * (silk_max_64(silk_Timer_max[k] - del, 0))) / lpFrequency.QuadPart);
            fprintf(fp, "%10d\n", silk_Timer_cnt[k]);
        }
        fprintf(fp, "                                microseconds\n");
        fclose(fp);
    }
}
#else
void silk_TimerSave(char *file_name)
{
    if( silk_Timer_nTimers > 0 )
    {
        int k;
        FILE *fp;
        
        fp = fopen(file_name, "w");
        fprintf(fp, "                                min         avg         max      count\n");
        for( k = 0; k < silk_Timer_nTimers; k++ )
        {
            if (silk_Timer_depth[k] == 0) {
                fprintf(fp, "%-28s", silk_Timer_tags[k]);
            } else if (silk_Timer_depth[k] == 1) {
                fprintf(fp, " %-27s", silk_Timer_tags[k]);
            } else if (silk_Timer_depth[k] == 2) {
                fprintf(fp, "  %-26s", silk_Timer_tags[k]);
            } else if (silk_Timer_depth[k] == 3) {
                fprintf(fp, "   %-25s", silk_Timer_tags[k]);
            } else {
                fprintf(fp, "    %-24s", silk_Timer_tags[k]);
            }
            fprintf(fp, "%d ", silk_Timer_min[k]);
            fprintf(fp, "%f ", (double)silk_Timer_sum[k] / (double)silk_Timer_cnt[k]);
            fprintf(fp, "%d ", silk_Timer_max[k]);
            fprintf(fp, "%10d\n", silk_Timer_cnt[k]);
        }
        fprintf(fp, "                                microseconds\n");
        fclose(fp);
    }
}
#endif

#endif 

#if SILK_DEBUG
FILE *silk_debug_store_fp[ silk_NUM_STORES_MAX ];
int silk_debug_store_count = 0;
#endif 

