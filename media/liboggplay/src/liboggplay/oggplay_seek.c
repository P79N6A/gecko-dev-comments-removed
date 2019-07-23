





































#include "oggplay_private.h"

OggPlayErrorCode
oggplay_seek(OggPlay *me, ogg_int64_t milliseconds) {

  ogg_int64_t           eof;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (milliseconds < 0) {
    return E_OGGPLAY_CANT_SEEK;
  }

  eof = oggplay_get_duration(me);
  if (eof > -1 && milliseconds > eof) {
    return E_OGGPLAY_CANT_SEEK;
  }

  if (me->reader->seek != NULL) {
    if
    (
      me->reader->seek(me->reader, me->oggz, milliseconds)
      ==
      E_OGGPLAY_CANT_SEEK
    )
    {
      return E_OGGPLAY_CANT_SEEK;
    }
  } else {
    if (oggz_seek_units(me->oggz, milliseconds, SEEK_SET) == -1) {
      return E_OGGPLAY_CANT_SEEK;
    }
  }

  oggplay_seek_cleanup(me, milliseconds);

  return E_OGGPLAY_OK;

}

OggPlayErrorCode
oggplay_seek_to_keyframe(OggPlay *me,
                         ogg_int64_t milliseconds,
                         ogg_int64_t offset_begin,
                         ogg_int64_t offset_end)
{
  ogg_int64_t eof, time;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (milliseconds < 0)
    return E_OGGPLAY_CANT_SEEK;
  
  eof = oggplay_get_duration(me);
  if (eof > -1 && milliseconds > eof) {
    return E_OGGPLAY_CANT_SEEK;
  }

  time = oggz_keyframe_seek_set(me->oggz,
                                milliseconds,
                                offset_begin,
                                offset_end);

  if (time == -1) {
    return E_OGGPLAY_CANT_SEEK;
  }

  oggplay_seek_cleanup(me, time);

  return E_OGGPLAY_OK;

}

void
oggplay_seek_cleanup(OggPlay* me, ogg_int64_t milliseconds)
{

  OggPlaySeekTrash    * trash;
  OggPlaySeekTrash   ** p;
  OggPlayDataHeader  ** end_of_list_p;
  int                   i;

  if (me  == NULL)
    return;

  





  trash = oggplay_calloc(1, sizeof(OggPlaySeekTrash));

  if (trash == NULL)
    return;

  


  if (me->buffer != NULL) {

    trash->old_buffer = (OggPlayBuffer *)me->buffer;

    



    me->buffer = oggplay_buffer_new_buffer(me->buffer->buffer_size);

    if (me->buffer == NULL) {
      return;
    }
  }
  
  




  end_of_list_p = &trash->old_data;
  for (i = 0; i < me->num_tracks; i++) {
    OggPlayDecode *track = me->decode_data[i];
    if (track->data_list != NULL) {
      *(end_of_list_p) = track->data_list;
      end_of_list_p = &(track->end_of_data_list->next);
      oggplay_data_free_list(track->untimed_data_list);
    }
    track->data_list = track->end_of_data_list = NULL;
    track->untimed_data_list = NULL;
    track->current_loc = -1;
    track->last_granulepos = -1;
    track->stream_info = OGGPLAY_STREAM_JUST_SEEKED;
  }

  



#ifdef HAVE_TIGER
  for (i = 0; i < me->num_tracks; i++) {
    OggPlayDecode *track = me->decode_data[i];
    if (track && track->content_type == OGGZ_CONTENT_KATE) {
      OggPlayKateDecode *decode = (OggPlayKateDecode *)(me->decode_data[i]);
      if (decode->use_tiger) tiger_renderer_seek(decode->tr, milliseconds/1000.0);
    }
  }
#endif

  


  me->presentation_time = milliseconds;
  me->target = me->callback_period - 1;
  me->pt_update_valid = 1;

  trash->next = NULL;

  p = &(me->trash);
  while (*p != NULL) {
    p = &((*p)->next);
  }

  *p = trash;
}

void
oggplay_take_out_trash(OggPlay *me, OggPlaySeekTrash *trash) {

  OggPlaySeekTrash *p = NULL;

  for (; trash != NULL; trash = trash->next) {

    oggplay_buffer_shutdown(me, trash->old_buffer);
    oggplay_data_free_list(trash->old_data);
    if (p != NULL) {
      oggplay_free(p);
    }
    p = trash;
  }

  if (p != NULL) {
    oggplay_free(p);
  }
}
