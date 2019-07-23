





































#include "oggplay_private.h"
#include "oggplay/oggplay_callback_info.h"

#include <stdlib.h>
#include <string.h>

#if HAVE_INTTYPES_H
#include <inttypes.h>
#else
#if LONG_MAX==2147483647L
#define PRId64 "lld"
#else
#define PRId64 "ld"
#endif
#endif
















void
oggplay_data_initialise_list (OggPlayDecode *decode) {

  if (decode == NULL) {
    return;
  }
  
  decode->data_list = decode->end_of_data_list = NULL;
  decode->untimed_data_list = NULL;

}




void
oggplay_data_add_to_list_end(OggPlayDecode *decode, OggPlayDataHeader *data) {

  if (decode == NULL) {
    return;
  }
  
  data->next = NULL;

  if (decode->data_list == NULL) {
    decode->data_list = data;
    decode->end_of_data_list = data;
  } else {
    decode->end_of_data_list->next = data;
    decode->end_of_data_list = data;
  }

}




void
oggplay_data_add_to_list_front(OggPlayDecode *decode, OggPlayDataHeader *data) {
  if (decode == NULL) {
    return;
  }
  
  if (decode->data_list == NULL) {
    decode->data_list = decode->end_of_data_list = data;
    data->next = NULL;
  } else {
    data->next = decode->data_list;
    decode->data_list = data;
  }
}

void
_print_list(char *name, OggPlayDataHeader *p) {
    printf("%s: ", name);
    for (; p != NULL; p = p->next) {
      printf("%"PRId64"[%d]", OGGPLAY_TIME_FP_TO_INT (p->presentation_time), p->lock);
      if (p->next != NULL) printf("->");
    }
    printf("\n");
}


void
oggplay_data_add_to_list (OggPlayDecode *decode, OggPlayDataHeader *data) {

  




  ogg_int64_t samples_in_next_in_list;

  if ((decode == NULL) || (data == NULL)) {
    return;
  }

  
  

  if (data->presentation_time == -1) {
    data->next = decode->untimed_data_list;
    decode->untimed_data_list = data;
  } else {
    




    ogg_int64_t presentation_time         = data->presentation_time;
    samples_in_next_in_list               = data->samples_in_record;


    while (decode->untimed_data_list != NULL) {
      OggPlayDataHeader *untimed = decode->untimed_data_list;

      presentation_time -=
                samples_in_next_in_list * decode->granuleperiod;
      untimed->presentation_time = presentation_time;
      decode->untimed_data_list = untimed->next;
      samples_in_next_in_list = untimed->samples_in_record;

      if (untimed->presentation_time >= decode->player->presentation_time) {
        oggplay_data_add_to_list_front(decode, untimed);
      } else {
        oggplay_free(untimed);
      }

    }

    oggplay_data_add_to_list_end(decode, data);

    





    if (decode->stream_info == OGGPLAY_STREAM_UNINITIALISED) {
      decode->stream_info = OGGPLAY_STREAM_FIRST_DATA;
    }

  }

  
  

}

void
oggplay_data_free_list(OggPlayDataHeader *list) {
  OggPlayDataHeader *p;

  while (list != NULL) {
    p = list;
    list = list->next;
    oggplay_free(p);
  }
}

void
oggplay_data_shutdown_list (OggPlayDecode *decode) {

  if (decode == NULL) {
    return;
  }
  
  oggplay_data_free_list(decode->data_list);
  oggplay_data_free_list(decode->untimed_data_list);

}







void
oggplay_data_clean_list (OggPlayDecode *decode) {

  ogg_int64_t         target;
  OggPlayDataHeader * header = NULL;
  OggPlayDataHeader * p      = NULL;
  
  if (decode == NULL) {
    return;
  }
  header = decode->data_list;
  target = decode->player->target;
  
  while (header != NULL) {
    if
    (
      header->lock == 0
      &&
      (
        (
          (header->presentation_time < (target + decode->offset))
          &&
          header->has_been_presented
        )
        ||
        (
          (header->presentation_time < decode->player->presentation_time)
        )
      )

    )
    {
      if (p == NULL) {
        decode->data_list = decode->data_list->next;
        if (decode->data_list == NULL)
          decode->end_of_data_list = NULL;
        oggplay_free (header);
        header = decode->data_list;
      } else {
        if (header->next == NULL)
          decode->end_of_data_list = p;
        p->next = header->next;
        oggplay_free (header);
        header = p->next;
      }
    } else {
      p = header;
      header = header->next;
    }
  }
}

void
oggplay_data_initialise_header (const OggPlayDecode *decode,
                                OggPlayDataHeader *header) {
  
  if ((decode == NULL) || (header == NULL)) {
    return;
  }
  
  



  header->lock = 0;
  header->next = NULL;
  header->presentation_time = decode->current_loc;
  header->has_been_presented = 0;
}

OggPlayErrorCode
oggplay_data_handle_audio_data (OggPlayDecode *decode, void *data,
                                long samples, size_t samplesize) {

  int                   num_channels, ret;
  size_t                record_size = sizeof(OggPlayAudioRecord);
  long                  samples_size;
  OggPlayAudioRecord  * record = NULL;

  num_channels = ((OggPlayAudioDecode *)decode)->sound_info.channels;
  
  
  if ((samples < 0) || (num_channels < 0)) {
    return E_OGGPLAY_TYPE_OVERFLOW;
  }

  ret = oggplay_mul_signed_overflow (samples, num_channels, &samples_size);
  if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
    return ret;
  }

  ret = oggplay_mul_signed_overflow (samples_size, samplesize, &samples_size);
  if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
    return ret;
  }

  ret = oggplay_check_add_overflow (record_size, samples_size, &record_size);
  if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
    return ret;
  }

  
  record = (OggPlayAudioRecord*)oggplay_calloc(record_size, 1);
  if (record == NULL) {
    return E_OGGPLAY_OUT_OF_MEMORY;
  }

  
  oggplay_data_initialise_header(decode, &(record->header));

  record->header.samples_in_record = samples;

  record->data = (void *)(record + 1);
  
  
  memcpy (record->data, data, samples_size);
  



  oggplay_data_add_to_list(decode, &(record->header));
  
  return E_OGGPLAY_CONTINUE;
}

OggPlayErrorCode
oggplay_data_handle_cmml_data(OggPlayDecode *decode, 
                              unsigned char *data, 
                              long size) {

  OggPlayTextRecord * record = NULL;
  size_t              record_size = sizeof(OggPlayTextRecord);

  
  if ((size < 0) || (size+1 < 0)) {
    return E_OGGPLAY_TYPE_OVERFLOW;
  }
  size += 1;
  
  if 
  (
    oggplay_check_add_overflow (record_size, size, &record_size)
    == 
    E_OGGPLAY_TYPE_OVERFLOW
  ) 
  {
    return E_OGGPLAY_TYPE_OVERFLOW;
  }
  
  
  record = (OggPlayTextRecord*)oggplay_calloc (record_size, 1);
  if (record == NULL) {
    return E_OGGPLAY_OUT_OF_MEMORY;
  }

  
  oggplay_data_initialise_header(decode, &(record->header));

  record->header.samples_in_record = 1;
  record->data = (char *)(record + 1);

  
  memcpy(record->data, data, size);
  record->data[size] = '\0';

  oggplay_data_add_to_list(decode, &(record->header));

  return E_OGGPLAY_CONTINUE;
}

static int
get_uv_offset(OggPlayTheoraDecode *decode, yuv_buffer *buffer)
{
  int xo=0, yo = 0;
  if (decode->y_width != 0 &&
      decode->uv_width != 0 &&
      decode->y_width/decode->uv_width != 0) {
    xo = (decode->video_info.offset_x/(decode->y_width/decode->uv_width));
  }
  if (decode->y_height != 0 &&
      decode->uv_height != 0 &&
      decode->y_height/decode->uv_height != 0) {
    yo = (buffer->uv_stride)*(decode->video_info.offset_y/(decode->y_height/decode->uv_height));
  }
  return xo + yo;
}

int
oggplay_data_handle_theora_frame (OggPlayTheoraDecode *decode,
                                  const yuv_buffer *buffer) {

  size_t                size = sizeof (OggPlayVideoRecord);
  int                   i, ret;
  long                  y_size, uv_size, y_offset, uv_offset;
  unsigned char       * p;
  unsigned char       * q;
  unsigned char       * p2;
  unsigned char       * q2;
  OggPlayVideoRecord  * record;
  OggPlayVideoData    * data;

  
  ret = 
    oggplay_mul_signed_overflow (buffer->y_height, buffer->y_stride, &y_size);
  if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
    return ret;
  }

  ret = 
    oggplay_mul_signed_overflow (buffer->uv_height, buffer->uv_stride, &uv_size);
  if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
    return ret;
  }

  ret = oggplay_mul_signed_overflow (uv_size, 2, &uv_size);
  if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
    return ret;
  }

  if (buffer->y_stride < 0) {
    y_size  *= -1;
    uv_size *= -1;
  }

  ret = oggplay_check_add_overflow (size, y_size, &size);
  if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
    return ret;
  }
  
  ret = oggplay_check_add_overflow (size, uv_size, &size);
  if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
    return ret;
  }

  



  record = (OggPlayVideoRecord*)oggplay_malloc (size);

  if (record == NULL) {
    return E_OGGPLAY_OUT_OF_MEMORY;
  }
  
  record->header.samples_in_record = 1;
  data = &(record->data);

  data->y = (unsigned char *)(record + 1);
  data->u = data->y + (decode->y_stride * decode->y_height);
  data->v = data->u + (decode->uv_stride * decode->uv_height);

  



  y_offset = (decode->video_info.offset_x&~1) 
              + buffer->y_stride*(decode->video_info.offset_y&~1);
  p = data->y;
  q = buffer->y + y_offset;
  for (i = 0; i < decode->y_height; i++) {
    memcpy(p, q, decode->y_width);
    p += decode->y_width;
    q += buffer->y_stride;
  }

  uv_offset = get_uv_offset(decode, buffer);
  p = data->u;
  q = buffer->u + uv_offset;
  p2 = data->v;
  q2 = buffer->v + uv_offset;
  for (i = 0; i < decode->uv_height; i++) {
    memcpy(p, q, decode->uv_width);
    memcpy(p2, q2, decode->uv_width);
    p += decode->uv_width;
    p2 += decode->uv_width;
    q += buffer->uv_stride;
    q2 += buffer->uv_stride;
  }

  
  if (decode->convert_to_rgb) {
    OggPlayYUVChannels      yuv;
    OggPlayRGBChannels      rgb;
    OggPlayOverlayRecord  * orecord;
    OggPlayOverlayData    * odata;
    long                    overlay_size;

    yuv.ptry = data->y;
    yuv.ptru = data->u;
    yuv.ptrv = data->v;
    yuv.y_width = decode->y_width;
    yuv.y_height = decode->y_height;
    yuv.uv_width = decode->uv_width;
    yuv.uv_height = decode->uv_height;

    size = sizeof(OggPlayOverlayRecord);
    
    ret = oggplay_mul_signed_overflow(decode->y_width, decode->y_height, 
                                      &overlay_size);
    if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
      return ret;
    }
    
    ret = oggplay_mul_signed_overflow(overlay_size, 4, &overlay_size);
    if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
      return ret;
    }

    ret =  oggplay_check_add_overflow (size, overlay_size, &size);
    if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
      return ret;
    }

    
    orecord = (OggPlayOverlayRecord*) oggplay_malloc (size);
    if (orecord != NULL) {
      oggplay_data_initialise_header((OggPlayDecode *)decode, &(orecord->header));
      orecord->header.samples_in_record = 1;
      odata = &(orecord->data);

      rgb.ptro = (unsigned char*)(orecord+1);
      rgb.rgb_width = yuv.y_width;
      rgb.rgb_height = yuv.y_height;

      if (!decode->swap_rgb) {
        oggplay_yuv2bgra(&yuv, &rgb);
      } else {
        oggplay_yuv2rgba(&yuv, &rgb);
      }
      
      odata->rgb = rgb.ptro;
      odata->rgba = NULL;
      odata->width = rgb.rgb_width;
      odata->height = rgb.rgb_height;
      odata->stride = rgb.rgb_width*4;

      oggplay_free(record);
    
      oggplay_data_add_to_list((OggPlayDecode *)decode, &(orecord->header));
    } else {
      
      oggplay_free (record);
      
      return E_OGGPLAY_OUT_OF_MEMORY;
    }
  }
  else {
    oggplay_data_initialise_header((OggPlayDecode *)decode, &(record->header));
    oggplay_data_add_to_list((OggPlayDecode *)decode, &(record->header));
  }
  
  return E_OGGPLAY_CONTINUE;
}

#ifdef HAVE_KATE
OggPlayErrorCode
oggplay_data_handle_kate_data(OggPlayKateDecode *decode, const kate_event *ev) {

  OggPlayTextRecord * record = NULL;
  size_t              rec_size = sizeof(OggPlayTextRecord);
  
  if (decode == NULL) {
    return -1; 
  }
#ifdef HAVE_TIGER
  tiger_renderer_add_event(decode->tr, ev->ki, ev);

  if (decode->use_tiger) {
    

  }
  else
#endif
  {
    
    if 
    ( 
      oggplay_check_add_overflow (rec_size, ev->len0, &rec_size)
      == 
      E_OGGPLAY_TYPE_OVERFLOW
    )
    {
      return E_OGGPLAY_TYPE_OVERFLOW;
    }
    
    record = (OggPlayTextRecord*)oggplay_calloc (rec_size, 1);
    if (!record) {
      return E_OGGPLAY_OUT_OF_MEMORY;
    }

    oggplay_data_initialise_header(&decode->decoder, &(record->header));

    
    record->header.samples_in_record = (ev->end_time-ev->start_time)*1000;
    record->data = (char *)(record + 1);

    memcpy(record->data, ev->text, ev->len0);

    oggplay_data_add_to_list(&decode->decoder, &(record->header));
  }
  
  return E_OGGPLAY_CONTINUE;
}
#endif

#ifdef HAVE_TIGER
OggPlayErrorCode
oggplay_data_update_tiger(OggPlayKateDecode *decode, int active, ogg_int64_t presentation_time, OggPlayCallbackInfo *info) {

  OggPlayOverlayRecord  * record = NULL;
  OggPlayOverlayData    * data = NULL;
  size_t                size = sizeof (OggPlayOverlayRecord);
  int                   track = active && decode->use_tiger;
  int                   ret;
  kate_float            t = OGGPLAY_TIME_FP_TO_INT(presentation_time) / 1000.0f;

  if (!decode->decoder.initialised) return -1;

  if (track) {
    if (info) {
      if (info->required_records>0) {
        OggPlayDataHeader *header = info->records[0];
        data = (OggPlayOverlayData*)(header+1);
        if (decode->tr && data->rgb) {
#if WORDS_BIGENDIAN || IS_BIG_ENDIAN
          tiger_renderer_set_buffer(decode->tr, data->rgb, data->width, data->height, data->stride, 0);
#else
          tiger_renderer_set_buffer(decode->tr, data->rgb, data->width, data->height, data->stride, 1);
#endif
        }
        else {
          
          
          return -1;
        }
      }
      else {
        
        
        return -1;
      }
    }
    else {
      
      int width = decode->k_state.ki->original_canvas_width;
      int height = decode->k_state.ki->original_canvas_height;
      long overlay_size;
      if (width <= 0 || height <= 0) {
        
        if (decode->default_width > 0 && decode->default_height > 0) {
          width = decode->default_width;
          height = decode->default_height;
        }
        else {
          width = 640;
          height = 480;
        }
      }
      
      ret = oggplay_mul_signed_overflow (width, height, &overlay_size);
      if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
        return ret;
      }
      
      ret = oggplay_mul_signed_overflow (overlay_size, 4, &overlay_size);
      if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
        return E_OGGPLAY_TYPE_OVERFLOW;
      }
      
      ret = oggplay_check_add_overflow (size, overlay_size, &size);
      if (ret == E_OGGPLAY_TYPE_OVERFLOW) {
        return E_OGGPLAY_TYPE_OVERFLOW;
      }

      record = (OggPlayOverlayRecord*)oggplay_calloc (1, size);
      if (!record)
        return E_OGGPLAY_OUT_OF_MEMORY;

      record->header.samples_in_record = 1;
      data= &(record->data);
      oggplay_data_initialise_header((OggPlayDecode *)decode, &(record->header));

      data->rgba = (unsigned char*)(record+1);
      data->rgb = NULL;
      data->width = width;
      data->height = height;
      data->stride = width*4;

      if (decode->tr && data->rgba) {
        tiger_renderer_set_buffer(decode->tr, data->rgba, data->width, data->height, data->stride, decode->swap_rgb);
      }

      oggplay_data_add_to_list(&decode->decoder, &(record->header));
      record->header.presentation_time=presentation_time;
    }
  }

  if (decode->tr) {
    tiger_renderer_update(decode->tr, t, track);
  }

  if (track) {
    
    if (decode->tr) {
      tiger_renderer_render(decode->tr);
    }
  }
  
  return E_OGGPLAY_CONTINUE;
}
#endif

