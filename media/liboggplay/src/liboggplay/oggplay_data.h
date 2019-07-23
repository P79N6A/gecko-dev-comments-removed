




































#ifndef __OGGPLAY_DATA_H__
#define __OGGPLAY_DATA_H__

void
oggplay_data_initialise_list (OggPlayDecode *decode);

OggPlayErrorCode
oggplay_data_handle_theora_frame (OggPlayTheoraDecode *decode, 
                                  const yuv_buffer *buffer);

OggPlayErrorCode
oggplay_data_handle_audio_data (OggPlayDecode *decode, 
                                void *data, long samples, size_t samplesize);

OggPlayErrorCode
oggplay_data_handle_cmml_data(OggPlayDecode *decode, 
                              unsigned char *data, long size);

#ifdef HAVE_KATE
OggPlayErrorCode
oggplay_data_handle_kate_data(OggPlayKateDecode *decode,
                              const kate_event *ev);
#endif

#ifdef HAVE_TIGER
OggPlayErrorCode
oggplay_data_update_tiger(OggPlayKateDecode *decode,
                          int active, ogg_int64_t presentation_time,
                          OggPlayCallbackInfo *info);
#endif

void
oggplay_data_clean_list (OggPlayDecode *decode);

void
oggplay_data_free_list(OggPlayDataHeader *list);

void
oggplay_data_shutdown_list (OggPlayDecode *decode);
#endif
