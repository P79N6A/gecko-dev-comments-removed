










#ifndef __INC_RDOPT_H
#define __INC_RDOPT_H

#define RDCOST(RM,DM,R,D) ( ((128+(R)*(RM)) >> 8) + (DM)*(D) )

extern void vp8_initialize_rd_consts(VP8_COMP *cpi, int Qvalue);
extern void vp8_rd_pick_inter_mode(VP8_COMP *cpi, MACROBLOCK *x, int recon_yoffset, int recon_uvoffset, int *returnrate, int *returndistortion, int *returnintra);
extern void vp8_rd_pick_intra_mode(VP8_COMP *cpi, MACROBLOCK *x, int *rate);

extern void vp8_mv_pred
(
    VP8_COMP *cpi,
    MACROBLOCKD *xd,
    const MODE_INFO *here,
    int_mv *mvp,
    int refframe,
    int *ref_frame_sign_bias,
    int *sr,
    int near_sadidx[]
);
void vp8_cal_sad(VP8_COMP *cpi, MACROBLOCKD *xd, MACROBLOCK *x, int recon_yoffset, int near_sadidx[]);

#endif
