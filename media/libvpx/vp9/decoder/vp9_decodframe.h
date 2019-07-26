










#ifndef VP9_DECODER_VP9_DECODFRAME_H_
#define VP9_DECODER_VP9_DECODFRAME_H_

struct VP9Common;
struct VP9Decompressor;

void vp9_init_dequantizer(struct VP9Common *cm);
int vp9_decode_frame(struct VP9Decompressor *cpi, const uint8_t **p_data_end);

#endif  
