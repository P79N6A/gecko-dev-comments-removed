




































#ifndef __OGGPLAY_CALLBACK_H__
#define __OGGPLAY_CALLBACK_H__

int
oggplay_callback_predetected (OGGZ *oggz, ogg_packet *op, long serialno,
                void *user_data);

void
oggplay_process_leftover_packet(OggPlay *me);












OggPlayDecode *
oggplay_initialise_decoder(OggPlay *me, int content_type, int serialno);

int
oggplay_callback_info_prepare(OggPlay *me, OggPlayCallbackInfo ***info);

void
oggplay_callback_info_destroy(OggPlay *me, OggPlayCallbackInfo **info);

void
oggplay_callback_shutdown(OggPlayDecode *decoder);
#endif
