





































#ifndef __OGGPLAY_SEEK_H__
#define __OGGPLAY_SEEK_H__

OggPlayErrorCode
oggplay_seek(OggPlay *me, ogg_int64_t milliseconds);




OggPlayErrorCode
oggplay_seek_to_keyframe(OggPlay *me,
                         ogg_int64_t milliseconds,
                         ogg_int64_t offset_begin,
                         ogg_int64_t offset_end);

#endif
