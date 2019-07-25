










#ifndef __INC_BLOCKD_H
#define __INC_BLOCKD_H

void vpx_log(const char *format, ...);

#include "vpx_ports/config.h"
#include "vpx_scale/yv12config.h"
#include "mv.h"
#include "treecoder.h"
#include "subpixel.h"
#include "vpx_ports/mem.h"

#define TRUE    1
#define FALSE   0


#define DCPREDSIMTHRESH 0
#define DCPREDCNTTHRESH 3

#define MB_FEATURE_TREE_PROBS   3
#define MAX_MB_SEGMENTS         4

#define MAX_REF_LF_DELTAS       4
#define MAX_MODE_LF_DELTAS      4


#define SEGMENT_DELTADATA   0
#define SEGMENT_ABSDATA     1

typedef struct
{
    int r, c;
} POS;

#define PLANE_TYPE_Y_NO_DC    0
#define PLANE_TYPE_Y2         1
#define PLANE_TYPE_UV         2
#define PLANE_TYPE_Y_WITH_DC  3


typedef char ENTROPY_CONTEXT;
typedef struct
{
    ENTROPY_CONTEXT y1[4];
    ENTROPY_CONTEXT u[2];
    ENTROPY_CONTEXT v[2];
    ENTROPY_CONTEXT y2;
} ENTROPY_CONTEXT_PLANES;

extern const unsigned char vp8_block2left[25];
extern const unsigned char vp8_block2above[25];

#define VP8_COMBINEENTROPYCONTEXTS( Dest, A, B) \
    Dest = ((A)!=0) + ((B)!=0);


typedef enum
{
    KEY_FRAME = 0,
    INTER_FRAME = 1
} FRAME_TYPE;

typedef enum
{
    DC_PRED,            
    V_PRED,             
    H_PRED,             
    TM_PRED,            
    B_PRED,             

    NEARESTMV,
    NEARMV,
    ZEROMV,
    NEWMV,
    SPLITMV,

    MB_MODE_COUNT
} MB_PREDICTION_MODE;


typedef enum
{
    MB_LVL_ALT_Q = 0,               
    MB_LVL_ALT_LF = 1,              
    MB_LVL_MAX = 2                  

} MB_LVL_FEATURES;


#define SEGMENT_ALTQ    0x01
#define SEGMENT_ALT_LF  0x02

#define VP8_YMODES  (B_PRED + 1)
#define VP8_UV_MODES (TM_PRED + 1)

#define VP8_MVREFS (1 + SPLITMV - NEARESTMV)

typedef enum
{
    B_DC_PRED,          
    B_TM_PRED,

    B_VE_PRED,           
    B_HE_PRED,           

    B_LD_PRED,
    B_RD_PRED,

    B_VR_PRED,
    B_VL_PRED,
    B_HD_PRED,
    B_HU_PRED,

    LEFT4X4,
    ABOVE4X4,
    ZERO4X4,
    NEW4X4,

    B_MODE_COUNT
} B_PREDICTION_MODE;

#define VP8_BINTRAMODES (B_HU_PRED + 1)  /* 10 */
#define VP8_SUBMVREFS (1 + NEW4X4 - LEFT4X4)





union b_mode_info
{
    B_PREDICTION_MODE as_mode;
    int_mv mv;
};

typedef enum
{
    INTRA_FRAME = 0,
    LAST_FRAME = 1,
    GOLDEN_FRAME = 2,
    ALTREF_FRAME = 3,
    MAX_REF_FRAMES = 4
} MV_REFERENCE_FRAME;

typedef struct
{
    MB_PREDICTION_MODE mode, uv_mode;
    MV_REFERENCE_FRAME ref_frame;
    int_mv mv;

    unsigned char partitioning;
    unsigned char mb_skip_coeff;                                
    unsigned char need_to_clamp_mvs;
    unsigned char segment_id;                  
} MB_MODE_INFO;

typedef struct
{
    MB_MODE_INFO mbmi;
    union b_mode_info bmi[16];
} MODE_INFO;

typedef struct
{
    short *qcoeff;
    short *dqcoeff;
    unsigned char  *predictor;
    short *diff;
    short *dequant;

    
    unsigned char **base_pre;
    int pre;
    int pre_stride;

    unsigned char **base_dst;
    int dst;
    int dst_stride;

    int eob;

    union b_mode_info bmi;
} BLOCKD;

typedef struct MacroBlockD
{
    DECLARE_ALIGNED(16, short, diff[400]);      
    DECLARE_ALIGNED(16, unsigned char,  predictor[384]);
    DECLARE_ALIGNED(16, short, qcoeff[400]);
    DECLARE_ALIGNED(16, short, dqcoeff[400]);
    DECLARE_ALIGNED(16, char,  eobs[25]);

    
    BLOCKD block[25];

    YV12_BUFFER_CONFIG pre; 
    YV12_BUFFER_CONFIG dst;

    MODE_INFO *mode_info_context;
    int mode_info_stride;

    FRAME_TYPE frame_type;

    int up_available;
    int left_available;

    
    ENTROPY_CONTEXT_PLANES *above_context;
    ENTROPY_CONTEXT_PLANES *left_context;

    
    unsigned char segmentation_enabled;

    
    unsigned char update_mb_segmentation_map;

    
    unsigned char update_mb_segmentation_data;

    
    unsigned char mb_segement_abs_delta;

    
    
    vp8_prob mb_segment_tree_probs[MB_FEATURE_TREE_PROBS];         

    signed char segment_feature_data[MB_LVL_MAX][MAX_MB_SEGMENTS];            

    
    unsigned char mode_ref_lf_delta_enabled;
    unsigned char mode_ref_lf_delta_update;

    
    signed char last_ref_lf_deltas[MAX_REF_LF_DELTAS];                
    signed char ref_lf_deltas[MAX_REF_LF_DELTAS];                     
    signed char last_mode_lf_deltas[MAX_MODE_LF_DELTAS];                      
    signed char mode_lf_deltas[MAX_MODE_LF_DELTAS];                           

    
    int mb_to_left_edge;
    int mb_to_right_edge;
    int mb_to_top_edge;
    int mb_to_bottom_edge;

    int ref_frame_cost[MAX_REF_FRAMES];


    unsigned int frames_since_golden;
    unsigned int frames_till_alt_ref_frame;
    vp8_subpix_fn_t  subpixel_predict;
    vp8_subpix_fn_t  subpixel_predict8x4;
    vp8_subpix_fn_t  subpixel_predict8x8;
    vp8_subpix_fn_t  subpixel_predict16x16;

    void *current_bc;

    int corrupted;

#if ARCH_X86 || ARCH_X86_64
    



    DECLARE_ALIGNED(32, unsigned char, y_buf[22*32]);
#endif

#if CONFIG_RUNTIME_CPU_DETECT
    struct VP8_COMMON_RTCD  *rtcd;
#endif
} MACROBLOCKD;


extern void vp8_build_block_doffsets(MACROBLOCKD *x);
extern void vp8_setup_block_dptrs(MACROBLOCKD *x);

static void update_blockd_bmi(MACROBLOCKD *xd)
{
    int i;
    int is_4x4;
    is_4x4 = (xd->mode_info_context->mbmi.mode == SPLITMV) ||
              (xd->mode_info_context->mbmi.mode == B_PRED);

    if (is_4x4)
    {
        for (i = 0; i < 16; i++)
        {
            xd->block[i].bmi = xd->mode_info_context->bmi[i];
        }
    }
}

#endif  
