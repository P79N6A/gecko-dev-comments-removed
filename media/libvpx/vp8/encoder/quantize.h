










#ifndef __INC_QUANTIZE_H
#define __INC_QUANTIZE_H

struct VP8_COMP;
struct macroblock;
extern void vp8_set_quantizer(struct VP8_COMP *cpi, int Q);
extern void vp8cx_frame_init_quantizer(struct VP8_COMP *cpi);
extern void vp8_update_zbin_extra(struct VP8_COMP *cpi, struct macroblock *x);
extern void vp8cx_mb_init_quantizer(struct VP8_COMP *cpi, struct macroblock *x, int ok_to_skip);
extern void vp8cx_init_quantizer(struct VP8_COMP *cpi);

#endif
