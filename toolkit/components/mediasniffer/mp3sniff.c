





#include "mp3sniff.h"


#define MP3_MAX_SIZE 1441

typedef struct {
  int version;
  int layer;
  int errp;
  int bitrate;
  int freq;
  int pad;
  int priv;
  int mode;
  int modex;
  int copyright;
  int original;
  int emphasis;
} mp3_header;


static void mp3_parse(const uint8_t *p, mp3_header *header)
{
  const int bitrates[2][16] = {
        
	{0,  32000,  40000,  48000,  56000,  64000,  80000,  96000,
         112000, 128000, 160000, 192000, 224000, 256000, 320000, 0},
        
        {0, 8000, 16000, 24000, 32000, 40000, 48000, 56000, 64000,
         80000, 96000, 112000, 128000, 144000, 160000, 0} };
  const int samplerates[4] = {44100, 48000, 32000, 0};

  header->version = (p[1] & 0x18) >> 3;
  header->layer = 4 - ((p[1] & 0x06) >> 1);
  header->errp = (p[1] & 0x01);

  header->bitrate = bitrates[(header->version & 1) ? 0 : 1][(p[2] & 0xf0) >> 4];
  header->freq = samplerates[(p[2] & 0x0c) >> 2];
  if (header->version == 2) header->freq >>= 1;
  else if (header->version == 0) header->freq >>= 2;
  header->pad = (p[2] & 0x02) >> 1;
  header->priv = (p[2] & 0x01);

  header->mode = (p[3] & 0xc0) >> 6;
  header->modex = (p[3] & 0x30) >> 4;
  header->copyright = (p[3] & 0x08) >> 3;
  header->original = (p[3] & 0x04) >> 2;
  header->emphasis = (p[3] & 0x03);
}


static int mp3_framesize(mp3_header *header)
{
  int size;
  int scale;

  if ((header->version & 1) == 0) scale = 72;
  else scale = 144;
  size = header->bitrate * scale / header->freq;
  if (header->pad) size += 1;

  return size;
}

static int is_mp3(const uint8_t *p, long length) {
  
  if (length < 4) return 0;
  
  if (p[0] == 0xff && (p[1] & 0xe0) == 0xe0) {
    
    if (((p[1] & 0x06) >> 1) == 0) return 0;  
    if (((p[2] & 0xf0) >> 4) == 15) return 0; 
    if (((p[2] & 0x0c) >> 2) == 3) return 0;  
    
    if ((4 - ((p[1] & 0x06) >> 1)) != 3) return 0; 
    return 1;
  }
  return 0;
}



static int is_id3(const uint8_t *p, long length) {
  
  if (length < 10) return 0;
  
  if (p[0] == 'I' && p[1] == 'D' && p[2] == '3') {
    if (p[3] == 0xff || p[4] == 0xff) return 0; 
    if (p[6] & 0x80 || p[7] & 0x80 ||
        p[8] & 0x80) return 0; 
    
    return 1;
  }
  return 0;
}


static int id3_framesize(const uint8_t *p, long length)
{
  int size;

  
  if (length < 10) {
    return 0;
  }
  
  size = 10 + (p[9] | (p[8] << 7) | (p[7] << 14) | (p[6] << 21));

  return size;
}

int mp3_sniff(const uint8_t *buf, long length)
{
  mp3_header header;
  const uint8_t *p, *q;
  long skip;
  long avail;

  p = buf;
  q = p;
  avail = length;
  while (avail >= 4) {
    if (is_id3(p, avail)) {
      
      skip = id3_framesize(p, avail);
      p += skip;
      avail -= skip;
    } else if (is_mp3(p, avail)) {
      mp3_parse(p, &header);
      skip = mp3_framesize(&header);
      if (skip < 4 || skip + 4 >= avail) {
        return 0;
      }
      p += skip;
      avail -= skip;
      
      if (is_mp3(p, avail)) {
        
        return 1;
      } else {
        
        return 0;
      }
    } else {
      
      return 0;
    }
  }

  return 0;
}
