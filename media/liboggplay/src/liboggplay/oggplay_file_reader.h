






































#ifndef __OGGPLAY_FILE_READER_H__
#define __OGGPLAY_FILE_READER_H__

#include <stdio.h>

#define FILE_READER_CHUNK_SIZE              8192
#define FILE_READER_INITIAL_NUM_BUFFERS     8

typedef struct {
  OggPlayReader     functions;
  const char      * file_name;
  FILE            * file;
  long              current_position;
  long              size;
} OggPlayFileReader;

#endif
