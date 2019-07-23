






































#ifndef __OGGPLAY_H__
#define __OGGPLAY_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <oggplay/oggplay_enums.h>
#include <oggplay/oggplay_reader.h>

typedef struct _OggPlay OggPlay;
typedef struct _OggPlayCallbackInfo OggPlayCallbackInfo;
typedef int (OggPlayDataCallback)(OggPlay *player, int num_records,
                OggPlayCallbackInfo **records, void *user);

#include <oggplay/oggplay_query.h>
#include <oggplay/oggplay_callback_info.h>
#include <oggplay/oggplay_tools.h>
#include <oggplay/oggplay_seek.h>











OggPlay *
oggplay_open_with_reader(OggPlayReader *reader);








OggPlay *
oggplay_new_with_reader(OggPlayReader *reader);

OggPlayErrorCode
oggplay_initialise(OggPlay *me, int block);

OggPlayErrorCode
oggplay_set_source(OggPlay *OS, char *source);

OggPlayErrorCode
oggplay_set_data_callback(OggPlay *me, OggPlayDataCallback callback, 
                            void *user);

OggPlayErrorCode
oggplay_set_callback_num_frames(OggPlay *me, int stream, int frames);

OggPlayErrorCode
oggplay_set_callback_period(OggPlay *me, int stream, int milliseconds);

OggPlayErrorCode
oggplay_set_offset(OggPlay *me, int track, ogg_int64_t offset);

OggPlayErrorCode
oggplay_get_video_y_size(OggPlay *me, int track, int *y_width, int *y_height);

OggPlayErrorCode
oggplay_get_video_uv_size(OggPlay *me, int track, int *uv_width, int *uv_height);

OggPlayErrorCode
oggplay_get_audio_channels(OggPlay *me, int track, int *channels);

OggPlayErrorCode
oggplay_get_audio_samplerate(OggPlay *me, int track, int *samplerate); 

OggPlayErrorCode
oggplay_get_video_fps(OggPlay *me, int track, int* fps_denom, int* fps_num);

OggPlayErrorCode
oggplay_get_video_aspect_ratio(OggPlay *me, int track, int* aspect_denom, int* aspect_num);

OggPlayErrorCode
oggplay_convert_video_to_rgb(OggPlay *me, int track, int convert);

OggPlayErrorCode
oggplay_get_kate_category(OggPlay *me, int track, const char** category);

OggPlayErrorCode
oggplay_get_kate_language(OggPlay *me, int track, const char** language);

OggPlayErrorCode
oggplay_set_kate_tiger_rendering(OggPlay *me, int track, int use_tiger);

OggPlayErrorCode
oggplay_overlay_kate_track_on_video(OggPlay *me, int kate_track, int video_track);

OggPlayErrorCode
oggplay_start_decoding(OggPlay *me);

OggPlayErrorCode
oggplay_step_decoding(OggPlay *me);

OggPlayErrorCode
oggplay_use_buffer(OggPlay *player, int size);

OggPlayCallbackInfo **
oggplay_buffer_retrieve_next(OggPlay *player);

OggPlayErrorCode
oggplay_buffer_release(OggPlay *player, OggPlayCallbackInfo **track_info);

void
oggplay_prepare_for_close(OggPlay *me);








OggPlayErrorCode
oggplay_close(OggPlay *player);

int
oggplay_get_available(OggPlay *player);

ogg_int64_t
oggplay_get_duration(OggPlay * player);

int
oggplay_media_finished_retrieving(OggPlay * player);

#ifdef __cplusplus
}
#endif

#endif
