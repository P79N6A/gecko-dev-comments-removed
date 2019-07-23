































#ifndef __OGGZ_STREAM_PRIVATE_H__
#define __OGGZ_STREAM_PRIVATE_H__

typedef struct _oggz_stream_t oggz_stream_t;

typedef int (*OggzReadBOS) (OGGZ * oggz, long serialno,
                            unsigned char * data, long length,
			    void * user_data);

typedef struct {
  const char      *bos_str;
  int             bos_str_len;
  const char      *content_type;
  OggzReadBOS     reader;
  ogg_int64_t     (*calculator)(ogg_int64_t now, oggz_stream_t *stream, 
                  ogg_packet *op);
  ogg_int64_t     (*r_calculator)(ogg_int64_t next_packet_gp, 
                  oggz_stream_t *stream, ogg_packet *this_packet, 
                  ogg_packet *next_packet);
} oggz_auto_contenttype_t;

extern const oggz_auto_contenttype_t oggz_auto_codec_ident[];


oggz_stream_t * oggz_get_stream (OGGZ * oggz, long serialno);
oggz_stream_t * oggz_add_stream (OGGZ * oggz, long serialno);

int oggz_stream_has_metric (OGGZ * oggz, long serialno);
int oggz_stream_set_content (OGGZ * oggz, long serialno, int content);

ogg_int64_t 
oggz_auto_calculate_granulepos(int content, ogg_int64_t now, 
                oggz_stream_t *stream, ogg_packet *op);

ogg_int64_t
oggz_auto_calculate_gp_backwards(int content, ogg_int64_t next_packet_gp,
      oggz_stream_t *stream, ogg_packet *this_packet, ogg_packet *next_packet);

#endif 
