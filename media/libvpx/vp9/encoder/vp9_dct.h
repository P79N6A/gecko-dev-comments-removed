










#ifndef VP9_ENCODER_VP9_DCT_H_
#define VP9_ENCODER_VP9_DCT_H_

void vp9_fht4x4(TX_TYPE tx_type, const int16_t *input, int16_t *output,
                int stride);

void vp9_fht8x8(TX_TYPE tx_type, const int16_t *input, int16_t *output,
                int stride);

void vp9_fht16x16(TX_TYPE tx_type, const int16_t *input, int16_t *output,
                  int stride);

#endif  
