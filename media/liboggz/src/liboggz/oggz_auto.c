





































#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "oggz_private.h"
#include "oggz_byteorder.h"
#include "dirac.h"

#include "oggz/oggz_stream.h"





int oggz_set_metric_internal (OGGZ * oggz, long serialno, OggzMetric metric,
			      void * user_data, int internal);

int oggz_set_metric_linear (OGGZ * oggz, long serialno,
			    ogg_int64_t granule_rate_numerator,
			    ogg_int64_t granule_rate_denominator);

static int
oggz_stream_set_numheaders (OGGZ * oggz, long serialno, int numheaders)
{
  oggz_stream_t * stream;

  if (oggz == NULL) return OGGZ_ERR_BAD_OGGZ;

  stream = oggz_get_stream (oggz, serialno);
  if (stream == NULL) return OGGZ_ERR_BAD_SERIALNO;

  stream->numheaders = numheaders;

  return 0;
}

static int
auto_speex (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  unsigned char * header = data;
  ogg_int64_t granule_rate = 0;
  int numheaders;

  if (length < 68) return 0;

  granule_rate = (ogg_int64_t) int32_le_at(&header[36]);
#ifdef DEBUG
  printf ("Got speex rate %d\n", (int)granule_rate);
#endif

  oggz_set_granulerate (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  oggz_set_preroll (oggz, serialno, 3);

  numheaders = (ogg_int64_t) int32_le_at(&header[68]) + 2;
  oggz_stream_set_numheaders (oggz, serialno, numheaders);

  return 1;
}

static int
auto_vorbis (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  unsigned char * header = data;
  ogg_int64_t granule_rate = 0;

  if (length < 30) return 0;

  granule_rate = (ogg_int64_t) int32_le_at(&header[12]);
#ifdef DEBUG
  printf ("Got vorbis rate %d\n", (int)granule_rate);
#endif

  oggz_set_granulerate (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  oggz_set_preroll (oggz, serialno, 2);

  oggz_stream_set_numheaders (oggz, serialno, 3);

  return 1;
}

#if USE_THEORA_PRE_ALPHA_3_FORMAT
static int intlog(int num) {
  int ret=0;
  while(num>0){
    num=num/2;
    ret=ret+1;
  }
  return(ret);
}
#endif

#define THEORA_VERSION(maj,min,rev) ((maj<<16)+(min<<8)+rev)

static int
auto_theora (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  unsigned char * header = data;
  int version;
  ogg_int32_t fps_numerator, fps_denominator;
  char keyframe_granule_shift = 0;
  int keyframe_shift;

  
  if (length < 41) return 0;

  version = THEORA_VERSION(header[7], header[8], header[9]);

  fps_numerator = int32_be_at(&header[22]);
  fps_denominator = int32_be_at(&header[26]);

  



  if (fps_numerator == 0) fps_numerator = 1;

#if USE_THEORA_PRE_ALPHA_3_FORMAT
  
  keyframe_granule_shift = (header[36] & 0xf8) >> 3;
  keyframe_shift = intlog (keyframe_granule_shift - 1);
#else
  keyframe_granule_shift = (char) ((header[40] & 0x03) << 3);
  keyframe_granule_shift |= (header[41] & 0xe0) >> 5; 
  keyframe_shift = keyframe_granule_shift;
#endif

#ifdef DEBUG
  printf ("Got theora fps %d/%d, keyframe_shift %d\n",
	  fps_numerator, fps_denominator, keyframe_shift);
#endif

  oggz_set_granulerate (oggz, serialno, (ogg_int64_t)fps_numerator,
			OGGZ_AUTO_MULT * (ogg_int64_t)fps_denominator);
  oggz_set_granuleshift (oggz, serialno, keyframe_shift);

  if (version > THEORA_VERSION(3,2,0))
    oggz_set_first_granule (oggz, serialno, 1);

  oggz_stream_set_numheaders (oggz, serialno, 3);

  return 1;
}

static int
auto_annodex (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  
  oggz_set_granulerate (oggz, serialno, 0, 1);

  return 1;
}

static int
auto_anxdata (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  unsigned char * header = data;
  ogg_int64_t granule_rate_numerator = 0, granule_rate_denominator = 0;

  if (length < 28) return 0;

  granule_rate_numerator = int64_le_at(&header[8]);
  granule_rate_denominator = int64_le_at(&header[16]);
#ifdef DEBUG
  printf ("Got AnxData rate %lld/%lld\n", granule_rate_numerator,
	  granule_rate_denominator);
#endif

  oggz_set_granulerate (oggz, serialno,
			granule_rate_numerator,
			OGGZ_AUTO_MULT * granule_rate_denominator);

  return 1;
}

static int
auto_flac0 (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  unsigned char * header = data;
  ogg_int64_t granule_rate = 0;

  granule_rate = (ogg_int64_t) (header[14] << 12) | (header[15] << 4) |
            ((header[16] >> 4)&0xf);
#ifdef DEBUG
    printf ("Got flac rate %d\n", (int)granule_rate);
#endif

  oggz_set_granulerate (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  oggz_stream_set_numheaders (oggz, serialno, 3);

  return 1;
}

static int
auto_flac (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  unsigned char * header = data;
  ogg_int64_t granule_rate = 0;
  int numheaders;

  if (length < 51) return 0;

  granule_rate = (ogg_int64_t) (header[27] << 12) | (header[28] << 4) |
            ((header[29] >> 4)&0xf);
#ifdef DEBUG
  printf ("Got flac rate %d\n", (int)granule_rate);
#endif

  oggz_set_granulerate (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  numheaders = int16_be_at(&header[7]);
  oggz_stream_set_numheaders (oggz, serialno, numheaders);

  return 1;
}





static int
auto_oggpcm2 (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  unsigned char * header = data;
  ogg_int64_t granule_rate;

  if (length < 28) return 0;

  granule_rate = (ogg_int64_t) int32_be_at(&header[16]);
#ifdef DEBUG
  printf ("Got OggPCM2 rate %d\n", (int)granule_rate);
#endif

  oggz_set_granulerate (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  oggz_stream_set_numheaders (oggz, serialno, 3);

  return 1;
}

static int
auto_celt (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  unsigned char * header = data;
  ogg_int64_t granule_rate = 0;
  int numheaders;

  if (length < 56) return 0;

  granule_rate = (ogg_int64_t) int32_le_at(&header[40]);
#ifdef DEBUG
  printf ("Got celt sample rate %d\n", (int)granule_rate);
#endif

  oggz_set_granulerate (oggz, serialno, granule_rate, OGGZ_AUTO_MULT);

  numheaders = (ogg_int64_t) int32_le_at(&header[52]) + 2;
  oggz_stream_set_numheaders (oggz, serialno, numheaders);

  return 1;
}

static int
auto_cmml (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  unsigned char * header = data;
  ogg_int64_t granule_rate_numerator = 0, granule_rate_denominator = 0;
  int granuleshift;

  if (length < 28) return 0;

  granule_rate_numerator = int64_le_at(&header[12]);
  granule_rate_denominator = int64_le_at(&header[20]);
  if (length > 28)
    granuleshift = (int)header[28];
  else
    granuleshift = 0;

#ifdef DEBUG
  printf ("Got CMML rate %lld/%lld\n", granule_rate_numerator,
	  granule_rate_denominator);
#endif

  oggz_set_granulerate (oggz, serialno,
			granule_rate_numerator,
			OGGZ_AUTO_MULT * granule_rate_denominator);
  oggz_set_granuleshift (oggz, serialno, granuleshift);

  oggz_stream_set_numheaders (oggz, serialno, 3);

  return 1;
}

static int
auto_kate (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  unsigned char * header = data;
  ogg_int32_t gps_numerator, gps_denominator;
  unsigned char granule_shift = 0;
  int numheaders;

  if (length < 64) return 0;

  gps_numerator = int32_le_at(&header[24]);
  gps_denominator = int32_le_at(&header[28]);

  granule_shift = (header[15]);
  numheaders = (header[11]);

#ifdef DEBUG
  printf ("Got kate gps %d/%d, granule shift %d\n",
	  gps_numerator, gps_denominator, granule_shift);
#endif

  oggz_set_granulerate (oggz, serialno, gps_numerator,
			OGGZ_AUTO_MULT * gps_denominator);
  oggz_set_granuleshift (oggz, serialno, granule_shift);

  oggz_stream_set_numheaders (oggz, serialno, numheaders);

  return 1;
}

static int
auto_dirac (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  int granule_shift = 22; 
  dirac_info *info;

  info = oggz_malloc(sizeof(dirac_info));
  if (info == NULL) return -1;

  dirac_parse_info(info, data, length);

  
  oggz_set_granulerate (oggz, serialno,
	2 * (ogg_int64_t)info->fps_numerator,
	OGGZ_AUTO_MULT * (ogg_int64_t)info->fps_denominator);
  oggz_set_granuleshift (oggz, serialno, granule_shift);

  oggz_stream_set_numheaders (oggz, serialno, 0);

  oggz_free(info);
  return 1;
}

static int
auto_fisbone (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  unsigned char * header = data;
  long fisbone_serialno; 
  ogg_int64_t granule_rate_numerator = 0, granule_rate_denominator = 0;
  int granuleshift, numheaders;

  if (length < 48) return 0;

  fisbone_serialno = (long) int32_le_at(&header[12]);

  
  if (oggz_stream_has_metric (oggz, fisbone_serialno)) return 1;

  granule_rate_numerator = int64_le_at(&header[20]);
  granule_rate_denominator = int64_le_at(&header[28]);
  granuleshift = (int)header[48];

#ifdef DEBUG
  printf ("Got fisbone granulerate %lld/%lld, granuleshift %d for serialno %010lu\n",
	  granule_rate_numerator, granule_rate_denominator, granuleshift,
	  fisbone_serialno);
#endif

  oggz_set_granulerate (oggz, fisbone_serialno,
			granule_rate_numerator,
			OGGZ_AUTO_MULT * granule_rate_denominator);
  oggz_set_granuleshift (oggz, fisbone_serialno, granuleshift);

  
  numheaders = oggz_stream_get_numheaders (oggz, serialno);
  oggz_stream_set_numheaders (oggz, serialno, numheaders+1);

  return 1;
}

static int
auto_fishead (OGGZ * oggz, long serialno, unsigned char * data, long length, void * user_data)
{
  oggz_set_granulerate (oggz, serialno, 0, 1);

  
  oggz_stream_set_numheaders (oggz, serialno, 1);

  return 1;
}





typedef struct {
  int headers_encountered;
  int packet_size;
  int encountered_first_data_packet;
} auto_calc_speex_info_t;

static ogg_int64_t
auto_calc_speex(ogg_int64_t now, oggz_stream_t *stream, ogg_packet *op) {

  




  auto_calc_speex_info_t *info
          = (auto_calc_speex_info_t *)stream->calculate_data;

  if (stream->calculate_data == NULL) {
    stream->calculate_data = oggz_malloc(sizeof(auto_calc_speex_info_t));
    if (stream->calculate_data == NULL) return -1;
    info = stream->calculate_data;
    info->encountered_first_data_packet = 0;
    info->packet_size =
            (*(int *)(op->packet + 64)) * (*(int *)(op->packet + 56));
    info->headers_encountered = 1;
    return 0;
  }

  if (info->headers_encountered < 2) {
    info->headers_encountered += 1;
  } else {
    info->encountered_first_data_packet = 1;
  }

  if (now > -1) {
    return now;
  }

  if (info->encountered_first_data_packet) {
    if (stream->last_granulepos > 0) {
      return stream->last_granulepos + info->packet_size;
    }

    return -1;
  }

  return 0;

}





typedef struct {
  int headers_encountered;
  int packet_size;
  int encountered_first_data_packet;
} auto_calc_celt_info_t;

static ogg_int64_t
auto_calc_celt (ogg_int64_t now, oggz_stream_t *stream, ogg_packet *op) {

  




  auto_calc_celt_info_t *info
          = (auto_calc_celt_info_t *)stream->calculate_data;

  if (stream->calculate_data == NULL) {
    stream->calculate_data = oggz_malloc(sizeof(auto_calc_celt_info_t));
    if (stream->calculate_data == NULL) return -1;

    info = stream->calculate_data;
    info->encountered_first_data_packet = 0;

    



    info->packet_size = 256;

    info->headers_encountered = 1;
    return 0;
  }

  if (info->headers_encountered < 2) {
    info->headers_encountered += 1;
  } else {
    info->encountered_first_data_packet = 1;
  }

  if (now > -1) {
    return now;
  }

  if (info->encountered_first_data_packet) {
    if (stream->last_granulepos > 0) {
      return stream->last_granulepos + info->packet_size;
    }

    return -1;
  }

  return 0;

}








typedef struct {
  int encountered_first_data_packet;
} auto_calc_theora_info_t;


static ogg_int64_t
auto_calc_theora(ogg_int64_t now, oggz_stream_t *stream, ogg_packet *op) {

  long keyframe_no;
  int keyframe_shift;
  unsigned char first_byte;
  auto_calc_theora_info_t *info;

  first_byte = op->packet[0];

  info = (auto_calc_theora_info_t *)stream->calculate_data;

  
  if (first_byte & 0x80)
  {
    if (info == NULL) {
      stream->calculate_data = oggz_malloc(sizeof(auto_calc_theora_info_t));
      if (stream->calculate_data == NULL) return -1;
      info = stream->calculate_data;
    }
    info->encountered_first_data_packet = 0;
    return (ogg_int64_t)0;
  }

  
  if (now > (ogg_int64_t)(-1)) {
    info->encountered_first_data_packet = 1;
    return now;
  }

  
  if (stream->last_granulepos == -1) {
    info->encountered_first_data_packet = 1;
    return (ogg_int64_t)-1;
  }

  


  if (!info->encountered_first_data_packet) {
    info->encountered_first_data_packet = 1;
    return (ogg_int64_t)-1;
  }

  
  if (first_byte & 0x40)
  {
    return stream->last_granulepos + 1;
  }

  keyframe_shift = stream->granuleshift;
  


  keyframe_no = (int)(stream->last_granulepos >> keyframe_shift);
  


  keyframe_no += (stream->last_granulepos & ((1 << keyframe_shift) - 1)) + 1;
  return ((ogg_int64_t)keyframe_no) << keyframe_shift;


}

static ogg_int64_t
auto_rcalc_theora(ogg_int64_t next_packet_gp, oggz_stream_t *stream,
                  ogg_packet *this_packet, ogg_packet *next_packet) {

  int keyframe = (int)(next_packet_gp >> stream->granuleshift);
  int offset = (int)(next_packet_gp - (keyframe << stream->granuleshift));

  



  if (offset == 0) {
    return ((keyframe - 60L) << stream->granuleshift) + 59;
  }
  else {
    return (((ogg_int64_t)keyframe) << stream->granuleshift) + (offset - 1);
  }

}





























typedef struct {
  int nln_increments[4];
  int nsn_increment;
  int short_size;
  int long_size;
  int encountered_first_data_packet;
  int last_was_long;
  int log2_num_modes;
  int mode_sizes[1];
} auto_calc_vorbis_info_t;


static ogg_int64_t
auto_calc_vorbis(ogg_int64_t now, oggz_stream_t *stream, ogg_packet *op) {

  auto_calc_vorbis_info_t *info;
  int ii;

  if (stream->calculate_data == NULL) {
    



    int short_size;
    int long_size;

    long_size = 1 << (op->packet[28] >> 4);
    short_size = 1 << (op->packet[28] & 0xF);

    stream->calculate_data = oggz_malloc(sizeof(auto_calc_vorbis_info_t));
    if (stream->calculate_data == NULL) return -1;

    info = (auto_calc_vorbis_info_t *)stream->calculate_data;
    info->nln_increments[3] = long_size >> 1;
    info->nln_increments[2] = 3 * (long_size >> 2) - (short_size >> 2);
    info->nln_increments[1] = (long_size >> 2) + (short_size >> 2);
    info->nln_increments[0] = info->nln_increments[3];
    info->short_size = short_size;
    info->long_size = long_size;
    info->nsn_increment = short_size >> 1;
    info->encountered_first_data_packet = 0;

    
    return 0;
  }

  


  if (op->packet[0] & 0x1) {
    













    if (op->packet[0] == 5) {
      unsigned char *current_pos = &op->packet[op->bytes - 1];
      int offset;
      int size;
      int size_check;
      int *mode_size_ptr;
      int i;
      size_t size_realloc_bytes;

      



































      size = 0;

      offset = 8;
      while (!((1 << --offset) & *current_pos)) {
        if (offset == 0) {
          offset = 8;
          current_pos -= 1;
        }
      }

      while (1)
      {

        



        offset = (offset + 7) % 8;
        if (offset == 7)
          current_pos -= 1;

        if
        (
          ((current_pos[-5] & ~((1 << (offset + 1)) - 1)) != 0)
          ||
          current_pos[-4] != 0
          ||
          current_pos[-3] != 0
          ||
          current_pos[-2] != 0
          ||
          ((current_pos[-1] & ((1 << (offset + 1)) - 1)) != 0)
        )
        {
          break;
        }

        size += 1;

        current_pos -= 5;

      }

      

      for (ii=0; ii < 2; ii++) {
       if (offset > 4) {
         size_check = (current_pos[0] >> (offset - 5)) & 0x3F;
       } else {
         
         size_check = (current_pos[0] & ((1 << (offset + 1)) - 1));
         
         size_check <<= (5 - offset);
         
         size_check |= (current_pos[-1] & ~((1 << (offset + 3)) - 1)) >>
           (offset + 3);
       }

       size_check += 1;
       if (size_check == size) {
         break;
       }
        offset = (offset + 1) % 8;
        if (offset == 0)
          current_pos += 1;
       current_pos += 5;
       size -= 1;
      }

#ifdef DEBUG
      if (size_check != size)
      {
        printf("WARNING: size parsing failed for VORBIS mode packets\n");
      }
#endif

      
      size_realloc_bytes = sizeof(auto_calc_vorbis_info_t) + (size - 1) * sizeof(int);
      if (size_realloc_bytes < sizeof (auto_calc_vorbis_info_t)) return -1;

      
      info = realloc(stream->calculate_data, size_realloc_bytes);
      if (info == NULL) return -1;

      stream->calculate_data = info;

      i = -1;
      while ((1 << (++i)) < size);
      info->log2_num_modes = i;

      mode_size_ptr = info->mode_sizes;

      for(i = 0; i < size; i++)
      {
        offset = (offset + 1) % 8;
        if (offset == 0)
          current_pos += 1;
        *mode_size_ptr++ = (current_pos[0] >> offset) & 0x1;
        current_pos += 5;
      }

    }

    return 0;
  }

  info = (auto_calc_vorbis_info_t *)stream->calculate_data;

  return -1;

  {
    



    int mode;
    int size;
    ogg_int64_t result;

    mode = (op->packet[0] >> 1) & ((1 << info->log2_num_modes) - 1);
    size = info->mode_sizes[mode];

    



    if (now > -1 && stream->last_granulepos == -1) {
      info->encountered_first_data_packet = 1;
      info->last_was_long = size;
      return now;
    }

    if (info->encountered_first_data_packet == 0) {
      info->encountered_first_data_packet = 1;
      info->last_was_long = size;
      return -1;
    }

    



    if (stream->last_granulepos == -1) {
      info->last_was_long = size;
      return -1;
    }

    result = stream->last_granulepos +
      (
        (info->last_was_long ? info->long_size  : info->short_size)
        +
        (size ? info->long_size : info->short_size)
      ) / 4;
    info->last_was_long = size;

    return result;

  }

}

ogg_int64_t
auto_rcalc_vorbis(ogg_int64_t next_packet_gp, oggz_stream_t *stream,
                  ogg_packet *this_packet, ogg_packet *next_packet) {

  auto_calc_vorbis_info_t *info =
                  (auto_calc_vorbis_info_t *)stream->calculate_data;

  int mode =
      (this_packet->packet[0] >> 1) & ((1 << info->log2_num_modes) - 1);
  int this_size = info->mode_sizes[mode] ? info->long_size : info->short_size;
  int next_size;
  ogg_int64_t r;

  mode = (next_packet->packet[0] >> 1) & ((1 << info->log2_num_modes) - 1);
  next_size = info->mode_sizes[mode] ? info->long_size : info->short_size;

  r = next_packet_gp - ((this_size + next_size) / 4);
  if (r < 0) return 0L;
  return r;

}
















typedef struct {
  ogg_int64_t previous_gp;
  int encountered_first_data_packet;
} auto_calc_flac_info_t;

static ogg_int64_t
auto_calc_flac (ogg_int64_t now, oggz_stream_t *stream, ogg_packet *op)
{
  auto_calc_flac_info_t *info;

  if (stream->calculate_data == NULL) {
    stream->calculate_data = oggz_malloc(sizeof(auto_calc_flac_info_t));
    if (stream->calculate_data == NULL) return -1;

    info = (auto_calc_flac_info_t *)stream->calculate_data;
    info->previous_gp = 0;
    info->encountered_first_data_packet = 0;

    
    goto out;
  }

  info = (auto_calc_flac_info_t *)stream->calculate_data;

  
  if (op->packet[0] == 0xff)
      info->encountered_first_data_packet = 1;

  if (now == -1 && op->packet[0] == 0xff && op->bytes > 2) {
    unsigned char bs;
    int block_size;

    bs = (op->packet[2] & 0xf0) >> 4;

    switch (bs) {
      case 0x0: 
        block_size = -1;
        break;
      case 0x1: 
        block_size = 192;
        break;
      
      case 0x2:
        block_size = 576;
        break;
      case 0x3:
        block_size = 1152;
        break;
      case 0x4:
        block_size = 2304;
        break;
      case 0x5:
        block_size = 4608;
        break;
      case 0x6: 
        block_size = -1;
        break;
      case 0x7: 
        block_size = -1;
        break;
      
      case 0x8:
        block_size = 256;
        break;
      case 0x9:
        block_size = 512;
        break;
      case 0xa:
        block_size = 1024;
        break;
      case 0xb:
        block_size = 2048;
        break;
      case 0xc:
        block_size = 4096;
        break;
      case 0xd:
        block_size = 8192;
        break;
      case 0xe:
        block_size = 16384;
        break;
      case 0xf:
        block_size = 32768;
        break;
      default:
        block_size = -1;
        break;
    }

    if (block_size != -1) {
      now = info->previous_gp + block_size;
    }
  } else if (now == -1 && info->encountered_first_data_packet == 0) {
    
    now = 0;
  }

out:
  info->previous_gp = now;
  return now;
}

const oggz_auto_contenttype_t oggz_auto_codec_ident[] = {
  {"\200theora", 7, "Theora", auto_theora, auto_calc_theora, auto_rcalc_theora},
  {"\001vorbis", 7, "Vorbis", auto_vorbis, auto_calc_vorbis, auto_rcalc_vorbis},
  {"Speex", 5, "Speex", auto_speex, auto_calc_speex, NULL},
  {"PCM     ", 8, "PCM", auto_oggpcm2, NULL, NULL},
  {"CMML\0\0\0\0", 8, "CMML", auto_cmml, NULL, NULL},
  {"Annodex", 8, "Annodex", auto_annodex, NULL, NULL},
  {"fishead", 7, "Skeleton", auto_fishead, NULL, NULL},
  {"fLaC", 4, "Flac0", auto_flac0, auto_calc_flac, NULL},
  {"\177FLAC", 5, "Flac", auto_flac, auto_calc_flac, NULL},
  {"AnxData", 7, "AnxData", auto_anxdata, NULL, NULL},
  {"CELT    ", 8, "CELT", auto_celt, auto_calc_celt, NULL},
  {"\200kate\0\0\0", 8, "Kate", auto_kate, NULL, NULL},
  {"BBCD\0", 5, "Dirac", auto_dirac, NULL, NULL},
  {"", 0, "Unknown", NULL, NULL, NULL}
};

static int
oggz_auto_identify (OGGZ * oggz, long serialno, unsigned char * data, long len)
{
  int i;

  for (i = 0; i < OGGZ_CONTENT_UNKNOWN; i++)
  {
    const oggz_auto_contenttype_t *codec = oggz_auto_codec_ident + i;

    if (len >= codec->bos_str_len &&
        memcmp (data, codec->bos_str, codec->bos_str_len) == 0) {

      oggz_stream_set_content (oggz, serialno, i);

      return 1;
    }
  }

  oggz_stream_set_content (oggz, serialno, OGGZ_CONTENT_UNKNOWN);
  return 0;
}

int
oggz_auto_identify_page (OGGZ * oggz, ogg_page *og, long serialno)
{
  return oggz_auto_identify (oggz, serialno, og->body, og->body_len);
}

int
oggz_auto_identify_packet (OGGZ * oggz, ogg_packet * op, long serialno)
{
  return oggz_auto_identify (oggz, serialno, op->packet, op->bytes);
}

int
oggz_auto_read_bos_page (OGGZ * oggz, ogg_page * og, long serialno,
                         void * user_data)
{
  int content = 0;

  content = oggz_stream_get_content(oggz, serialno);
  if (content < 0 || content >= OGGZ_CONTENT_UNKNOWN) {
    return 0;
  } else if (content == OGGZ_CONTENT_SKELETON && !ogg_page_bos(og)) {
    return auto_fisbone(oggz, serialno, og->body, og->body_len, user_data);
  } else {
    return oggz_auto_codec_ident[content].reader(oggz, serialno, og->body, og->body_len, user_data);
  }
}

int
oggz_auto_read_bos_packet (OGGZ * oggz, ogg_packet * op, long serialno,
                           void * user_data)
{
  int content = 0;

  content = oggz_stream_get_content(oggz, serialno);
  if (content < 0 || content >= OGGZ_CONTENT_UNKNOWN) {
    return 0;
  } else if (content == OGGZ_CONTENT_SKELETON && !op->b_o_s) {
    return auto_fisbone(oggz, serialno, op->packet, op->bytes, user_data);
  } else {
    return oggz_auto_codec_ident[content].reader(oggz, serialno, op->packet, op->bytes, user_data);
  }
}

ogg_int64_t
oggz_auto_calculate_granulepos(int content, ogg_int64_t now,
                oggz_stream_t *stream, ogg_packet *op) {
  if (oggz_auto_codec_ident[content].calculator != NULL) {
    ogg_int64_t r = oggz_auto_codec_ident[content].calculator(now, stream, op);
    return r;
  }

  return now;
}

ogg_int64_t
oggz_auto_calculate_gp_backwards(int content, ogg_int64_t next_packet_gp,
      oggz_stream_t *stream, ogg_packet *this_packet, ogg_packet *next_packet) {

  if (oggz_auto_codec_ident[content].r_calculator != NULL) {
    return oggz_auto_codec_ident[content].r_calculator(next_packet_gp,
            stream, this_packet, next_packet);
  }

  return 0;

}

int
oggz_auto_read_comments (OGGZ * oggz, oggz_stream_t * stream, long serialno,
                         ogg_packet * op)
{
  int offset = -1;
  long len = -1;

  switch (stream->content) {
    case OGGZ_CONTENT_VORBIS:
      if (op->bytes > 7 && memcmp (op->packet, "\003vorbis", 7) == 0)
        offset = 7;
      break;
    case OGGZ_CONTENT_SPEEX:
      offset = 0; break;
    case OGGZ_CONTENT_THEORA:
      if (op->bytes > 7 && memcmp (op->packet, "\201theora", 7) == 0)
        offset = 7;
      break;
    case OGGZ_CONTENT_KATE:
      if (op->bytes > 9 && memcmp (op->packet, "\201kate\0\0\0", 8) == 0) {
        
        offset = 9;
      }
      break;
    case OGGZ_CONTENT_FLAC:
      if (op->bytes > 4 && (op->packet[0] & 0x7) == 4) {
        len = (op->packet[1]<<16) + (op->packet[2]<<8) + op->packet[3];
        offset = 4;
      }
      break;
    default:
      break;
  }

  

  if (len == -1)
    len = op->bytes - offset;

  if (offset >= 0) {
    oggz_comments_decode (oggz, serialno, op->packet+offset, len);
  }

  return 0;
}

