










#ifndef VP9_ENCODER_VP9_ENCODEFRAME_H_
#define VP9_ENCODER_VP9_ENCODEFRAME_H_

#ifdef __cplusplus
extern "C" {
#endif

struct macroblock;
struct yv12_buffer_config;
struct VP9_COMP;
struct ThreadData;


#define VAR_HIST_MAX_BG_VAR 1000
#define VAR_HIST_FACTOR 10
#define VAR_HIST_BINS (VAR_HIST_MAX_BG_VAR / VAR_HIST_FACTOR + 1)
#define VAR_HIST_LARGE_CUT_OFF 75
#define VAR_HIST_SMALL_CUT_OFF 45

void vp9_setup_src_planes(struct macroblock *x,
                          const struct yv12_buffer_config *src,
                          int mi_row, int mi_col);

void vp9_encode_frame(struct VP9_COMP *cpi);

void vp9_init_tile_data(struct VP9_COMP *cpi);
void vp9_encode_tile(struct VP9_COMP *cpi, struct ThreadData *td,
                     int tile_row, int tile_col);

void vp9_set_vbp_thresholds(struct VP9_COMP *cpi, int q);

#ifdef __cplusplus
}  
#endif

#endif
