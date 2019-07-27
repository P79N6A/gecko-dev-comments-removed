










#ifndef VP9_DECODER_VP9_DECODEFRAME_H_
#define VP9_DECODER_VP9_DECODEFRAME_H_

#ifdef __cplusplus
extern "C" {
#endif

struct VP9Common;
struct VP9Decompressor;

void vp9_init_dequantizer(struct VP9Common *cm);
int vp9_decode_frame(struct VP9Decompressor *cpi, const uint8_t **p_data_end);

#ifdef __cplusplus
}  
#endif

#endif
