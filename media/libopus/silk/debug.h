






























#ifndef SILK_DEBUG_H
#define SILK_DEBUG_H

#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE    1
#endif

#include "typedef.h"
#include <stdio.h>      
#include <string.h>     

#ifdef  __cplusplus
extern "C"
{
#endif

unsigned long GetHighResolutionTime(void); 


#if defined _WIN32
    #ifdef _DEBUG
        #define SILK_DEBUG  1
    #else
        #define SILK_DEBUG  0
    #endif

    
    #if 0
    
    #undef  SILK_DEBUG
    #define SILK_DEBUG  1
    #endif
#else
    #define SILK_DEBUG  0
#endif


#define SILK_TIC_TOC    0


#if SILK_TIC_TOC

#if (defined(_WIN32) || defined(_WINCE))
#include <windows.h>    
#pragma warning( disable : 4996 )       /* stop bitching about strcpy in TIC()*/
#else   
#include <sys/time.h>
#endif
















void silk_TimerSave(char *file_name);


#define silk_NUM_TIMERS_MAX                  50

#define silk_NUM_TIMERS_MAX_TAG_LEN          30

extern int           silk_Timer_nTimers;
extern int           silk_Timer_depth_ctr;
extern char          silk_Timer_tags[silk_NUM_TIMERS_MAX][silk_NUM_TIMERS_MAX_TAG_LEN];
#ifdef _WIN32
extern LARGE_INTEGER silk_Timer_start[silk_NUM_TIMERS_MAX];
#else
extern unsigned long silk_Timer_start[silk_NUM_TIMERS_MAX];
#endif
extern unsigned int  silk_Timer_cnt[silk_NUM_TIMERS_MAX];
extern opus_int64    silk_Timer_sum[silk_NUM_TIMERS_MAX];
extern opus_int64    silk_Timer_max[silk_NUM_TIMERS_MAX];
extern opus_int64    silk_Timer_min[silk_NUM_TIMERS_MAX];
extern opus_int64    silk_Timer_depth[silk_NUM_TIMERS_MAX];


#ifdef _WIN32
#define TIC(TAG_NAME) {                                     \
    static int init = 0;                                    \
    static int ID = -1;                                     \
    if( init == 0 )                                         \
    {                                                       \
        int k;                                              \
        init = 1;                                           \
        for( k = 0; k < silk_Timer_nTimers; k++ ) {         \
            if( strcmp(silk_Timer_tags[k], #TAG_NAME) == 0 ) { \
                ID = k;                                     \
                break;                                      \
            }                                               \
        }                                                   \
        if (ID == -1) {                                     \
            ID = silk_Timer_nTimers;                        \
            silk_Timer_nTimers++;                           \
            silk_Timer_depth[ID] = silk_Timer_depth_ctr;    \
            strcpy(silk_Timer_tags[ID], #TAG_NAME);         \
            silk_Timer_cnt[ID] = 0;                         \
            silk_Timer_sum[ID] = 0;                         \
            silk_Timer_min[ID] = 0xFFFFFFFF;                \
            silk_Timer_max[ID] = 0;                         \
        }                                                   \
    }                                                       \
    silk_Timer_depth_ctr++;                                 \
    QueryPerformanceCounter(&silk_Timer_start[ID]);         \
}
#else
#define TIC(TAG_NAME) {                                     \
    static int init = 0;                                    \
    static int ID = -1;                                     \
    if( init == 0 )                                         \
    {                                                       \
        int k;                                              \
        init = 1;                                           \
        for( k = 0; k < silk_Timer_nTimers; k++ ) {         \
        if( strcmp(silk_Timer_tags[k], #TAG_NAME) == 0 ) {  \
                ID = k;                                     \
                break;                                      \
            }                                               \
        }                                                   \
        if (ID == -1) {                                     \
            ID = silk_Timer_nTimers;                        \
            silk_Timer_nTimers++;                           \
            silk_Timer_depth[ID] = silk_Timer_depth_ctr;    \
            strcpy(silk_Timer_tags[ID], #TAG_NAME);         \
            silk_Timer_cnt[ID] = 0;                         \
            silk_Timer_sum[ID] = 0;                         \
            silk_Timer_min[ID] = 0xFFFFFFFF;                \
            silk_Timer_max[ID] = 0;                         \
        }                                                   \
    }                                                       \
    silk_Timer_depth_ctr++;                                 \
    silk_Timer_start[ID] = GetHighResolutionTime();         \
}
#endif

#ifdef _WIN32
#define TOC(TAG_NAME) {                                             \
    LARGE_INTEGER lpPerformanceCount;                               \
    static int init = 0;                                            \
    static int ID = 0;                                              \
    if( init == 0 )                                                 \
    {                                                               \
        int k;                                                      \
        init = 1;                                                   \
        for( k = 0; k < silk_Timer_nTimers; k++ ) {                 \
            if( strcmp(silk_Timer_tags[k], #TAG_NAME) == 0 ) {      \
                ID = k;                                             \
                break;                                              \
            }                                                       \
        }                                                           \
    }                                                               \
    QueryPerformanceCounter(&lpPerformanceCount);                   \
    lpPerformanceCount.QuadPart -= silk_Timer_start[ID].QuadPart;   \
    if((lpPerformanceCount.QuadPart < 100000000) &&                 \
        (lpPerformanceCount.QuadPart >= 0)) {                       \
        silk_Timer_cnt[ID]++;                                       \
        silk_Timer_sum[ID] += lpPerformanceCount.QuadPart;          \
        if( lpPerformanceCount.QuadPart > silk_Timer_max[ID] )      \
            silk_Timer_max[ID] = lpPerformanceCount.QuadPart;       \
        if( lpPerformanceCount.QuadPart < silk_Timer_min[ID] )      \
            silk_Timer_min[ID] = lpPerformanceCount.QuadPart;       \
    }                                                               \
    silk_Timer_depth_ctr--;                                         \
}
#else
#define TOC(TAG_NAME) {                                             \
    unsigned long endTime;                                          \
    static int init = 0;                                            \
    static int ID = 0;                                              \
    if( init == 0 )                                                 \
    {                                                               \
        int k;                                                      \
        init = 1;                                                   \
        for( k = 0; k < silk_Timer_nTimers; k++ ) {                 \
            if( strcmp(silk_Timer_tags[k], #TAG_NAME) == 0 ) {      \
                ID = k;                                             \
                break;                                              \
            }                                                       \
        }                                                           \
    }                                                               \
    endTime = GetHighResolutionTime();                              \
    endTime -= silk_Timer_start[ID];                                \
    if((endTime < 100000000) &&                                     \
        (endTime >= 0)) {                                           \
        silk_Timer_cnt[ID]++;                                       \
        silk_Timer_sum[ID] += endTime;                              \
        if( endTime > silk_Timer_max[ID] )                          \
            silk_Timer_max[ID] = endTime;                           \
        if( endTime < silk_Timer_min[ID] )                          \
            silk_Timer_min[ID] = endTime;                           \
    }                                                               \
        silk_Timer_depth_ctr--;                                     \
}
#endif

#else 


#define TIC(TAG_NAME)
#define TOC(TAG_NAME)
#define silk_TimerSave(FILE_NAME)

#endif 


#if SILK_DEBUG





#define silk_NUM_STORES_MAX                                  100
extern FILE *silk_debug_store_fp[ silk_NUM_STORES_MAX ];
extern int silk_debug_store_count;


#define DEBUG_STORE_DATA( FILE_NAME, DATA_PTR, N_BYTES ) {          \
    static opus_int init = 0, cnt = 0;                              \
    static FILE **fp;                                               \
    if (init == 0) {                                                \
        init = 1;                                                   \
        cnt = silk_debug_store_count++;                             \
        silk_debug_store_fp[ cnt ] = fopen(#FILE_NAME, "wb");       \
    }                                                               \
    fwrite((DATA_PTR), (N_BYTES), 1, silk_debug_store_fp[ cnt ]);   \
}


#define SILK_DEBUG_STORE_CLOSE_FILES {                              \
    opus_int i;                                                     \
    for( i = 0; i < silk_debug_store_count; i++ ) {                 \
        fclose( silk_debug_store_fp[ i ] );                         \
    }                                                               \
}


#define silk_GETTIME(void)       time = (opus_int64) silk_GetHighResolutionTime();

#else 


#define DEBUG_STORE_DATA(FILE_NAME, DATA_PTR, N_BYTES)
#define SILK_DEBUG_STORE_CLOSE_FILES

#endif 

#ifdef  __cplusplus
}
#endif

#endif
