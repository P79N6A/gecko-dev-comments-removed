





































#ifndef __OGGPLAY_BUFFER_H__
#define __OGGPLAY_BUFFER_H__








OggPlayBuffer *
oggplay_buffer_new_buffer(int size);

int
oggplay_buffer_is_full(volatile OggPlayBuffer *buffer);

void
oggplay_buffer_shutdown(OggPlay *me, volatile OggPlayBuffer *buffer);

void
oggplay_buffer_prepare(OggPlay *me);

#endif
