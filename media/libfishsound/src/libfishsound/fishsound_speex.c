































#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_STDINT_H
#include <stdint.h>
#endif

#include <ctype.h>

#include "private.h"
#include "convert.h"


#include "debug.h"

#if HAVE_SPEEX

#if HAVE_SPEEX_1_1
#include <speex/speex.h>
#include <speex/speex_header.h>
#include <speex/speex_stereo.h>
#include <speex/speex_callbacks.h>

#else 

#include <speex.h>
#include <speex_header.h>
#include <speex_stereo.h>
#include <speex_callbacks.h>
#endif




#define VENDOR_FORMAT "Encoded with Speex %s"

#define DEFAULT_ENH_ENABLED 1

#define MAX_FRAME_BYTES 2000

typedef struct _FishSoundSpeexEnc {
  int frame_offset; 
  int pcm_offset;
  char cbits[MAX_FRAME_BYTES];
  int id;
} FishSoundSpeexEnc;

typedef struct _FishSoundSpeexInfo {
  int packetno;
  void * st;
  SpeexBits bits;
  int frame_size;
  int nframes;
  int extra_headers;
  SpeexStereoState stereo;
  int pcm_len; 
  float * ipcm; 
  float * pcm[2]; 
  FishSoundSpeexEnc * enc;
} FishSoundSpeexInfo;

int
fish_sound_speex_identify (unsigned char * buf, long bytes)
{
  SpeexHeader * header;

  if (bytes < 8) return FISH_SOUND_UNKNOWN;

  if (!strncmp ((char *)buf, "Speex   ", 8)) {
    
    if (bytes == 8) return FISH_SOUND_SPEEX;

    

    if ((header = speex_packet_to_header ((char *)buf, (int)bytes)) != NULL) {
      fs_free(header);
      return FISH_SOUND_SPEEX;
    }
  }

  return FISH_SOUND_UNKNOWN;
}

static int
fs_speex_command (FishSound * fsound, int command, void * data, int datasize)
{
  return 0;
}

#if FS_DECODE
static void *
process_header(unsigned char * buf, long bytes, int enh_enabled,
	       int * frame_size, int * rate,
	       int * nframes, int forceMode, int * channels,
	       SpeexStereoState * stereo, int * extra_headers)
{
  void *st;
  SpeexMode *mode;
  SpeexHeader *header;
  int modeID;
  SpeexCallback callback;

  header = speex_packet_to_header((char*)buf, (int)bytes);
  if (!header) {
    
    return NULL;
  }

  if (header->mode >= SPEEX_NB_MODES || header->mode < 0) {
    
    return NULL;
  }

  modeID = header->mode;
  if (forceMode!=-1)
    modeID = forceMode;

#if HAVE_SPEEX_LIB_GET_MODE
  mode = (SpeexMode *) speex_lib_get_mode (modeID);
#else
  
  mode = (SpeexMode *)speex_mode_list[modeID];
#endif

  if (header->speex_version_id > 1) {
    
    return NULL;
  }

  if (mode->bitstream_version < header->mode_bitstream_version) {
    

    return NULL;
  }

  if (mode->bitstream_version > header->mode_bitstream_version) {
    

    return NULL;
  }

  st = speex_decoder_init(mode);
  if (!st) {
    
    return NULL;
  }

  speex_decoder_ctl(st, SPEEX_SET_ENH, &enh_enabled);
  speex_decoder_ctl(st, SPEEX_GET_FRAME_SIZE, frame_size);

  if (!(*channels==1))
    {
      callback.callback_id = SPEEX_INBAND_STEREO;
      callback.func = speex_std_stereo_request_handler;
      callback.data = stereo;
      speex_decoder_ctl(st, SPEEX_SET_HANDLER, &callback);
    }
  if (!*rate)
    *rate = header->rate;
  
  if (forceMode!=-1)
    {
      if (header->mode < forceMode)
	*rate <<= (forceMode - header->mode);
      if (header->mode > forceMode)
	*rate >>= (header->mode - forceMode);
    }

  speex_decoder_ctl(st, SPEEX_SET_SAMPLING_RATE, rate);

  *nframes = header->frames_per_packet;

  if (*channels == -1)
    *channels = header->nb_channels;

  debug_printf (1, "Decoding %d Hz audio using %s mode",
                *rate, mode->modeName);

#ifdef DEBUG
  if (*channels==1)
      fprintf (stderr, " (mono");
   else
      fprintf (stderr, " (stereo");

  if (header->vbr)
    fprintf (stderr, " (VBR)\n");
  else
    fprintf(stderr, "\n");
#endif

  *extra_headers = header->extra_headers;

  fs_free(header);

  return st;
}

static int
fs_speex_free_buffers (FishSound * fsound)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;

  if (fsound->mode == FISH_SOUND_DECODE) {
    if (fss->ipcm && fss->ipcm != fss->pcm[0]) fs_free (fss->ipcm);
    if (fss->pcm[0]) fs_free (fss->pcm[0]);
    if (fss->pcm[1]) fs_free (fss->pcm[1]);
  } else {
    if (fss->ipcm) fs_free (fss->ipcm);
  }

  return 0;
}

static inline int
fs_speex_float_dispatch (FishSound * fsound)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;
  FishSoundDecoded_FloatIlv df;
  FishSoundDecoded_Float dfi;
  int retval;

  if (fsound->interleave) {
    dfi = (FishSoundDecoded_FloatIlv)fsound->callback.decoded_float_ilv;
    retval = dfi (fsound, (float **)fss->ipcm, fss->frame_size,
                  fsound->user_data);
  } else {
    df = (FishSoundDecoded_Float)fsound->callback.decoded_float;
    retval = df (fsound, fss->pcm, fss->frame_size, fsound->user_data);
  }
  
  return retval;
}

static long
fs_speex_decode (FishSound * fsound, unsigned char * buf, long bytes)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;
  int enh_enabled = DEFAULT_ENH_ENABLED;
  int rate = 0;
  int channels = -1;
  int forceMode = -1;
  int i, j;

  if (fss->packetno == 0) {
    fss->st = process_header (buf, bytes, enh_enabled,
			      &fss->frame_size, &rate,
			      &fss->nframes, forceMode, &channels,
			      &fss->stereo,
			      &fss->extra_headers);

    if (fss->st == NULL) {
      
      return FISH_SOUND_ERR_GENERIC;
    }

    debug_printf (1, "speex: got %d channels, %d Hz", channels, rate);

    fsound->info.samplerate = rate;
    fsound->info.channels = channels;

    


    if (channels < 1 || channels > 2)
      return FISH_SOUND_ERR_GENERIC;

#if HAVE_UINTPTR_T
    



    if (fss->frame_size > UINTPTR_MAX / (sizeof(float) * channels))
      return FISH_SOUND_ERR_GENERIC;
#endif

    fss->ipcm = fs_malloc (sizeof (float) * fss->frame_size * channels);
    if (fss->ipcm == NULL) {
      return FISH_SOUND_ERR_OUT_OF_MEMORY;
    }

    if (channels == 1) {
      fss->pcm[0] = fss->ipcm;
    } else if (channels == 2) {
      fss->pcm[0] = fs_malloc (sizeof (float) * fss->frame_size);
      if (fss->pcm[0] == NULL) {
        fs_free (fss->ipcm);
        return FISH_SOUND_ERR_OUT_OF_MEMORY;
      }
      fss->pcm[1] = fs_malloc (sizeof (float) * fss->frame_size);
      if (fss->pcm[1] == NULL) {
        fs_free (fss->pcm[0]);
        fs_free (fss->ipcm);
        return FISH_SOUND_ERR_OUT_OF_MEMORY;
      }
    }

    if (fss->nframes == 0) fss->nframes = 1;

  } else if (fss->packetno == 1) {
    
    if (fish_sound_comments_decode (fsound, buf, bytes) == FISH_SOUND_ERR_OUT_OF_MEMORY) {
      fss->packetno++;
      return FISH_SOUND_ERR_OUT_OF_MEMORY;
    }
  } else if (fss->packetno <= 1+fss->extra_headers) {
    
  } else {
    speex_bits_read_from (&fss->bits, (char *)buf, (int)bytes);

    for (i = 0; i < fss->nframes; i++) {
      
      speex_decode (fss->st, &fss->bits, fss->ipcm);

      if (fsound->info.channels == 2) {
	speex_decode_stereo (fss->ipcm, fss->frame_size, &fss->stereo);
	if (fsound->interleave) {
	  for (j = 0; j < fss->frame_size * fsound->info.channels; j++) {
	    fss->ipcm[j] /= 32767.0;
	  }
	} else {
	  _fs_deinterleave ((float **)fss->ipcm, fss->pcm,
			    fss->frame_size, 2, (float)(1/32767.0));
	}
      } else {
	for (j = 0; j < fss->frame_size; j++) {
	  fss->ipcm[j] /= 32767.0;
	}
      }

      fsound->frameno += fss->frame_size;

      fs_speex_float_dispatch (fsound);
    }
  }

  fss->packetno++;

  return 0;
}
#else 

#define fs_speex_decode NULL

#endif


#if FS_ENCODE
static FishSound *
fs_speex_enc_headers (FishSound * fsound)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;
  int modeID;
  SpeexMode * mode = NULL;
  SpeexHeader header;
  unsigned char * header_buf = NULL, * comments_buf = NULL;
  int header_bytes, comments_bytes;
  size_t buflen;

  modeID = 1;

#if HAVE_SPEEX_LIB_GET_MODE
  mode = (SpeexMode *) speex_lib_get_mode (modeID);
#else
  
  mode = (SpeexMode *)speex_mode_list[modeID];
#endif

  speex_init_header (&header, fsound->info.samplerate, 1, mode);
  header.frames_per_packet = fss->nframes; 
  header.vbr = 1; 
  header.nb_channels = fsound->info.channels;

  fss->st = speex_encoder_init (mode);

  if (fsound->callback.encoded) {
    char vendor_string[128];

    
    header_buf = (unsigned char *) speex_header_to_packet (&header, &header_bytes);
    if (header_buf == NULL) {
      return NULL;
    }

    
    snprintf (vendor_string, 128, VENDOR_FORMAT, header.speex_version);
    if (fish_sound_comment_set_vendor (fsound, vendor_string) == FISH_SOUND_ERR_OUT_OF_MEMORY) {
      fs_free (header_buf);
      return NULL;
    }
    comments_bytes = fish_sound_comments_encode (fsound, NULL, 0);
    comments_buf = fs_malloc (comments_bytes);
    if (comments_buf == NULL) {
      fs_free (header_buf);
      return NULL;
    }
  }

  speex_encoder_ctl (fss->st, SPEEX_SET_SAMPLING_RATE,
		     &fsound->info.samplerate);

  speex_encoder_ctl (fss->st, SPEEX_GET_FRAME_SIZE, &fss->frame_size);

  debug_printf (1, "got frame size %d", fss->frame_size);

  

  buflen = fss->frame_size * fsound->info.channels * sizeof (float);
  fss->ipcm = fs_malloc (buflen);
  if (fss->ipcm == NULL) {
    if (comments_buf) fs_free (comments_buf);
    if (header_buf) fs_free (header_buf);
    return NULL;
  }
  memset (fss->ipcm, 0, buflen);

  
  if (fsound->callback.encoded) {
    FishSoundEncoded encoded = (FishSoundEncoded)fsound->callback.encoded;

    
    encoded (fsound, header_buf, (long)header_bytes, fsound->user_data);
    fss->packetno++;
    fs_free (header_buf);

    
    comments_bytes = fish_sound_comments_encode (fsound, comments_buf, comments_bytes);
    encoded (fsound, comments_buf, (long)comments_bytes, fsound->user_data);
    fss->packetno++;
    fs_free (comments_buf);
  }

  return fsound;
}

static long
fs_speex_encode_write (FishSound * fsound)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;
  FishSoundSpeexEnc * fse = (FishSoundSpeexEnc *)fss->enc;
  int bytes;

  speex_bits_insert_terminator (&fss->bits);
  bytes = speex_bits_write (&fss->bits, fse->cbits, MAX_FRAME_BYTES);
  speex_bits_reset (&fss->bits);

  if (fsound->callback.encoded) {
    FishSoundEncoded encoded = (FishSoundEncoded)fsound->callback.encoded;

    encoded (fsound, (unsigned char *)fse->cbits, (long)bytes,
	     fsound->user_data);
  }

  return bytes;
}

static long
fs_speex_encode_block (FishSound * fsound)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;
  FishSoundSpeexEnc * fse = (FishSoundSpeexEnc *)fss->enc;
  long nencoded = fse->pcm_offset;

  if (fsound->info.channels == 2)
    speex_encode_stereo (fss->ipcm, fse->pcm_offset, &fss->bits);

  speex_encode (fss->st, fss->ipcm, &fss->bits);

  fsound->frameno += fse->pcm_offset;
  fse->frame_offset++;

  if (fse->frame_offset == fss->nframes) {
    fs_speex_encode_write (fsound);
    fse->frame_offset = 0;
  }

  fse->pcm_offset = 0;

  return nencoded;
}

static long
fs_speex_encode_f_ilv (FishSound * fsound, float ** pcm, long frames)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;
  FishSoundSpeexEnc * fse = (FishSoundSpeexEnc *)fss->enc;
  long remaining = frames, len, nencoded = 0;
  int j, start, end;
  int channels = fsound->info.channels;
  float * p = (float *)pcm;

  if (fss->packetno == 0)
    fs_speex_enc_headers (fsound);

  while (remaining > 0) {
    len = MIN (remaining, fss->frame_size - fse->pcm_offset);

    start = fse->pcm_offset * channels;
    end = (len + fse->pcm_offset) * channels;
    for (j = start; j < end; j++) {
      fss->ipcm[j] = *p++ * (float)32767.0;
    }

    fse->pcm_offset += len;

    if (fse->pcm_offset == fss->frame_size) {
      nencoded += fs_speex_encode_block (fsound);
    }

    remaining -= len;
  }

  return frames - remaining;
}

static long
fs_speex_encode_f (FishSound * fsound, float * pcm[], long frames)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;
  FishSoundSpeexEnc * fse = (FishSoundSpeexEnc *)fss->enc;
  long remaining = frames, len, n = 0, nencoded = 0;
  int j, start;

  if (fss->packetno == 0)
    fs_speex_enc_headers (fsound);

  while (remaining > 0) {
    len = MIN (remaining, fss->frame_size - fse->pcm_offset);

    start = fse->pcm_offset;
    fss->pcm[0] = &pcm[0][n];

    if (fsound->info.channels == 2) {
      fss->pcm[1] = &pcm[1][n];
      _fs_interleave (fss->pcm, (float **)&fss->ipcm[start*2],
		      len, 2, 32767.0);
    } else {
      for (j = 0; j < len; j++) {
	fss->ipcm[start + j] = fss->pcm[0][j] * (float)32767.0;
      }
    }

    fse->pcm_offset += len;

    if (fse->pcm_offset == fss->frame_size) {
      nencoded += fs_speex_encode_block (fsound);
    }

    remaining -= len;
    n += len;
  }

  return frames - remaining;
}

static long
fs_speex_flush (FishSound * fsound)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;
  FishSoundSpeexEnc * fse = (FishSoundSpeexEnc *)fss->enc;
  long nencoded = 0;

  if (fsound->mode != FISH_SOUND_ENCODE)
    return 0;

  if (fse->pcm_offset > 0) {
    nencoded += fs_speex_encode_block (fsound);
  }

  





  if (fse->frame_offset == 0) return 0;

  while (fse->frame_offset < fss->nframes) {
    speex_bits_pack (&fss->bits, 15, 5);
    fse->frame_offset++;
  }

  nencoded += fs_speex_encode_write (fsound);
  fse->frame_offset = 0;

  return nencoded;
}

#else 

#define fs_speex_encode_f NULL
#define fs_speex_encode_f_ilv NULL
#define fs_speex_flush NULL

#endif

static int
fs_speex_reset (FishSound * fsound)
{
  

  return 0;
}

static int
fs_speex_update (FishSound * fsound, int interleave)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;
  size_t pcm_size = sizeof (float);
  float *ipcm_new, *pcm0, *pcm1;

  ipcm_new = (float *)fs_realloc (fss->ipcm,
		  pcm_size * fss->frame_size * fsound->info.channels);
  if (ipcm_new == NULL) return FISH_SOUND_ERR_OUT_OF_MEMORY;

  fss->ipcm = ipcm_new;

  if (interleave) {
    

    if (!fsound->interleave && fsound->info.channels == 2) {
      if (fss->pcm[0]) fs_free (fss->pcm[0]);
      if (fss->pcm[1]) fs_free (fss->pcm[1]);
      fss->pcm[0] = NULL;
      fss->pcm[1] = NULL;
    }
  } else {
    if (fsound->info.channels == 1) {
      fss->pcm[0] = (float *) fss->ipcm;
    } else if (fsound->info.channels == 2) {
#if HAVE_UINTPTR_T
      



      if (fss->frame_size > UINTPTR_MAX / pcm_size)
        return FISH_SOUND_ERR_GENERIC;
#endif

      pcm0 = fs_realloc (fss->pcm[0], pcm_size * fss->frame_size);
      if (pcm0 == NULL) {
        return FISH_SOUND_ERR_OUT_OF_MEMORY;
      }

      pcm1 = fs_realloc (fss->pcm[1], pcm_size * fss->frame_size);
      if (pcm1 == NULL) {
        fs_free (pcm0);
        return FISH_SOUND_ERR_OUT_OF_MEMORY;
      }

      fss->pcm[0] = pcm0;
      fss->pcm[1] = pcm1;
    }
  }

  return 0;
}

static FishSound *
fs_speex_enc_init (FishSound * fsound)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;
  FishSoundSpeexEnc * fse;

  fse = fs_malloc (sizeof (FishSoundSpeexEnc));
  if (fse == NULL) return NULL;

  fse->frame_offset = 0;
  fse->pcm_offset = 0;
  fse->id = 0;

  fss->enc = fse;

  return fsound;
}

static FishSound *
fs_speex_init (FishSound * fsound)
{
  FishSoundSpeexInfo * fss;
  SpeexStereoState stereo_init = SPEEX_STEREO_STATE_INIT;

  fss = fs_malloc (sizeof (FishSoundSpeexInfo));
  if (fss == NULL) return NULL;

  fss->packetno = 0;
  fss->st = NULL;
  fss->frame_size = 0;
  fss->nframes = 1;
  fss->pcm_len = 0;
  fss->ipcm = NULL;
  fss->pcm[0] = NULL;
  fss->pcm[1] = NULL;

  memcpy (&fss->stereo, &stereo_init, sizeof (SpeexStereoState));

  speex_bits_init (&fss->bits);

  fsound->codec_data = fss;

  if (fsound->mode == FISH_SOUND_ENCODE)
    fs_speex_enc_init (fsound);

  return fsound;
}

static FishSound *
fs_speex_delete (FishSound * fsound)
{
  FishSoundSpeexInfo * fss = (FishSoundSpeexInfo *)fsound->codec_data;

  fs_speex_free_buffers (fsound);

  if (fsound->mode == FISH_SOUND_DECODE) {
    if (fss->st) speex_decoder_destroy (fss->st);
  } else if (fsound->mode == FISH_SOUND_ENCODE) {
    if (fss->st) speex_encoder_destroy (fss->st);
    if (fss->enc) fs_free (fss->enc);
  }
  speex_bits_destroy (&fss->bits);

  fs_free (fss);
  fsound->codec_data = NULL;

  return fsound;
}

FishSoundCodec *
fish_sound_speex_codec (void)
{
  FishSoundCodec * codec;

  codec = (FishSoundCodec *) fs_malloc (sizeof (FishSoundCodec));
  if (codec == NULL) return NULL;

  codec->format.format = FISH_SOUND_SPEEX;
  codec->format.name = "Speex (Xiph.Org)";
  codec->format.extension = "spx";

  codec->init = fs_speex_init;
  codec->del = fs_speex_delete;
  codec->reset = fs_speex_reset;
  codec->update = fs_speex_update;
  codec->command = fs_speex_command;
  codec->decode = fs_speex_decode;
  codec->encode_f = fs_speex_encode_f;
  codec->encode_f_ilv = fs_speex_encode_f_ilv;
  codec->flush = fs_speex_flush;

  return codec;
}

#else 

int
fish_sound_speex_identify (unsigned char * buf, long bytes)
{
  return FISH_SOUND_UNKNOWN;
}

FishSoundCodec *
fish_sound_speex_codec (void)
{
  return NULL;
}

#endif

