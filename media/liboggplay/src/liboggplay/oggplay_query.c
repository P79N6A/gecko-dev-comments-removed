





































#include "oggplay_private.h"

int
oggplay_get_num_tracks (OggPlay * me) {

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (me->reader == NULL) {
    return E_OGGPLAY_BAD_READER;
  }

  if (me->all_tracks_initialised == 0) {
    return E_OGGPLAY_UNINITIALISED;
  }

  return me->num_tracks;

}

OggzStreamContent
oggplay_get_track_type (OggPlay * me, int track_num) {

  if (me == NULL) {
    return (OggzStreamContent)E_OGGPLAY_BAD_OGGPLAY;
  }

  if (me->reader == NULL) {
    return (OggzStreamContent)E_OGGPLAY_BAD_READER;
  }

  if (me->all_tracks_initialised == 0) {
    return E_OGGPLAY_UNINITIALISED;
  }

  if (track_num < 0 || track_num >= me->num_tracks) {
    return (OggzStreamContent)E_OGGPLAY_BAD_TRACK;
  }

  return (OggzStreamContent)me->decode_data[track_num]->content_type;
}

const char *
oggplay_get_track_typename (OggPlay * me, int track_num) {

  if (me == NULL) {
    return NULL;
  }

  if (me->reader == NULL) {
    return NULL;
  }

  if (me->all_tracks_initialised == 0) {
    return NULL;
  }

  if (track_num < 0 || track_num >= me->num_tracks) {
    return NULL;
  }

  return me->decode_data[track_num]->content_type_name;
}

OggPlayErrorCode
oggplay_set_track_active(OggPlay *me, int track_num) {

  ogg_int64_t p;

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (me->reader == NULL) {
    return E_OGGPLAY_BAD_READER;
  }

  if (me->all_tracks_initialised == 0) {
    return E_OGGPLAY_UNINITIALISED;
  }

  if (track_num < 0 || track_num >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  




  if (me->decode_data[track_num]->content_type == OGGZ_CONTENT_SKELETON) {
    return E_OGGPLAY_TRACK_IS_SKELETON;
  }
  
  if (me->decode_data[track_num]->content_type == OGGZ_CONTENT_UNKNOWN) {
    return E_OGGPLAY_TRACK_IS_UNKNOWN;
  }

  
  if (me->decode_data[track_num]->initialised != 1) {
    return E_OGGPLAY_TRACK_UNINITIALISED;
  }

  if ((p = me->decode_data[track_num]->final_granulepos) != -1) {
    if (p * me->decode_data[track_num]->granuleperiod > me->target) {
      return E_OGGPLAY_TRACK_IS_OVER;
    }
  }

  if (me->decode_data[track_num]->active == 0) {
    me->decode_data[track_num]->active = 1;

    






    if (me->decode_data[track_num]->content_type != OGGZ_CONTENT_CMML && me->decode_data[track_num]->content_type != OGGZ_CONTENT_KATE) {
      me->active_tracks ++;
    }
  }

  return E_OGGPLAY_OK;

}

OggPlayErrorCode
oggplay_set_track_inactive(OggPlay *me, int track_num) {

  if (me == NULL) {
    return E_OGGPLAY_BAD_OGGPLAY;
  }

  if (me->reader == NULL) {
    return E_OGGPLAY_BAD_READER;
  }

  if (me->all_tracks_initialised == 0) {
    return E_OGGPLAY_UNINITIALISED;
  }

  if (track_num < 0 || track_num >= me->num_tracks) {
    return E_OGGPLAY_BAD_TRACK;
  }

  if (me->decode_data[track_num]->content_type == OGGZ_CONTENT_SKELETON) {
    return E_OGGPLAY_TRACK_IS_SKELETON;
  }

  if (me->decode_data[track_num]->content_type == OGGZ_CONTENT_UNKNOWN) {
    return E_OGGPLAY_TRACK_IS_UNKNOWN;
  }

  if (me->decode_data[track_num]->active == 1) {
    me->decode_data[track_num]->active = 0;

    


    if (me->decode_data[track_num]->content_type != OGGZ_CONTENT_CMML && me->decode_data[track_num]->content_type != OGGZ_CONTENT_KATE) {
      me->active_tracks --;
    }
  }

  return E_OGGPLAY_OK;
}
