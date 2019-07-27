










#ifndef VP9_DECODER_VP9_DECODEFRAME_H_
#define VP9_DECODER_VP9_DECODEFRAME_H_

#ifdef __cplusplus
extern "C" {
#endif

struct VP9Common;
struct VP9Decoder;
struct vp9_read_bit_buffer;

void vp9_init_dequantizer(struct VP9Common *cm);

void vp9_decode_frame(struct VP9Decoder *pbi,
                      const uint8_t *data, const uint8_t *data_end,
                      const uint8_t **p_data_end);

int vp9_read_sync_code(struct vp9_read_bit_buffer *const rb);
void vp9_read_frame_size(struct vp9_read_bit_buffer *rb,
                         int *width, int *height);
BITSTREAM_PROFILE vp9_read_profile(struct vp9_read_bit_buffer *rb);

void vp9_dec_build_inter_predictors_sb(struct VP9Decoder *const pbi,
                                       MACROBLOCKD *xd, int mi_row, int mi_col,
                                       BLOCK_SIZE bsize);
#ifdef __cplusplus
}  
#endif

#endif
