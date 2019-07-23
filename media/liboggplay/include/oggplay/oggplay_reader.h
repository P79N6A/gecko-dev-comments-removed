






































#ifndef __OGGPLAY_READER_H__
#define __OGGPLAY_READER_H__

#include <stdlib.h>
#include <oggz/oggz.h>
#include <ogg/ogg.h>

struct _OggPlayReader;

typedef struct _OggPlayReader {
  OggPlayErrorCode  (*initialise)(struct _OggPlayReader * me, int block);
  OggPlayErrorCode  (*destroy)(struct _OggPlayReader * me);
  OggPlayErrorCode  (*seek)(struct _OggPlayReader *me, OGGZ *oggz, 
                                                    ogg_int64_t milliseconds);
  int               (*available)(struct _OggPlayReader *me,
                                              ogg_int64_t current_bytes,
                                              ogg_int64_t current_time);
  ogg_int64_t       (*duration)(struct _OggPlayReader *me);
  int               (*finished_retrieving)(struct _OggPlayReader *me);

  
  size_t            (*io_read)(void *user_handle, void *buf, size_t n);
  int               (*io_seek)(void *user_handle, long offset, int whence);
  long              (*io_tell)(void *user_handle);
} OggPlayReader;








OggPlayReader *
oggplay_file_reader_new(char *filename);










OggPlayReader *
oggplay_tcp_reader_new(char *uri, char *proxy, int proxy_port);

#endif
