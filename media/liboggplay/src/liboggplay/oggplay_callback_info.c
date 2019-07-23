




































#include "oggplay_private.h"
#include <stdlib.h>

extern void _print_list(char *name, OggPlayDataHeader *p);

static void
clear_callback_info (OggPlay *me, OggPlayCallbackInfo ***info) {
  int i;
  
  for (i = 0; i < me->num_tracks; ++i) {
    if (((*info)[i] != NULL) && ((*info)[i]->records != NULL)) {
        oggplay_free ((*info)[i]->records);
    }
  }
  oggplay_free (*info);
  *info = NULL;
}

int
oggplay_callback_info_prepare(OggPlay *me, OggPlayCallbackInfo ***info) {

  int i;
  int tcount = 0;
  
  int         added_required_record   = me->num_tracks;
  ogg_int64_t diff;
  ogg_int64_t latest_first_record     = 0x0LL;
  
 
  


  (*info) = oggplay_calloc (me->num_tracks, sizeof (OggPlayCallbackInfo *));
  if ((*info) == NULL)
    return E_OGGPLAY_OUT_OF_MEMORY;
  
  


  for (i = 0; i < me->num_tracks; i++) {
    OggPlayDecode       * track = me->decode_data[i];
    OggPlayCallbackInfo * track_info = me->callback_info + i;
    size_t                count = 0;
    OggPlayDataHeader   * p;
    OggPlayDataHeader   * q = NULL;
    
    (*info)[i] = track_info;

#ifdef HAVE_TIGER
    







    if (track->content_type == OGGZ_CONTENT_KATE) {
      OggPlayKateDecode *decode = (OggPlayKateDecode *)track;
      OggPlayCallbackInfo * video_info = NULL;
      if (decode->overlay_dest >= 0)
        video_info = me->callback_info + decode->overlay_dest;
      oggplay_data_update_tiger(decode, track->active, me->target, video_info);
    }
#endif

    



    if (track->active == 0 && track->data_list == NULL) {
      track_info->data_type = OGGPLAY_INACTIVE;
      track_info->available_records = track_info->required_records = 0;
      track_info->records = NULL;
      track_info->stream_info = OGGPLAY_STREAM_UNINITIALISED;
      added_required_record --;
      continue;
    }
 
    



    for (p = track->data_list; p != NULL; p = p->next) {
      if (!p->has_been_presented) {
        if (q == NULL) {
          q = p;
        }
        
        
        if 
        (
          oggplay_check_add_overflow (count, 1, &count)
          == 
          E_OGGPLAY_TYPE_OVERFLOW
        )
        {
          clear_callback_info (me, info);
          return E_OGGPLAY_TYPE_OVERFLOW;
        }
      }
    }
  
    


    if (count > 0) {
      tcount = 1;

      




      if 
      (
        track->active == 0 
        && 
        (
          track->end_of_data_list->presentation_time
          <=
          me->target + track->offset
        )
      ) 
      {
        track_info->stream_info = OGGPLAY_STREAM_LAST_DATA;
      } else {
        track_info->stream_info = track->stream_info;
      }

    } else {
      track_info->stream_info = OGGPLAY_STREAM_UNINITIALISED;
    }

    if ((count+1) < count) {
      clear_callback_info (me, info);
      return E_OGGPLAY_TYPE_OVERFLOW;
    }
    
    track_info->records = 
      oggplay_calloc ((count+1), sizeof (OggPlayDataHeader *));
    if (track_info->records == NULL) {
      clear_callback_info (me, info);
      return E_OGGPLAY_OUT_OF_MEMORY;
    }

    track_info->records[count] = NULL;

    track_info->available_records = count;
    track_info->required_records = 0;

    track_info->data_type = track->decoded_type;
 
    count = 0;
    for (p = q; p != NULL; p = p->next) {
      if (!p->has_been_presented) {
        track_info->records[count++] = p;
        if (p->presentation_time <= me->target + track->offset) {
          track_info->required_records++;
          p->has_been_presented = 1;
          
        }
      }
    }
     
    if (track_info->required_records > 0) {
      



      if 
      (
        track->stream_info == OGGPLAY_STREAM_FIRST_DATA
        ||
        track->stream_info == OGGPLAY_STREAM_JUST_SEEKED
      ) 
      {
        track->stream_info = OGGPLAY_STREAM_INITIALISED;
      }
 
    }

    











    























     
     
     
    if 
    (
      track->decoded_type == OGGPLAY_CMML 
      ||
      track->decoded_type == OGGPLAY_KATE 
      ||
      (
        track_info->required_records == 0
        &&
        track->active == 1
        && 
        me->pt_update_valid
      )
    ) {
      added_required_record --;
    }

  } 
 
   me->pt_update_valid = 0;
    
  

  










  latest_first_record = 0x0LL;
  if (tcount > 0 && added_required_record == 0) {
    for (i = 0; i < me->num_tracks; i++) {
      OggPlayCallbackInfo * track_info = me->callback_info + i;
      if (track_info->data_type == OGGPLAY_CMML || track_info->data_type == OGGPLAY_KATE) {
        continue;
      }
      if (track_info->available_records > 0) {
        if (track_info->records[0]->presentation_time > latest_first_record) {
          latest_first_record = track_info->records[0]->presentation_time;
        }
      }
    }
    




    diff = latest_first_record - me->target;
    diff = (diff / me->callback_period) * me->callback_period;
    me->target += diff + me->callback_period;


    



    me->presentation_time = me->target - me->callback_period;

    



    for (i = 0; i < me->num_tracks; i++) {
      if ((*info)[i]->records != NULL) 
        oggplay_free((*info)[i]->records);
    }
    oggplay_free(*info);
    (*info) = NULL;

  }

  if (tcount == 0) {
    for (i = 0; i < me->num_tracks; i++) {
      if ((*info)[i]->records != NULL) 
        oggplay_free((*info)[i]->records);
    }
    oggplay_free(*info);
    (*info) = NULL;
  }

  return me->num_tracks;
  
}


void
oggplay_callback_info_destroy(OggPlay *me, OggPlayCallbackInfo **info) {

  int                   i;
  OggPlayCallbackInfo * p;

  for (i = 0; i < me->num_tracks; i++) {
    p = info[i];
    if (me->buffer == NULL && p->records != NULL)
      oggplay_free(p->records);
  }

  oggplay_free(info);

}

OggPlayDataType
oggplay_callback_info_get_type(OggPlayCallbackInfo *info) {

  if (info == NULL) {
    return (OggPlayDataType)E_OGGPLAY_BAD_CALLBACK_INFO;
  }

  return info->data_type;

}

int
oggplay_callback_info_get_available(OggPlayCallbackInfo *info) {

  if (info == NULL) {
    return E_OGGPLAY_BAD_CALLBACK_INFO;
  }

  return info->available_records;

}

int
oggplay_callback_info_get_required(OggPlayCallbackInfo *info) {

  if (info == NULL) {
    return E_OGGPLAY_BAD_CALLBACK_INFO;
  }

  return info->required_records;

}

OggPlayStreamInfo
oggplay_callback_info_get_stream_info(OggPlayCallbackInfo *info) {

  if (info == NULL) {
    return E_OGGPLAY_BAD_CALLBACK_INFO;
  }

  return info->stream_info;
}

OggPlayDataHeader **
oggplay_callback_info_get_headers(OggPlayCallbackInfo *info) {

  if (info == NULL) {
    return NULL;
  }

  return info->records;

}




ogg_int64_t
oggplay_callback_info_get_record_size(OggPlayDataHeader *header) {

  if (header == NULL) {
    return 0;
  }

  return header->samples_in_record;

}

void
oggplay_callback_info_lock_item(OggPlayDataHeader *header) {

  if (header == NULL) {
    return;
  }

  header->lock += 1;

}

void
oggplay_callback_info_unlock_item(OggPlayDataHeader *header) {
  
  if (header == NULL) {
    return;
  }

  header->lock -= 1;
}

long
oggplay_callback_info_get_presentation_time(OggPlayDataHeader *header) {

  if (header == NULL) {
    return -1;
  }

  return OGGPLAY_TIME_FP_TO_INT(header->presentation_time);
}

OggPlayVideoData *
oggplay_callback_info_get_video_data(OggPlayDataHeader *header) {

  if (header == NULL) {
    return NULL;
  }

  return &((OggPlayVideoRecord *)header)->data;

}

OggPlayOverlayData *
oggplay_callback_info_get_overlay_data(OggPlayDataHeader *header) {

  if (header == NULL) {
    return NULL;
  }

  return &((OggPlayOverlayRecord *)header)->data;

}

OggPlayAudioData *
oggplay_callback_info_get_audio_data(OggPlayDataHeader *header) {

  if (header == NULL) {
    return NULL;
  }

  return (OggPlayAudioData *)((OggPlayAudioRecord *)header)->data;
}

OggPlayTextData *
oggplay_callback_info_get_text_data(OggPlayDataHeader *header) {

  if (header == NULL) {
    return NULL;
  }

  return ((OggPlayTextRecord *)header)->data;

}

