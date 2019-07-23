





































#ifndef __OGGPLAY_CALLBACK_INFO__
#define __OGGPLAY_CALLBACK_INFO__

typedef struct {
  unsigned char   * y;
  unsigned char   * u;
  unsigned char   * v;
} OggPlayVideoData;

typedef struct {
  unsigned char   * rgba; 
  unsigned char   * rgb; 
  size_t          width; 
  size_t          height; 
  size_t          stride; 
} OggPlayOverlayData;

typedef void * OggPlayAudioData;

typedef char OggPlayTextData;

struct _OggPlayDataHeader;
typedef struct _OggPlayDataHeader OggPlayDataHeader;

OggPlayDataType
oggplay_callback_info_get_type(OggPlayCallbackInfo *info);
int
oggplay_callback_info_get_available(OggPlayCallbackInfo *info);
int
oggplay_callback_info_get_required(OggPlayCallbackInfo *info);
OggPlayDataHeader **
oggplay_callback_info_get_headers(OggPlayCallbackInfo *info);

ogg_int64_t
oggplay_callback_info_get_record_size(OggPlayDataHeader *header);

OggPlayVideoData *
oggplay_callback_info_get_video_data(OggPlayDataHeader *header);

OggPlayOverlayData *
oggplay_callback_info_get_overlay_data(OggPlayDataHeader *header);

OggPlayAudioData *
oggplay_callback_info_get_audio_data(OggPlayDataHeader *header);

OggPlayTextData *
oggplay_callback_info_get_text_data(OggPlayDataHeader *header);

OggPlayStreamInfo
oggplay_callback_info_get_stream_info(OggPlayCallbackInfo *info);

void
oggplay_callback_info_lock_item(OggPlayDataHeader *header);

void
oggplay_callback_info_unlock_item(OggPlayDataHeader *header);

long
oggplay_callback_info_get_presentation_time(OggPlayDataHeader *header);

#endif
