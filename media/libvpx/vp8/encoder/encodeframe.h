








#ifndef ENCODEFRAME_H
#define ENCODEFRAME_H
extern void vp8_activity_masking(VP8_COMP *cpi, MACROBLOCK *x);

extern void vp8_build_block_offsets(MACROBLOCK *x);

extern void vp8_setup_block_ptrs(MACROBLOCK *x);

extern void vp8_encode_frame(VP8_COMP *cpi);

extern int vp8cx_encode_inter_macroblock(VP8_COMP *cpi, MACROBLOCK *x,
        TOKENEXTRA **t,
        int recon_yoffset, int recon_uvoffset,
        int mb_row, int mb_col);

extern int vp8cx_encode_intra_macroblock(VP8_COMP *cpi, MACROBLOCK *x,
        TOKENEXTRA **t);
#endif
