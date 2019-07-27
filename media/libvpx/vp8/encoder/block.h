










#ifndef VP8_ENCODER_BLOCK_H_
#define VP8_ENCODER_BLOCK_H_

#include "vp8/common/onyx.h"
#include "vp8/common/blockd.h"
#include "vp8/common/entropymv.h"
#include "vp8/common/entropy.h"
#include "vpx_ports/mem.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_MODES 20
#define MAX_ERROR_BINS 1024


typedef struct
{
    MV mv;
    int offset;
} search_site;

typedef struct block
{
    
    short *src_diff;
    short *coeff;

    
    short *quant;
    short *quant_fast;
    short *quant_shift;
    short *zbin;
    short *zrun_zbin_boost;
    short *round;

    
    short zbin_extra;

    unsigned char **base_src;
    int src;
    int src_stride;
} BLOCK;

typedef struct
{
    int count;
    struct
    {
        B_PREDICTION_MODE mode;
        int_mv mv;
    } bmi[16];
} PARTITION_INFO;

typedef struct macroblock
{
    DECLARE_ALIGNED(16, short, src_diff[400]); 
    DECLARE_ALIGNED(16, short, coeff[400]); 
    DECLARE_ALIGNED(16, unsigned char, thismb[256]);

    unsigned char *thismb_ptr;
    
    BLOCK block[25];

    YV12_BUFFER_CONFIG src;

    MACROBLOCKD e_mbd;
    PARTITION_INFO *partition_info; 
    PARTITION_INFO *pi;   
    PARTITION_INFO *pip;  

    int ref_frame_cost[MAX_REF_FRAMES];

    search_site *ss;
    int ss_count;
    int searches_per_step;

    int errorperbit;
    int sadperbit16;
    int sadperbit4;
    int rddiv;
    int rdmult;
    unsigned int * mb_activity_ptr;
    int * mb_norm_activity_ptr;
    signed int act_zbin_adj;
    signed int last_act_zbin_adj;

    int *mvcost[2];
    
    DECLARE_ALIGNED(16, int*, mvsadcost[2]);
    int (*mbmode_cost)[MB_MODE_COUNT];
    int (*intra_uv_mode_cost)[MB_MODE_COUNT];
    int (*bmode_costs)[10][10];
    int *inter_bmode_costs;
    int (*token_costs)[COEF_BANDS][PREV_COEF_CONTEXTS]
    [MAX_ENTROPY_TOKENS];

    


    int mv_col_min;
    int mv_col_max;
    int mv_row_min;
    int mv_row_max;

    int skip;

    unsigned int encode_breakout;

    signed char *gf_active_ptr;

    unsigned char *active_ptr;
    MV_CONTEXT *mvc;

    int optimize;
    int q_index;

#if CONFIG_TEMPORAL_DENOISING
    MB_PREDICTION_MODE best_sse_inter_mode;
    int_mv best_sse_mv;
    MV_REFERENCE_FRAME best_reference_frame;
    MV_REFERENCE_FRAME best_zeromv_reference_frame;
    unsigned char need_to_clamp_best_mvs;
#endif

    int skip_true_count;
    unsigned int coef_counts [BLOCK_TYPES] [COEF_BANDS] [PREV_COEF_CONTEXTS] [MAX_ENTROPY_TOKENS];
    unsigned int MVcount [2] [MVvals];  
    int ymode_count [VP8_YMODES];        
    int uv_mode_count[VP8_UV_MODES];     
    int64_t prediction_error;
    int64_t intra_error;
    int count_mb_ref_frame_usage[MAX_REF_FRAMES];

    int rd_thresh_mult[MAX_MODES];
    int rd_threshes[MAX_MODES];
    unsigned int mbs_tested_so_far;
    unsigned int mode_test_hit_counts[MAX_MODES];
    int zbin_mode_boost_enabled;
    int zbin_mode_boost;
    int last_zbin_mode_boost;

    int last_zbin_over_quant;
    int zbin_over_quant;
    int error_bins[MAX_ERROR_BINS];

    void (*short_fdct4x4)(short *input, short *output, int pitch);
    void (*short_fdct8x4)(short *input, short *output, int pitch);
    void (*short_walsh4x4)(short *input, short *output, int pitch);
    void (*quantize_b)(BLOCK *b, BLOCKD *d);
    void (*quantize_b_pair)(BLOCK *b1, BLOCK *b2, BLOCKD *d0, BLOCKD *d1);

} MACROBLOCK;


#ifdef __cplusplus
}  
#endif

#endif
