










#ifndef __INC_VP8D_INT_H
#define __INC_VP8D_INT_H
#include "vpx_config.h"
#include "vp8/common/onyxd.h"
#include "treereader.h"
#include "vp8/common/onyxc_int.h"
#include "vp8/common/threading.h"


#if CONFIG_ERROR_CONCEALMENT
#include "ec_types.h"
#endif

typedef struct
{
    int ithread;
    void *ptr1;
    void *ptr2;
} DECODETHREAD_DATA;

typedef struct
{
    MACROBLOCKD  mbd;
    int mb_row;
    int current_mb_col;
    short *coef_ptr;
} MB_ROW_DEC;

typedef struct
{
    int64_t time_stamp;
    int size;
} DATARATE;


typedef struct VP8D_COMP
{
    DECLARE_ALIGNED(16, MACROBLOCKD, mb);

    DECLARE_ALIGNED(16, VP8_COMMON, common);

    vp8_reader bc, bc2;

    VP8D_CONFIG oxcf;


    const unsigned char *fragments[MAX_PARTITIONS];
    unsigned int   fragment_sizes[MAX_PARTITIONS];
    unsigned int   num_fragments;

#if CONFIG_MULTITHREAD
    

    volatile int b_multithreaded_rd;
    int max_threads;
    int current_mb_col_main;
    int decoding_thread_count;
    int allocated_decoding_thread_count;

    int mt_baseline_filter_level[MAX_MB_SEGMENTS];
    int sync_range;
    int *mt_current_mb_col;                  

    unsigned char **mt_yabove_row;           
    unsigned char **mt_uabove_row;
    unsigned char **mt_vabove_row;
    unsigned char **mt_yleft_col;            
    unsigned char **mt_uleft_col;            
    unsigned char **mt_vleft_col;            

    MB_ROW_DEC           *mb_row_di;
    DECODETHREAD_DATA    *de_thread_data;

    pthread_t           *h_decoding_thread;
    sem_t               *h_event_start_decoding;
    sem_t                h_event_end_decoding;
    
#endif

    vp8_reader *mbc;
    int64_t last_time_stamp;
    int   ready_for_new_data;

    DATARATE dr[16];

    vp8_prob prob_intra;
    vp8_prob prob_last;
    vp8_prob prob_gf;
    vp8_prob prob_skip_false;

#if CONFIG_ERROR_CONCEALMENT
    MB_OVERLAP *overlaps;
    
    unsigned int mvs_corrupt_from_mb;
#endif
    int ec_enabled;
    int ec_active;
    int input_fragments;
    int decoded_key_frame;
    int independent_partitions;
    int frame_corrupt_residual;

} VP8D_COMP;

int vp8_decode_frame(VP8D_COMP *cpi);
void vp8_dmachine_specific_config(VP8D_COMP *pbi);


#if CONFIG_DEBUG
#define CHECK_MEM_ERROR(lval,expr) do {\
        lval = (expr); \
        if(!lval) \
            vpx_internal_error(&pbi->common.error, VPX_CODEC_MEM_ERROR,\
                               "Failed to allocate "#lval" at %s:%d", \
                               __FILE__,__LINE__);\
    } while(0)
#else
#define CHECK_MEM_ERROR(lval,expr) do {\
        lval = (expr); \
        if(!lval) \
            vpx_internal_error(&pbi->common.error, VPX_CODEC_MEM_ERROR,\
                               "Failed to allocate "#lval);\
    } while(0)
#endif

#endif
