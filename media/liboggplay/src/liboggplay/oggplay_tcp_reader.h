






































#ifndef __OGGPLAY_FILE_READER_H__
#define __OGGPLAY_FILE_READER_H__

#include <stdio.h>




#define TCP_READER_MAX_IN_MEMORY            128*1024
#define TCP_READER_WRITE_THRESHOLD           64*1024

typedef enum {
  TCP_READER_FROM_MEMORY,
  TCP_READER_FROM_FILE
} dataLocation;

typedef enum {
  OTRS_UNINITIALISED,
  OTRS_SOCKET_CREATED,
  OTRS_CONNECTED,
  OTRS_SENT_HEADER,
  OTRS_HTTP_RESPONDED,
  OTRS_INIT_COMPLETE
} OPTCPReaderState;

typedef struct {
  OggPlayReader     functions;
  OPTCPReaderState  state;
#ifdef _WIN32
  SOCKET            socket;
#else
  int               socket;
#endif
  unsigned char   * buffer;
  int               buffer_size;
  int               current_position;
  char            * location;
  char            * proxy;
  int               proxy_port;
  int               amount_in_memory;
  FILE            * backing_store;
  int               stored_offset;
  dataLocation      mode;
  int               duration;
} OggPlayTCPReader;

#endif
