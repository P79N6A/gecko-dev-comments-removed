





































#ifndef __OGGPLAY_QUERY_H__
#define __OGGPLAY_QUERY_H__

#include <oggz/oggz.h>

int
oggplay_get_num_tracks (OggPlay * me);

OggzStreamContent
oggplay_get_track_type (OggPlay * me, int track_num);

const char *
oggplay_get_track_typename (OggPlay * me, int track_num);

OggPlayErrorCode
oggplay_set_track_active(OggPlay *me, int track_num);

OggPlayErrorCode
oggplay_set_track_inactive(OggPlay *me, int track_num);

#endif
