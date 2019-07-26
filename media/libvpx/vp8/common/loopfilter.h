










#ifndef loopfilter_h
#define loopfilter_h

#include "vpx_ports/mem.h"
#include "vpx_config.h"
#include "vpx_rtcd.h"

#define MAX_LOOP_FILTER             63


#define PARTIAL_FRAME_FRACTION      8

typedef enum
{
    NORMAL_LOOPFILTER = 0,
    SIMPLE_LOOPFILTER = 1
} LOOPFILTERTYPE;

#if ARCH_ARM
#define SIMD_WIDTH 1
#else
#define SIMD_WIDTH 16
#endif




typedef struct
{
    DECLARE_ALIGNED(SIMD_WIDTH, unsigned char, mblim[MAX_LOOP_FILTER + 1][SIMD_WIDTH]);
    DECLARE_ALIGNED(SIMD_WIDTH, unsigned char, blim[MAX_LOOP_FILTER + 1][SIMD_WIDTH]);
    DECLARE_ALIGNED(SIMD_WIDTH, unsigned char, lim[MAX_LOOP_FILTER + 1][SIMD_WIDTH]);
    DECLARE_ALIGNED(SIMD_WIDTH, unsigned char, hev_thr[4][SIMD_WIDTH]);
    unsigned char lvl[4][4][4];
    unsigned char hev_thr_lut[2][MAX_LOOP_FILTER + 1];
    unsigned char mode_lf_lut[10];
} loop_filter_info_n;

typedef struct loop_filter_info
{
    const unsigned char * mblim;
    const unsigned char * blim;
    const unsigned char * lim;
    const unsigned char * hev_thr;
} loop_filter_info;


typedef void loop_filter_uvfunction
(
    unsigned char *u,   
    int p,              
    const unsigned char *blimit,
    const unsigned char *limit,
    const unsigned char *thresh,
    unsigned char *v
);


struct VP8Common;
struct macroblockd;
struct modeinfo;

void vp8_loop_filter_init(struct VP8Common *cm);

void vp8_loop_filter_frame_init(struct VP8Common *cm,
                                struct macroblockd *mbd,
                                int default_filt_lvl);

void vp8_loop_filter_frame(struct VP8Common *cm, struct macroblockd *mbd,
                           int frame_type);

void vp8_loop_filter_partial_frame(struct VP8Common *cm,
                                   struct macroblockd *mbd,
                                   int default_filt_lvl);

void vp8_loop_filter_frame_yonly(struct VP8Common *cm,
                                 struct macroblockd *mbd,
                                 int default_filt_lvl);

void vp8_loop_filter_update_sharpness(loop_filter_info_n *lfi,
                                      int sharpness_lvl);

void vp8_loop_filter_row_normal(struct VP8Common *cm,
                                struct modeinfo *mode_info_context,
                                int mb_row, int post_ystride, int post_uvstride,
                                unsigned char *y_ptr, unsigned char *u_ptr,
                                unsigned char *v_ptr);

void vp8_loop_filter_row_simple(struct VP8Common *cm,
                                struct modeinfo *mode_info_context,
                                int mb_row, int post_ystride, int post_uvstride,
                                unsigned char *y_ptr, unsigned char *u_ptr,
                                unsigned char *v_ptr);
#endif
