






































#include "oggplay_private.h"
#include "oggplay_file_reader.h"

#include <stdlib.h>
#include <string.h>

OggPlayErrorCode
oggplay_file_reader_initialise(OggPlayReader * opr, int block) {

  OggPlayFileReader * me = (OggPlayFileReader *)opr;
  (void)block; 

  if (me == NULL) {
    return E_OGGPLAY_BAD_READER;
  }

  me->file = fopen(me->file_name, "rb");

  if (me->file == NULL) {
    return E_OGGPLAY_BAD_INPUT;
  }

  fseek(me->file, 0L, SEEK_END);
  me->size = ftell(me->file);
  fseek(me->file, 0L, SEEK_SET);

  me->current_position = 0;

  return E_OGGPLAY_OK;
}

OggPlayErrorCode
oggplay_file_reader_destroy(OggPlayReader * opr) {

  OggPlayFileReader * me;

  me = (OggPlayFileReader *)opr;

  fclose(me->file);
  oggplay_free(me);

  return E_OGGPLAY_OK;
}

int
oggplay_file_reader_available(OggPlayReader * opr, ogg_int64_t current_bytes,
    ogg_int64_t current_time) {

  OggPlayFileReader *me = (OggPlayFileReader *)opr;
  return me->size;

}

int
oggplay_file_reader_finished_retrieving(OggPlayReader *opr) {

  return 1;

}


static size_t
oggplay_file_reader_io_read(void * user_handle, void * buf, size_t n) {

  OggPlayFileReader *me = (OggPlayFileReader *)user_handle;
  int r;
  r = fread(buf, 1, n, me->file);
  if (r > 0) {
    me->current_position += r;
  }

  return r;
}

static int
oggplay_file_reader_io_seek(void * user_handle, long offset, int whence) {

  OggPlayFileReader * me = (OggPlayFileReader *)user_handle;
  int                 r;

  r = fseek(me->file, offset, whence);
  me->current_position = ftell(me->file);
  return r;

}

static long
oggplay_file_reader_io_tell(void * user_handle) {

  OggPlayFileReader * me = (OggPlayFileReader *)user_handle;

  return ftell(me->file);

}

OggPlayReader *
oggplay_file_reader_new(char *file_name) {

  OggPlayFileReader * me = oggplay_malloc (sizeof (OggPlayFileReader));

  if (me == NULL)
    return NULL;

  me->current_position = 0;
  me->file_name = file_name;
  me->file = NULL;

  me->functions.initialise = &oggplay_file_reader_initialise;
  me->functions.destroy = &oggplay_file_reader_destroy;
  me->functions.available = &oggplay_file_reader_available;
  me->functions.finished_retrieving = &oggplay_file_reader_finished_retrieving;
  me->functions.seek = NULL;
  me->functions.duration = NULL;
  me->functions.io_read = &oggplay_file_reader_io_read;
  me->functions.io_seek = &oggplay_file_reader_io_seek;
  me->functions.io_tell = &oggplay_file_reader_io_tell;
  me->functions.duration = NULL;

  return (OggPlayReader *)me;

}
