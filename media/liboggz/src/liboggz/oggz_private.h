































#ifndef __OGGZ_PRIVATE_H__
#define __OGGZ_PRIVATE_H__

#include <stdio.h>
#include <sys/types.h>

#include <ogg/ogg.h>
#include <oggz/oggz_constants.h>
#include <oggz/oggz_off_t.h>

#include "oggz_macros.h"
#include "oggz_vector.h"
#include "oggz_dlist.h"

#define OGGZ_AUTO_MULT 1000Ull

typedef struct _OGGZ OGGZ;
typedef struct _OggzComment OggzComment;
typedef struct _OggzIO OggzIO;
typedef struct _OggzReader OggzReader;
typedef struct _OggzWriter OggzWriter;


typedef int (*OggzReadPacket) (OGGZ * oggz, ogg_packet * op, long serialno,
			       void * user_data);
typedef int (*OggzReadPage) (OGGZ * oggz, const ogg_page * og, long serialno,
			     void * user_data);


#include "oggz_stream_private.h"

typedef ogg_int64_t (*OggzMetric) (OGGZ * oggz, long serialno,
				   ogg_int64_t granulepos,
				   void * user_data);

typedef int (*OggzOrder) (OGGZ * oggz, ogg_packet * op, void * target,
			  void * user_data);

typedef int (*OggzWriteHungry) (OGGZ * oggz, int empty, void * user_data);


typedef size_t (*OggzIORead) (void * user_handle, void * buf, size_t n);
typedef size_t (*OggzIOWrite) (void * user_handle, void * buf, size_t n);
typedef int (*OggzIOSeek) (void * user_handle, long offset, int whence);
typedef long (*OggzIOTell) (void * user_handle);
typedef int (*OggzIOFlush) (void * user_handle);

struct _oggz_stream_t {
  ogg_stream_state ogg_stream;

  
  int content;
  int numheaders;
  int preroll;
  ogg_int64_t granulerate_n;
  ogg_int64_t granulerate_d;
  ogg_int64_t first_granule;
  ogg_int64_t basegranule;
  int granuleshift;

  
  char * vendor;
  OggzVector * comments;

  
  
  int delivered_non_b_o_s;

  int b_o_s; 
  int e_o_s; 
  ogg_int64_t granulepos;
  ogg_int64_t packetno;

  
  OggzMetric metric;
  void * metric_user_data;
  int metric_internal;

  OggzOrder order;
  void * order_user_data;

  OggzReadPacket read_packet;
  void * read_user_data;

  OggzReadPage read_page;
  void * read_page_user_data;

  
  ogg_int64_t last_granulepos;
  ogg_int64_t page_granulepos;
  void * calculate_data;
  ogg_packet  * last_packet;
};

struct _OggzReader {
  ogg_sync_state ogg_sync;

  
  ogg_stream_state ogg_stream;
  long current_serialno;

  OggzReadPacket read_packet;
  void * read_user_data;

  OggzReadPage read_page;
  void * read_page_user_data;

  ogg_int64_t current_unit;
  ogg_int64_t current_granulepos;

  long current_page_bytes;
#if 0
  oggz_off_t offset_page_end; 
#endif
};





typedef struct {
  ogg_packet op;
  oggz_stream_t * stream;
  int flush;
  int * guard;
} oggz_writer_packet_t;

enum oggz_writer_state {
  OGGZ_MAKING_PACKETS = 0,
  OGGZ_WRITING_PAGES = 1
};

struct _OggzWriter {
  oggz_writer_packet_t * next_zpacket; 
  OggzVector * packet_queue;

  OggzWriteHungry hungry;
  void * hungry_user_data;
  int hungry_only_when_empty;

  int writing; 
  int state; 

  int flushing; 

#if 0
  int eog; 
  int eop; 
#endif
  int eos; 

  oggz_writer_packet_t * current_zpacket;

  int packet_offset; 
  int page_offset; 

  ogg_stream_state * current_stream;

  int no_more_packets; 


};

struct _OggzIO {
  OggzIORead read;
  void * read_user_handle;

  OggzIOWrite write;
  void * write_user_handle;

  OggzIOSeek seek;
  void * seek_user_handle;

  OggzIOTell tell;
  void * tell_user_handle;

  OggzIOFlush flush;
  void * flush_user_handle;
};

struct _OggzComment {
  
  char * name;

  
  char * value;
};

struct _OGGZ {
  int flags;
  FILE * file;
  OggzIO * io;

  ogg_packet current_packet;
  ogg_page current_page;

  oggz_off_t offset; 
  oggz_off_t offset_data_begin; 

  long run_blocksize; 
  int cb_next;

  OggzVector * streams;
  int all_at_eos; 

  OggzMetric metric;
  void * metric_user_data;
  int metric_internal;

  OggzOrder order;
  void * order_user_data;

  union {
    OggzReader reader;
    OggzWriter writer;
  } x;

  OggzDList * packet_buffer;
};

OGGZ * oggz_read_init (OGGZ * oggz);
OGGZ * oggz_read_close (OGGZ * oggz);

OGGZ * oggz_write_init (OGGZ * oggz);
int oggz_write_flush (OGGZ * oggz);
OGGZ * oggz_write_close (OGGZ * oggz);

int oggz_map_return_value_to_error (int cb_ret);

int oggz_get_bos (OGGZ * oggz, long serialno);
ogg_int64_t oggz_get_unit (OGGZ * oggz, long serialno, ogg_int64_t granulepos);

int oggz_set_metric_internal (OGGZ * oggz, long serialno, OggzMetric metric,
			      void * user_data, int internal);
int oggz_has_metrics (OGGZ * oggz);

int oggz_purge (OGGZ * oggz);



int
oggz_set_granulerate (OGGZ * oggz, long serialno, 
                                    ogg_int64_t granule_rate_numerator,
                                    ogg_int64_t granule_rate_denominator);

int
oggz_get_granulerate (OGGZ * oggz, long serialno,
                                    ogg_int64_t * granulerate_n,
                                    ogg_int64_t * granulerate_d);

int oggz_set_granuleshift (OGGZ * oggz, long serialno, int granuleshift);
int oggz_get_granuleshift (OGGZ * oggz, long serialno);

int oggz_set_first_granule (OGGZ * oggz, long serialno, ogg_int64_t first_granule);

int oggz_set_preroll (OGGZ * oggz, long serialno, int preroll);
int oggz_get_preroll (OGGZ * oggz, long serialno);


 
int
oggz_auto_read_bos_page (OGGZ * oggz, ogg_page * og, long serialno,
                         void * user_data);
int
oggz_auto_read_bos_packet (OGGZ * oggz, ogg_packet * op, long serialno, 
                           void * user_data);

int
oggz_auto_read_comments (OGGZ * oggz, oggz_stream_t * stream, long serialno,
                         ogg_packet * op);

int oggz_auto_identify_page (OGGZ *oggz, ogg_page *og, long serialno);
int oggz_auto_identify_packet (OGGZ * oggz, ogg_packet * op, long serialno);


int oggz_comments_init (oggz_stream_t * stream);
int oggz_comments_free (oggz_stream_t * stream);
int oggz_comments_decode (OGGZ * oggz, long serialno,
                          unsigned char * comments, long length);
long oggz_comments_encode (OGGZ * oggz, long serialno,
                           unsigned char * buf, long length);


size_t oggz_io_read (OGGZ * oggz, void * buf, size_t n);
size_t oggz_io_write (OGGZ * oggz, void * buf, size_t n);
int oggz_io_seek (OGGZ * oggz, long offset, int whence);
long oggz_io_tell (OGGZ * oggz);
int oggz_io_flush (OGGZ * oggz);

#endif 
