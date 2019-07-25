










#ifndef __INC_VP8C_INT_H
#define __INC_VP8C_INT_H

#include "vpx_config.h"
#include "vpx/internal/vpx_codec_internal.h"
#include "loopfilter.h"
#include "entropymv.h"
#include "entropy.h"
#include "idct.h"
#include "recon.h"
#if CONFIG_POSTPROC
#include "postproc.h"
#endif


#include "header.h"




void vp8_initialize_common(void);

#define MINQ 0
#define MAXQ 127
#define QINDEX_RANGE (MAXQ + 1)

#define NUM_YV12_BUFFERS 4

#define MAX_PARTITIONS 9

typedef struct frame_contexts
{
    vp8_prob bmode_prob [VP8_BINTRAMODES-1];
    vp8_prob ymode_prob [VP8_YMODES-1];   
    vp8_prob uv_mode_prob [VP8_UV_MODES-1];
    vp8_prob sub_mv_ref_prob [VP8_SUBMVREFS-1];
    vp8_prob coef_probs [BLOCK_TYPES] [COEF_BANDS] [PREV_COEF_CONTEXTS] [ENTROPY_NODES];
    MV_CONTEXT mvc[2];
    MV_CONTEXT pre_mvc[2];  
} FRAME_CONTEXT;

typedef enum
{
    ONE_PARTITION  = 0,
    TWO_PARTITION  = 1,
    FOUR_PARTITION = 2,
    EIGHT_PARTITION = 3
} TOKEN_PARTITION;

typedef enum
{
    RECON_CLAMP_REQUIRED        = 0,
    RECON_CLAMP_NOTREQUIRED     = 1
} CLAMP_TYPE;

typedef enum
{
    SIXTAP   = 0,
    BILINEAR = 1
} INTERPOLATIONFILTERTYPE;

typedef struct VP8_COMMON_RTCD
{
#if CONFIG_RUNTIME_CPU_DETECT
    vp8_idct_rtcd_vtable_t        idct;
    vp8_recon_rtcd_vtable_t       recon;
    vp8_subpix_rtcd_vtable_t      subpix;
    vp8_loopfilter_rtcd_vtable_t  loopfilter;
#if CONFIG_POSTPROC
    vp8_postproc_rtcd_vtable_t    postproc;
#endif
    int                           flags;
#else
    int unused;
#endif
} VP8_COMMON_RTCD;

typedef struct VP8Common

{
    struct vpx_internal_error_info  error;

    DECLARE_ALIGNED(16, short, Y1dequant[QINDEX_RANGE][16]);
    DECLARE_ALIGNED(16, short, Y2dequant[QINDEX_RANGE][16]);
    DECLARE_ALIGNED(16, short, UVdequant[QINDEX_RANGE][16]);

    int Width;
    int Height;
    int horiz_scale;
    int vert_scale;

    YUV_TYPE clr_type;
    CLAMP_TYPE  clamp_type;

    YV12_BUFFER_CONFIG *frame_to_show;

    YV12_BUFFER_CONFIG yv12_fb[NUM_YV12_BUFFERS];
    int fb_idx_ref_cnt[NUM_YV12_BUFFERS];
    int new_fb_idx, lst_fb_idx, gld_fb_idx, alt_fb_idx;

    YV12_BUFFER_CONFIG post_proc_buffer;
    YV12_BUFFER_CONFIG temp_scale_frame;


    FRAME_TYPE last_frame_type;  
    FRAME_TYPE frame_type;

    int show_frame;

    int frame_flags;
    int MBs;
    int mb_rows;
    int mb_cols;
    int mode_info_stride;

    
    int mb_no_coeff_skip;
    int no_lpf;
    int use_bilinear_mc_filter;
    int full_pixel;

    int base_qindex;
    int last_kf_gf_q;  

    int y1dc_delta_q;
    int y2dc_delta_q;
    int y2ac_delta_q;
    int uvdc_delta_q;
    int uvac_delta_q;

    unsigned int frames_since_golden;
    unsigned int frames_till_alt_ref_frame;

    


    MODE_INFO *mip; 
    MODE_INFO *mi;  
    MODE_INFO *prev_mip; 
    MODE_INFO *prev_mi;  


    INTERPOLATIONFILTERTYPE mcomp_filter_type;
    LOOPFILTERTYPE filter_type;

    loop_filter_info_n lf_info;

    int filter_level;
    int last_sharpness_level;
    int sharpness_level;

    int refresh_last_frame;       
    int refresh_golden_frame;     
    int refresh_alt_ref_frame;     

    int copy_buffer_to_gf;         
    int copy_buffer_to_arf;        

    int refresh_entropy_probs;    

    int ref_frame_sign_bias[MAX_REF_FRAMES];    

    
    ENTROPY_CONTEXT_PLANES *above_context;   
    ENTROPY_CONTEXT_PLANES left_context;  


    

    vp8_prob kf_bmode_prob [VP8_BINTRAMODES] [VP8_BINTRAMODES] [VP8_BINTRAMODES-1];
    vp8_prob kf_ymode_prob [VP8_YMODES-1];  
    vp8_prob kf_uv_mode_prob [VP8_UV_MODES-1];


    FRAME_CONTEXT lfc; 
    FRAME_CONTEXT fc;  

    unsigned int current_video_frame;

    int near_boffset[3];
    int version;

    TOKEN_PARTITION multi_token_partition;

#ifdef PACKET_TESTING
    VP8_HEADER oh;
#endif
    double bitrate;
    double framerate;

#if CONFIG_RUNTIME_CPU_DETECT
    VP8_COMMON_RTCD rtcd;
#endif
#if CONFIG_MULTITHREAD
    int processor_core_count;
#endif
#if CONFIG_POSTPROC
    struct postproc_state  postproc_state;
#endif
} VP8_COMMON;

#endif
