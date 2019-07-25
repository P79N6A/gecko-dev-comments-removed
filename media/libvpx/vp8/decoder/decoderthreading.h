













#ifndef _DECODER_THREADING_H
#define _DECODER_THREADING_H


extern void vp8_mtdecode_mb_rows(VP8D_COMP *pbi,
                                 MACROBLOCKD *xd);
extern void vp8_mt_loop_filter_frame(VP8D_COMP *pbi);
extern void vp8_stop_lfthread(VP8D_COMP *pbi);
extern void vp8_start_lfthread(VP8D_COMP *pbi);
extern void vp8_decoder_remove_threads(VP8D_COMP *pbi);
extern void vp8_decoder_create_threads(VP8D_COMP *pbi);
#endif
