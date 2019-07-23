






































#ifndef __OGGPLAY_FILE_READER_H__
#define __OGGPLAY_FILE_READER_H__

#include <stdio.h>

#define FILE_READER_CHUNK_SIZE              8192
#define FILE_READER_INITIAL_NUM_BUFFERS     8

typedef struct {
  OggPlayReader     functions;
  char            * file_name;
  FILE            * file;
  int               current_position;
  int               size;
} OggPlayFileReader;

#endif
