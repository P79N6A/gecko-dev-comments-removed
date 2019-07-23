






































#include "oggplay_private.h"
#include "oggplay_buffer.h"

#include <string.h>
#include <stdlib.h>

#define OGGZ_READ_CHUNK_SIZE 8192

OggPlay *
oggplay_new_with_reader(OggPlayReader *reader) {

  OggPlay * me = NULL;

  
  if (reader == NULL)
    return NULL;

  me = (OggPlay *)oggplay_malloc (sizeof(OggPlay));
  if (me == NULL)
	  return NULL;

  me->reader = reader;
  me->decode_data = NULL;
  me->callback_info = NULL;
  me->num_tracks = 0;
  me->all_tracks_initialised = 0;
  me->callback_period = 0;
  me->callback = NULL;
  me->target = 0L;
  me->active_tracks = 0;
  me->buffer = NULL;
  me->shutdown = 0;
  me->trash = NULL;
  me->oggz = NULL;
  me->pt_update_valid = 1;
  me->duration = -1;

  return me;

}

OggPlayErrorCode
oggplay_initialise(OggPlay *me, int block) {

  OggPlayErrorCode  return_val;
  int               i;

  return_val = me->reader->initialise(me->reader, block);

  if (return_val != E_OGGPLAY_OK) {
    return return_val;
  }

  



  me->presentation_time = 0;

  




  me->oggz = oggz_new(OGGZ_READ | OGGZ_AUTO);
  if (me->oggz == NULL)
    return E_OGGPLAY_OGGZ_UNHAPPY;

  if (oggz_io_set_read(me->oggz, me->reader->io_read, me->reader) != 0)
    return E_OGGPLAY_OGGZ_UNHAPPY;

  if (oggz_io_set_seek(me->oggz, me->reader->io_seek, me->reader) != 0)
    return E_OGGPLAY_OGGZ_UNHAPPY;

  if (oggz_io_set_tell(me->oggz, me->reader->io_tell, me->reader) != 0)
    return E_OGGPLAY_OGGZ_UNHAPPY;

  if (oggz_set_read_callback(me->oggz, -1, oggplay_callback_predetected, me))
    return E_OGGPLAY_OGGZ_UNHAPPY;

  while (1) {

    if (oggz_read(me->oggz, OGGZ_READ_CHUNK_SIZE) <= 0) {
      return E_OGGPLAY_BAD_INPUT;
    }

    if (me->all_tracks_initialised) {
      break;
    }
  }

  


  for (i = 0; i < me->num_tracks; i++) {
    me->decode_data[i]->active = 0;
  }

  


  if (me->buffer != NULL) {
    oggplay_buffer_prepare(me);
  }

  return E_OGGPLAY_OK;

}

OggPlay *
oggplay_open_with_reader(OggPlayReader *reader) {

  OggPlay *me = NULL;
  int r = E_OGGPLAY_TIMEOUT;

  if ( (me = oggplay_new_with_reader(reader)) == NULL)
    return NULL;

  while (r == E_OGGPLAY_TIMEOUT) {
    r = oggplay_initialise(me, 0);
  }

  if (r != E_OGGPLAY_OK) {
    
    oggplay_close(me);

    return NULL;
  }

  return me;
}





OggPlayErrorCode
oggplay_set_data_callback(OggPlay *me, OggPlayDataCallback callback,
                          void *user) {

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (me->buffer != NULL) {
    return E_OGGPLAY_BUFFER_MODE;
  }

  oggplay_set_data_callback_force(me, callback, user);
  return E_OGGPLAY_OK;

}





void
oggplay_set_data_callback_force(OggPlay *me, OggPlayDataCallback callback,
                void *user) {

  me->callback = callback;
  me->callback_user_ptr = user;

}


OggPlayErrorCode
oggplay_set_callback_num_frames(OggPlay *me, int track, int frames) {

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  me->callback_period = me->decode_data[track]->granuleperiod * frames;
  me->target = me->presentation_time + me->callback_period - 1;

  return E_OGGPLAY_OK;
}

OggPlayErrorCode
oggplay_set_callback_period(OggPlay *me, int track, int milliseconds) {

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  me->callback_period = OGGPLAY_TIME_INT_TO_FP(((ogg_int64_t)milliseconds))/1000;
  me->target = me->presentation_time + me->callback_period - 1;

  return E_OGGPLAY_OK;
}

OggPlayErrorCode
oggplay_set_offset(OggPlay *me, int track, ogg_int64_t offset) {

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  me->decode_data[track]->offset = OGGPLAY_TIME_INT_TO_FP(offset) / 1000;

  return E_OGGPLAY_OK;

}

OggPlayErrorCode
oggplay_get_video_fps(OggPlay *me, int track, int* fps_denom, int* fps_num) {
  OggPlayTheoraDecode *decode;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  if (me->decode_data[track]->decoded_type != OGGPLAY_YUV_VIDEO) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  decode = (OggPlayTheoraDecode *)(me->decode_data[track]);

  if ((decode->video_info.fps_denominator == 0)
    || (decode->video_info.fps_numerator == 0)) {
    return E_OGGPLAY_UNINITIALISED;
  }

  (*fps_denom) = decode->video_info.fps_denominator;
  (*fps_num) = decode->video_info.fps_numerator;

  return E_OGGPLAY_OK;
}

OggPlayErrorCode
oggplay_convert_video_to_rgb(OggPlay *me, int track, int convert) {
  OggPlayTheoraDecode *decode;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  if (me->decode_data[track]->content_type != OGGZ_CONTENT_THEORA) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  decode = (OggPlayTheoraDecode *)(me->decode_data[track]);

  if (decode->convert_to_rgb != convert) {
    decode->convert_to_rgb = convert;
    me->decode_data[track]->decoded_type = convert ? OGGPLAY_RGBA_VIDEO : OGGPLAY_YUV_VIDEO;

    
    oggplay_data_free_list(me->decode_data[track]->data_list);
    me->decode_data[track]->data_list = NULL;
  }

  return E_OGGPLAY_OK;
}

OggPlayErrorCode
oggplay_get_video_y_size(OggPlay *me, int track, int *y_width, int *y_height) {

  OggPlayTheoraDecode *decode;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  if (me->decode_data[track]->decoded_type != OGGPLAY_YUV_VIDEO) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  decode = (OggPlayTheoraDecode *)(me->decode_data[track]);

  if (decode->y_width == 0) {
    return E_OGGPLAY_UNINITIALISED;
  }

  (*y_width) = decode->y_width;
  (*y_height) = decode->y_height;

  return E_OGGPLAY_OK;

}

OggPlayErrorCode
oggplay_get_video_uv_size(OggPlay *me, int track, int *uv_width, int *uv_height)
{

  OggPlayTheoraDecode *decode;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  if (me->decode_data[track]->decoded_type != OGGPLAY_YUV_VIDEO) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  decode = (OggPlayTheoraDecode *)(me->decode_data[track]);

  if (decode->y_width == 0) {
    return E_OGGPLAY_UNINITIALISED;
  }

  (*uv_width) = decode->uv_width;
  (*uv_height) = decode->uv_height;

  return E_OGGPLAY_OK;

}

OggPlayErrorCode
oggplay_get_audio_channels(OggPlay *me, int track, int* channels) {

  OggPlayAudioDecode *decode;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  if (me->decode_data[track]->decoded_type != OGGPLAY_FLOATS_AUDIO) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  decode = (OggPlayAudioDecode *)(me->decode_data[track]);

  if (decode->sound_info.channels == 0) {
    return E_OGGPLAY_UNINITIALISED;
  }
  (*channels) = decode->sound_info.channels;
  return E_OGGPLAY_OK;

}

OggPlayErrorCode
oggplay_get_audio_samplerate(OggPlay *me, int track, int* rate) {

  OggPlayAudioDecode * decode;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  if (me->decode_data[track]->decoded_type != OGGPLAY_FLOATS_AUDIO) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  decode = (OggPlayAudioDecode *)(me->decode_data[track]);

  if (decode->sound_info.channels == 0) {
    return E_OGGPLAY_UNINITIALISED;
  }
  (*rate) = decode->sound_info.samplerate;
  return E_OGGPLAY_OK;

}

OggPlayErrorCode
oggplay_get_kate_category(OggPlay *me, int track, const char** category) {

  OggPlayKateDecode * decode;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  if (me->decode_data[track]->content_type != OGGZ_CONTENT_KATE) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  decode = (OggPlayKateDecode *)(me->decode_data[track]);

#ifdef HAVE_KATE
  if (decode->init) {
    (*category) = decode->k.ki->category;
    return E_OGGPLAY_OK;
  }
  else return E_OGGPLAY_UNINITIALISED;
#else
  return E_OGGPLAY_NO_KATE_SUPPORT;
#endif
}

OggPlayErrorCode
oggplay_get_kate_language(OggPlay *me, int track, const char** language) {

  OggPlayKateDecode * decode;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  if (me->decode_data[track]->content_type != OGGZ_CONTENT_KATE) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  decode = (OggPlayKateDecode *)(me->decode_data[track]);

#ifdef HAVE_KATE
  if (decode->init) {
    (*language) = decode->k.ki->language;
    return E_OGGPLAY_OK;
  }
  else return E_OGGPLAY_UNINITIALISED;
#else
  return E_OGGPLAY_NO_KATE_SUPPORT;
#endif
}

OggPlayErrorCode
oggplay_set_kate_tiger_rendering(OggPlay *me, int track, int use_tiger) {

  OggPlayKateDecode * decode;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (track < 0 || track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  if (me->decode_data[track]->content_type != OGGZ_CONTENT_KATE) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  decode = (OggPlayKateDecode *)(me->decode_data[track]);

#ifdef HAVE_KATE
#ifdef HAVE_TIGER
  if (decode->init && decode->tr) {
    decode->use_tiger = use_tiger;
    decode->decoder.decoded_type = use_tiger ? OGGPLAY_RGBA_VIDEO : OGGPLAY_KATE;
    return E_OGGPLAY_OK;
  }
  else return E_OGGPLAY_UNINITIALISED;
#else
  return E_OGGPLAY_NO_TIGER_SUPPORT;
#endif
#else
  return E_OGGPLAY_NO_KATE_SUPPORT;
#endif
}

OggPlayErrorCode
oggplay_overlay_kate_track_on_video(OggPlay *me, int kate_track, int video_track) {

  OggPlayKateDecode * decode;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (kate_track < 0 || kate_track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }
  if (video_track < 0 || video_track >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  if (me->decode_data[kate_track]->content_type != OGGZ_CONTENT_KATE) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  if (me->decode_data[kate_track]->decoded_type != OGGPLAY_RGBA_VIDEO) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  if (me->decode_data[video_track]->content_type != OGGZ_CONTENT_THEORA) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  if (me->decode_data[video_track]->decoded_type != OGGPLAY_RGBA_VIDEO) {
    return E_OGGPLAY_WRONG_TRACK_TYPE;
  }

  decode = (OggPlayKateDecode *)(me->decode_data[kate_track]);

#ifdef HAVE_KATE
#ifdef HAVE_TIGER
  decode->overlay_dest = video_track;
  return E_OGGPLAY_OK;
#else
  return E_OGGPLAY_NO_TIGER_SUPPORT;
#endif
#else
  return E_OGGPLAY_NO_KATE_SUPPORT;
#endif
}

#define MAX_CHUNK_COUNT   10

OggPlayErrorCode
oggplay_step_decoding(OggPlay *me) {

  OggPlayCallbackInfo  ** info;
  int                     num_records;
  int                     r;
  int                     i;
  int                     need_data  = 0;
  int                     chunk_count = 0;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  




  if (me->trash != NULL && me->buffer->last_emptied > -1) {
    oggplay_take_out_trash(me, me->trash);
    me->trash = NULL;
  }

read_more_data:

  while (1) {
     




    int r;

    if (me->active_tracks == 0) {
      int remaining = 0;
      for (i = 0; i < me->num_tracks; i++) {
        if (me->decode_data[i]->current_loc +
                     me->decode_data[i]->granuleperiod >= me->target + me->decode_data[i]->offset) {
          remaining++;
        }
      }
      if (remaining == 0) {
        return E_OGGPLAY_OK;
      }
    }

    



    need_data = 0;
    for (i = 0; i < me->num_tracks; i++) {
      if (me->decode_data[i]->active == 0)
        continue;
      if (me->decode_data[i]->content_type == OGGZ_CONTENT_CMML)
        continue;
      if (me->decode_data[i]->content_type == OGGZ_CONTENT_KATE)
        continue;
      if
      (
        me->decode_data[i]->current_loc
        <
        me->target + me->decode_data[i]->offset
      )
      {
        need_data = 1;
        break;
      }
    }

    if (!need_data) {
      break;
    }

    





    if (chunk_count > MAX_CHUNK_COUNT) {
      return E_OGGPLAY_TIMEOUT;
    }

    chunk_count += 1;

    r = oggz_read(me->oggz, OGGZ_READ_CHUNK_SIZE);

    
    if (r == 0) {
      num_records = oggplay_callback_info_prepare(me, &info);
     


      for (i = 0; i < me->num_tracks; i++) {
        me->decode_data[i]->active = 0;
        me->active_tracks = 0;
      }

      if (info != NULL) {
        me->callback (me, num_records, info, me->callback_user_ptr);
        oggplay_callback_info_destroy(me, info);
      }

      




      if (me->buffer != NULL && !me->shutdown) {
        oggplay_buffer_set_last_data(me, me->buffer);
      }

      return E_OGGPLAY_OK;
    }

  }
  


  num_records = oggplay_callback_info_prepare (me, &info);
  if (info != NULL) {
    r = me->callback (me, num_records, info, me->callback_user_ptr);
    oggplay_callback_info_destroy (me, info);
  } else {
    r = 0;
  }

  


  for (i = 0; i < me->num_tracks; i++) {
    oggplay_data_clean_list (me->decode_data[i]);
  }

  if (me->shutdown) {
    return E_OGGPLAY_OK;
  }

  if (info == NULL) {
    goto read_more_data;
  }

  me->target += me->callback_period;
  if (r == -1) {
    return E_OGGPLAY_USER_INTERRUPT;
  }

  return E_OGGPLAY_CONTINUE;

}

OggPlayErrorCode
oggplay_start_decoding(OggPlay *me) {

  int r;

  while (1) {
    r = oggplay_step_decoding(me);
    if (r == E_OGGPLAY_CONTINUE || r == E_OGGPLAY_TIMEOUT) {
      continue;
    }
    return (OggPlayErrorCode)r;
  }
}

OggPlayErrorCode
oggplay_close(OggPlay *me) {

  int i;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (me->reader != NULL) {
    me->reader->destroy(me->reader);
  }


  if (me->decode_data) {
    for (i = 0; i < me->num_tracks; i++) {
      oggplay_callback_shutdown(me->decode_data[i]);
    }
  }

  if (me->oggz)
    oggz_close(me->oggz);

  if (me->buffer != NULL) {
    oggplay_buffer_shutdown(me, me->buffer);
  }

  oggplay_free(me->callback_info);
  oggplay_free(me->decode_data);
  oggplay_free(me);

  return E_OGGPLAY_OK;
}





void
oggplay_prepare_for_close(OggPlay *me) {

  me->shutdown = 1;
  if (me->buffer != NULL) {
    SEM_SIGNAL(((OggPlayBuffer *)(me->buffer))->frame_sem);
  }
}

int
oggplay_get_available(OggPlay *me) {

  ogg_int64_t current_time, current_byte;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  current_time = oggz_tell_units(me->oggz);
  current_byte = (ogg_int64_t)oggz_tell(me->oggz);

  return me->reader->available(me->reader, current_byte, current_time);

}

ogg_int64_t
oggplay_get_duration(OggPlay *me) {

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  







  if (me->reader->duration) {
      ogg_int64_t d = me->reader->duration(me->reader);
      if (d >= 0) {
        me->duration = d;
      }
  }

  if (me->duration < 0) {
    ogg_int64_t pos;
    pos = oggz_tell_units(me->oggz);
    me->duration = oggz_seek_units(me->oggz, 0, SEEK_END);
    oggz_seek_units(me->oggz, pos, SEEK_SET);
    oggplay_seek_cleanup(me, pos);
  }

  return me->duration;
}

int
oggplay_media_finished_retrieving(OggPlay *me) {

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (me->reader == NULL) {
    return E_OGGPLAY_BAD_READER;
  }

  return me->reader->finished_retrieving(me->reader);

}

