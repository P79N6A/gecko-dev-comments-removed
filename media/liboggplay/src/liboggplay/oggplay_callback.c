




































#include "oggplay_private.h"

#define TIME_THEORA_DECODE 0

#include <stdlib.h>
#if TIME_THEORA_DECODE
#include <sys/time.h>
#endif
#include <time.h>
#include <string.h>

void
oggplay_init_theora(void *user_data) {

  OggPlayTheoraDecode   * decoder     = (OggPlayTheoraDecode *)user_data;

  theora_info_init(&(decoder->video_info));
  theora_comment_init(&(decoder->video_comment));
  decoder->remaining_header_packets = 3;
  decoder->granulepos_seen = 0;
  decoder->frame_delta = 0;
  decoder->y_width = 0;
  decoder->convert_to_rgb = 0;
  decoder->decoder.decoded_type = OGGPLAY_YUV_VIDEO;
  decoder->decoder.player->active_tracks++;
}

void
oggplay_shutdown_theora(void *user_data) {

  OggPlayTheoraDecode   * decoder = (OggPlayTheoraDecode *)user_data;

  if (decoder->remaining_header_packets == 0) {
    theora_clear(&(decoder->video_handle));
  }
  theora_info_clear(&(decoder->video_info));
  theora_comment_clear(&(decoder->video_comment));
}

int
oggplay_callback_theora (OGGZ * oggz, ogg_packet * op, long serialno,
                void * user_data) {

  OggPlayTheoraDecode   * decoder     = (OggPlayTheoraDecode *)user_data;
  OggPlayDecode         * common      = &(decoder->decoder);
  ogg_int64_t             granulepos  = oggz_tell_granulepos(oggz);
  yuv_buffer              buffer;
  int granuleshift;
  long frame;

#if TIME_THEORA_DECODE
  struct timeval          tv;
  struct timeval          tv2;
  int                     musec;
#endif

  if ( (granulepos > 0) && (common->last_granulepos > granulepos)) {
    




    return 0;
  }

  


  if (theora_packet_isheader(op)) {
    if (theora_decode_header(&(decoder->video_info), &(decoder->video_comment), op) < 0)
      return -1;

    




    decoder->y_width = decoder->y_stride = decoder->video_info.frame_width;
    decoder->y_height = decoder->video_info.frame_height;
    decoder->uv_width = decoder->uv_stride = decoder->video_info.frame_width / 2;
    decoder->uv_height = decoder->video_info.frame_height / 2;
  
    if (decoder->y_width == 0 ||
        decoder->y_height == 0 || 
        decoder->uv_width == 0 ||
        decoder->uv_height == 0) {
      decoder->decoder.active = 0;
      return 0;
    }
    
    if (--(decoder->remaining_header_packets) == 0) {
      
      if (((decoder->video_info.height - decoder->video_info.offset_y)<decoder->video_info.frame_height)||
          ((decoder->video_info.width - decoder->video_info.offset_x)<decoder->video_info.frame_width))
          return -1;
          
      theora_decode_init(&(decoder->video_handle), &(decoder->video_info));
    }
    return 0;
  }
  else if (decoder->remaining_header_packets != 0) {
    



    return -1;
  }

  if (!decoder->decoder.active) {
    


    return 0;
  }

  


  if (common->current_loc == -1)
    common->current_loc = 0;

  



#if TIME_THEORA_DECODE
  gettimeofday(&tv, NULL);
#endif

  if (theora_decode_packetin(&(decoder->video_handle), op) < 0)
    return -1;

  if (theora_decode_YUVout(&(decoder->video_handle), &buffer) < 0)
    return -1;

#if TIME_THEORA_DECODE
  gettimeofday(&tv2, NULL);
  musec = tv2.tv_usec - tv.tv_usec;
  if (tv2.tv_sec > tv.tv_sec)
    musec += (tv2.tv_sec - tv.tv_sec) * 1000000;
  printf("decode took %dus\n", musec);
#endif

  if (granulepos != -1) {
    



    common->last_granulepos = granulepos;

    
    granuleshift = oggz_get_granuleshift(oggz, serialno);
    frame = (granulepos >> granuleshift);
    frame += (granulepos & ((1 << granuleshift) - 1));
    
    
    common->current_loc = frame * common->granuleperiod;    
  } else {
    common->current_loc = -1;
  }

  if
  (
    (common->current_loc == -1)
    ||
    (common->current_loc >= common->player->presentation_time)
  )
  {
    




    oggplay_data_handle_theora_frame(decoder, &buffer);
  }

  if (op->e_o_s) {
    common->active = 0;
    common->player->active_tracks--;
  }

  return 0;

}

void
oggplay_init_cmml (void * user_data) {

  OggPlayCmmlDecode * decoder = (OggPlayCmmlDecode *)user_data;
  decoder->decoder.decoded_type = OGGPLAY_CMML;
  decoder->granuleshift = 32; 
}

int
oggplay_callback_cmml (OGGZ * oggz, ogg_packet * op, long serialno,
                void * user_data) {

  OggPlayCmmlDecode * decoder     = (OggPlayCmmlDecode *)user_data;
  OggPlayDecode     * common      = &(decoder->decoder);
  ogg_int64_t         granulepos  = oggz_tell_granulepos (oggz);

  if (granulepos == 0) {
    if (memcmp(op->packet, "CMML\0\0\0\0", 8) == 0) {
      decoder->granuleshift = op->packet[28];
    }
  } else {

    if (decoder->granuleshift > 0) {
      granulepos >>= decoder->granuleshift;
    }

    common->current_loc = granulepos * common->granuleperiod;
    common->last_granulepos = granulepos;

    oggplay_data_handle_cmml_data (&(decoder->decoder), op->packet, op->bytes);
  }

  return 0;

}

void
oggplay_init_skel (void * user_data) {

  OggPlaySkeletonDecode * decoder = (OggPlaySkeletonDecode *)user_data;

  decoder->presentation_time = 0;
  decoder->base_time = 0;
}

static inline unsigned long extract_int32(unsigned char *data) {
  return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

static inline ogg_int64_t extract_int64(unsigned char *data) {
  return ((ogg_int64_t)(extract_int32(data))) |
         (((ogg_int64_t)(extract_int32(data + 4))) << 32);
}

int
oggplay_callback_skel (OGGZ * oggz, ogg_packet * op, long serialno,
                void * user_data) {

  OggPlaySkeletonDecode * decoder = (OggPlaySkeletonDecode *)user_data;

  if (strncmp((char *)op->packet, "fishead", 7) == 0) {
    ogg_int64_t pt_num, pt_den, bt_num, bt_den;

    pt_num = extract_int64(op->packet + 12);
    pt_den = extract_int64(op->packet + 20);
    bt_num = extract_int64(op->packet + 28);
    bt_den = extract_int64(op->packet + 36);

    if (pt_den != 0) {
      decoder->presentation_time = OGGPLAY_TIME_INT_TO_FP(pt_num) / pt_den;
    } else {
      decoder->presentation_time = 0;
    }
    if (bt_den != 0) {
      decoder->base_time = OGGPLAY_TIME_INT_TO_FP(bt_num) / bt_den;
    } else {
      decoder->base_time = 0;
    }

    
    decoder->decoder.player->presentation_time = decoder->presentation_time;
  } else {
    int i;
    long          preroll       = extract_int32(op->packet + 44);
    long          serialno      = extract_int32(op->packet + 12);
    

    for (i = 1; i < decoder->decoder.player->num_tracks; i++) {
      if (decoder->decoder.player->decode_data[i]->serialno == serialno) {
        decoder->decoder.player->decode_data[i]->preroll = preroll;
        break;
      }
    }
  }

  return 0;

}

int
oggplay_fish_sound_callback_floats(FishSound * fsound, float ** pcm, 
                                          long frames, void *user_data) {

  OggPlayAudioDecode *decoder = (OggPlayAudioDecode *)user_data;
  OggPlayDecode *common = &(decoder->decoder);

  



  if (common->last_granulepos > 0) {
    common->current_loc = common->last_granulepos * common->granuleperiod;
  } else {
    common->current_loc = -1;
  }

  if
  (
    (common->current_loc == -1)
    ||
    (common->current_loc >= common->player->presentation_time)
  )
  {


    


    oggplay_data_handle_audio_data(&(decoder->decoder), (short *)pcm, frames,
              sizeof(float));

      return FISH_SOUND_STOP_ERR;
  }

  return FISH_SOUND_CONTINUE;
}

void
oggplay_init_audio (void * user_data) {

  OggPlayAudioDecode  * decoder = (OggPlayAudioDecode *)user_data;

  decoder->sound_handle = fish_sound_new(FISH_SOUND_DECODE,
                                                      &(decoder->sound_info));

  decoder->sound_info.channels = 0;
  fish_sound_set_interleave(decoder->sound_handle, 1);
  fish_sound_set_decoded_float_ilv(decoder->sound_handle,
                                      oggplay_fish_sound_callback_floats,
                                      (void *)decoder);

  decoder->decoder.decoded_type = OGGPLAY_FLOATS_AUDIO;
  decoder->decoder.player->active_tracks++;
}

void
oggplay_shutdown_audio(void *user_data) {

  OggPlayAudioDecode   * decoder = (OggPlayAudioDecode *)user_data;

  fish_sound_delete(decoder->sound_handle);

}

int
oggplay_callback_audio (OGGZ * oggz, ogg_packet * op, long serialno,
                void * user_data) {

  OggPlayAudioDecode   * decoder     = (OggPlayAudioDecode *)user_data;
  OggPlayDecode        * common      = &(decoder->decoder);
  ogg_int64_t            granulepos  = oggz_tell_granulepos(oggz);

  if (granulepos > 0 && (!decoder->decoder.active)) {
    return 0;
  }

  common->last_granulepos = granulepos;

  fish_sound_prepare_truncation (decoder->sound_handle, op->granulepos,
                                                                op->e_o_s);
  if (fish_sound_decode (decoder->sound_handle, op->packet, op->bytes) == -1) {
    
    op->e_o_s = 1;
    common->active = 0;
    common->player->active_tracks--;
    return OGGZ_ERR_HOLE_IN_DATA;
  }

  if (decoder->sound_info.channels == 0) {
    fish_sound_command(decoder->sound_handle, FISH_SOUND_GET_INFO,
                    &(decoder->sound_info), sizeof(FishSoundInfo));
  }

  if (op->e_o_s) {
    common->active = 0;
    common->player->active_tracks--;
  }

  return 0;
}

void
oggplay_init_kate(void *user_data) {

#ifdef HAVE_KATE
  int ret;
  OggPlayKateDecode   * decoder     = (OggPlayKateDecode *)user_data;

  decoder->init = 0;
  ret = kate_high_decode_init(&(decoder->k));
  if (ret < 0) {
    
  }
  else {
    decoder->init = 1;
  }
  decoder->decoder.decoded_type = OGGPLAY_KATE;

#ifdef HAVE_TIGER
  decoder->use_tiger = 1;
  decoder->overlay_dest = -1;

  ret = tiger_renderer_create(&(decoder->tr));
  if (ret < 0) {
    
    decoder->tr = NULL;
  }
  if (decoder->use_tiger) {
    decoder->decoder.decoded_type = OGGPLAY_RGBA_VIDEO;
  }
#endif

#endif
}

void
oggplay_shutdown_kate(void *user_data) {

#ifdef HAVE_KATE
  OggPlayKateDecode   * decoder = (OggPlayKateDecode *)user_data;

#ifdef HAVE_TIGER
  if (decoder->tr) {
    tiger_renderer_destroy(decoder->tr);
  }
#endif

  if (decoder->init) {
    kate_high_decode_clear(&(decoder->k));
  }
#endif
}

int
oggplay_callback_kate (OGGZ * oggz, ogg_packet * op, long serialno,
                void * user_data) {

#ifdef HAVE_KATE
  OggPlayKateDecode     * decoder     = (OggPlayKateDecode *)user_data;
  OggPlayDecode         * common      = &(decoder->decoder);
  ogg_int64_t             granulepos  = oggz_tell_granulepos(oggz);
  int                     granuleshift;
  ogg_int64_t             base, offset;
  kate_packet kp;
  const kate_event *ev = NULL;
  int ret;

  if (!decoder->init) {
    return E_OGGPLAY_UNINITIALISED;
  }

  kate_packet_wrap(&kp, op->bytes, op->packet);
  ret = kate_high_decode_packetin(&(decoder->k), &kp, &ev);
  if (ret < 0) {
    return E_OGGPLAY_BAD_INPUT;
  }

  if (granulepos != -1) {
    granuleshift = oggz_get_granuleshift(oggz, serialno);
    base = (granulepos >> granuleshift);
    offset = granulepos - (base << granuleshift);
    common->current_loc = (base+offset) * common->granuleperiod;
    common->last_granulepos = granulepos;
  } else {
    common->current_loc = -1;
  }

  if
  (
    (common->current_loc == -1)
    ||
    (common->current_loc >= common->player->presentation_time)
  )
  {
    


    if (ev) {
      oggplay_data_handle_kate_data(decoder, ev);
    }
  }

  if (op->e_o_s) {
    common->active = 0;
  }

#endif

  return 0;

}

OggPlayCallbackFunctions callbacks[] = {
  {oggplay_init_theora, oggplay_callback_theora, oggplay_shutdown_theora,
        sizeof(OggPlayTheoraDecode)},        
  {oggplay_init_audio, oggplay_callback_audio, oggplay_shutdown_audio,
        sizeof(OggPlayAudioDecode)},         
  {oggplay_init_audio, oggplay_callback_audio, oggplay_shutdown_audio,
        sizeof(OggPlayAudioDecode)},         
  {NULL, NULL, NULL, sizeof(OggPlayDecode)}, 
  {oggplay_init_cmml, oggplay_callback_cmml, NULL, sizeof(OggPlayCmmlDecode)},
  {NULL, NULL, NULL, sizeof(OggPlayDecode)}, 
  {oggplay_init_skel, oggplay_callback_skel, NULL,
        sizeof(OggPlaySkeletonDecode)},
  {NULL, NULL, NULL, sizeof(OggPlayDecode)}, 
  {NULL, NULL, NULL, sizeof(OggPlayDecode)}, 
  {NULL, NULL, NULL, sizeof(OggPlayDecode)}, 
  {NULL, NULL, NULL, sizeof(OggPlayDecode)}, 
  {oggplay_init_kate, oggplay_callback_kate, oggplay_shutdown_kate,
        sizeof(OggPlayKateDecode)},          
  {NULL, NULL, NULL, sizeof(OggPlayDecode)}, 
  {NULL, NULL, NULL, sizeof(OggPlayDecode)}  
};

OggPlayDecode *
oggplay_initialise_decoder(OggPlay *me, int content_type, int serialno) {

  ogg_int64_t    num;
  ogg_int64_t    denom;
  OggPlayDecode *decoder = NULL;

  if (me == NULL)
    return NULL;

  decoder = oggplay_malloc (callbacks[content_type].size);

  if (decoder == NULL)
    return NULL;

  decoder->serialno = serialno;
  decoder->content_type = content_type;
  decoder->content_type_name =
          oggz_stream_get_content_type (me->oggz, serialno);
  decoder->active = 1;
  decoder->final_granulepos = -1;
  decoder->player = me;
  decoder->decoded_type = OGGPLAY_TYPE_UNKNOWN;

  


  decoder->stream_info = OGGPLAY_STREAM_UNINITIALISED;

  


  decoder->current_loc = -1;
  decoder->last_granulepos = -1;

  




  decoder->offset = 0;

  oggz_get_granulerate(me->oggz, serialno, &num, &denom);

  


  if (num != 0) {
    decoder->granuleperiod = OGGPLAY_TIME_INT_TO_FP(denom) / num;
  } else {
    decoder->granuleperiod = 0;
  }

  if (callbacks[content_type].init != NULL) {
    callbacks[content_type].init(decoder);
  }

  oggplay_data_initialise_list(decoder);

  return decoder;
}





void
oggplay_callback_shutdown(OggPlayDecode *decoder) {

  if (callbacks[decoder->content_type].shutdown != NULL) {
    callbacks[decoder->content_type].shutdown(decoder);
  }

  oggplay_data_shutdown_list(decoder);

  oggplay_free(decoder);
}







int
oggplay_callback_predetected (OGGZ *oggz, ogg_packet *op, long serialno,
                void *user_data) {

  OggPlay     * me;
  int           i;
  int           content_type  = 0;

  me = (OggPlay *)user_data;
  content_type = oggz_stream_get_content (me->oggz, serialno);

  



  for (i = 0; i < me->num_tracks; i++) {
    if (serialno == me->decode_data[i]->serialno) {
      int ret = 0;
      
      


      if (callbacks[content_type].callback != NULL) {
        ret = callbacks[content_type].callback(oggz, op, serialno,
                                               me->decode_data[i]);
      }

      if 
      (
        (op->granulepos >= 0) 
        ||
        (op->granulepos == -1 && me->decode_data[i]->last_granulepos != -1)
      )
      {
        


        for (i = 0; i < me->num_tracks; i++) {
          serialno = me->decode_data[i]->serialno;
          content_type = oggz_stream_get_content (me->oggz, serialno);
          oggz_set_read_callback(me->oggz, serialno,
                          callbacks[content_type].callback, me->decode_data[i]);
        }

        


        oggz_set_read_callback (me->oggz, -1, NULL, NULL);
        me->all_tracks_initialised = 1;
      }

      return ret < 0 ? OGGZ_ERR_HOLE_IN_DATA : ret;
    }
  }

  me->callback_info = oggplay_realloc (me->callback_info,
                  sizeof (OggPlayCallbackInfo) * ++me->num_tracks);
  if (me->callback_info == NULL)
    return -1;

  me->decode_data = oggplay_realloc (me->decode_data, sizeof (long) * me->num_tracks);
  if (me->decode_data == NULL)
    return -1;

  me->decode_data[me->num_tracks - 1] = oggplay_initialise_decoder(me,
                                                      content_type, serialno);
  if (me->decode_data[me->num_tracks - 1] == NULL)
    return -1; 

  

  


  if (callbacks[content_type].callback != NULL) {
    return callbacks[content_type].callback(oggz, op, serialno,
                                            me->decode_data[me->num_tracks - 1]);
  }

  return 0;

}
