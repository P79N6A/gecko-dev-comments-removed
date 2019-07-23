





































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

void
oggplay_seek_cleanup(OggPlay* me, ogg_int64_t milliseconds)
{

  OggPlaySeekTrash    * trash;
  OggPlaySeekTrash   ** p;
  OggPlayDataHeader  ** end_of_list_p;
  int                   i;

  





  trash = calloc(sizeof(OggPlaySeekTrash), 1);

  


  trash->old_buffer = (OggPlayBuffer *)me->buffer;

  



  me->buffer = oggplay_buffer_new_buffer(me->buffer->buffer_size);

  




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
    track->stream_info = OGGPLAY_STREAM_JUST_SEEKED;
  }

  


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
      free(p);
    }
    p = trash;
  }

  if (p != NULL) {
    free(p);
  }
}
