










#ifndef VP9_ENCODER_VP9_ENCODEFRAME_H_
#define VP9_ENCODER_VP9_ENCODEFRAME_H_

#ifdef __cplusplus
extern "C" {
#endif

struct macroblock;
struct yv12_buffer_config;

void vp9_setup_src_planes(struct macroblock *x,
                          const struct yv12_buffer_config *src,
                          int mi_row, int mi_col);

#ifdef __cplusplus
}  
#endif

#endif
