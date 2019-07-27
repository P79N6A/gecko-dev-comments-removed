









#ifndef VP8_DECODER_DECODERTHREADING_H_
#define VP8_DECODER_DECODERTHREADING_H_

#ifdef __cplusplus
extern "C" {
#endif

#if CONFIG_MULTITHREAD
void vp8mt_decode_mb_rows(VP8D_COMP *pbi, MACROBLOCKD *xd);
void vp8_decoder_remove_threads(VP8D_COMP *pbi);
void vp8_decoder_create_threads(VP8D_COMP *pbi);
void vp8mt_alloc_temp_buffers(VP8D_COMP *pbi, int width, int prev_mb_rows);
void vp8mt_de_alloc_temp_buffers(VP8D_COMP *pbi, int mb_rows);
#endif

#ifdef __cplusplus
}  
#endif

#endif
