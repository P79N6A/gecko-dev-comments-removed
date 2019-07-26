



































#ifndef _nr_common_h
#define _nr_common_h

#include <csi_platform.h>

#ifdef USE_MPATROL
#define USEDEBUG 1
#include <mpatrol.h>
#endif

#ifdef USE_DMALLOC
#include <dmalloc.h>
#endif

#include <string.h>
#include <time.h>

#ifdef WIN32
#include <winsock2.h>
#include <errno.h>
#else
#include <sys/errno.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <sys/queue.h>
#include <r_log.h>

extern int NR_LOG_REASSD;

#include "registry.h"
#include "nrstats.h"

typedef struct nr_captured_packet_ {
     UCHAR cap_interface;       
     struct timeval ts;     
     UINT4 len;             
     UINT8 packet_number;   
} nr_captured_packet;

#ifndef NR_ROOT_PATH
#define NR_ROOT_PATH "/usr/local/ctc/"
#endif

#define NR_ARCHIVE_DIR NR_ROOT_PATH  "archive/"
#define NR_TEMP_DIR NR_ROOT_PATH  "tmp/"
#define NR_ARCHIVE_STATEFILE NR_ROOT_PATH  "archive/state"
#define NR_CAPTURED_PID_FILENAME  NR_ROOT_PATH "captured.pid"
#define NR_REASSD_PID_FILENAME  NR_ROOT_PATH "reassd.pid"
#define NR_MODE_FILENAME  NR_ROOT_PATH "mode.txt"

char *nr_revision_number(void);





#define NR_MEM_TCP       1
#define NR_MEM_HTTP      2
#define NR_MEM_DELIVERY  3
#define NR_MEM_OUT_HM    4
#define NR_MEM_OUT_SSL   5
#define NR_MEM_SSL       7
#define NR_MEM_COMMON    8
#define NR_MEM_CODEC     9

#endif

