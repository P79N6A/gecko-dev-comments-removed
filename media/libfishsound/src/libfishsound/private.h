































#ifndef __FISH_SOUND_PRIVATE_H__
#define __FISH_SOUND_PRIVATE_H__

#include <stdlib.h>

#include "fs_compat.h"
#include "fs_vector.h"

#include <fishsound/constants.h>

#undef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))

typedef struct _FishSound FishSound;
typedef struct _FishSoundInfo FishSoundInfo;
typedef struct _FishSoundCodec FishSoundCodec;
typedef struct _FishSoundFormat FishSoundFormat;
typedef struct _FishSoundComment FishSoundComment;

typedef int         (*FSCodecIdentify) (unsigned char * buf, long bytes);
typedef FishSound * (*FSCodecInit) (FishSound * fsound);
typedef FishSound * (*FSCodecDelete) (FishSound * fsound);
typedef int         (*FSCodecReset) (FishSound * fsound);
typedef int         (*FSCodecUpdate) (FishSound * fsound, int interleave);
typedef int         (*FSCodecCommand) (FishSound * fsound, int command,
				       void * data, int datasize);
typedef long        (*FSCodecDecode) (FishSound * fsound, unsigned char * buf,
				      long bytes);
typedef long        (*FSCodecEncode_Float) (FishSound * fsound, float * pcm[],
					    long frames);
typedef long        (*FSCodecEncode_FloatIlv) (FishSound * fsound,
					       float ** pcm, long frames);
typedef long        (*FSCodecFlush) (FishSound * fsound);

#include <fishsound/decode.h>
#include <fishsound/encode.h>

struct _FishSoundFormat {
  int format;
  const char * name;
  const char * extension;
};

struct _FishSoundCodec {
  struct _FishSoundFormat format;
  FSCodecInit init;
  FSCodecDelete del;
  FSCodecReset reset;
  FSCodecUpdate update;
  FSCodecCommand command;
  FSCodecDecode decode;
  FSCodecEncode_FloatIlv encode_f_ilv;
  FSCodecEncode_Float encode_f;
  FSCodecFlush flush;
};

struct _FishSoundInfo {
  int samplerate;
  int channels;
  int format;
};

struct _FishSoundComment {
  char * name;
  char * value;
};

union FishSoundCallback {
  FishSoundDecoded_Float decoded_float;
  FishSoundDecoded_FloatIlv decoded_float_ilv;
  FishSoundEncoded encoded;
};

struct _FishSound {
  
  FishSoundMode mode;

  
  FishSoundInfo info;

  
  int interleave;

  


  long frameno;

  




  long next_granulepos;

  





  int next_eos;

  
  FishSoundCodec * codec;

  
  void * codec_data;

  
  union FishSoundCallback callback;

  
  void * user_data; 

  
  char * vendor;
  FishSoundVector * comments;
};

int fish_sound_identify (unsigned char * buf, long bytes);
int fish_sound_set_format (FishSound * fsound, int format);  


int fish_sound_vorbis_identify (unsigned char * buf, long bytes);
FishSoundCodec * fish_sound_vorbis_codec (void);

int fish_sound_speex_identify (unsigned char * buf, long bytes);
FishSoundCodec * fish_sound_speex_codec (void);

int fish_sound_flac_identify (unsigned char * buf, long bytes);
FishSoundCodec * fish_sound_flac_codec (void);


int fish_sound_comments_init (FishSound * fsound);
int fish_sound_comments_free (FishSound * fsound);
int fish_sound_comments_decode (FishSound * fsound, unsigned char * buf,
				long bytes);
long fish_sound_comments_encode (FishSound * fsound, unsigned char * buf,
				 long length);









int
fish_sound_comment_set_vendor (FishSound * fsound, const char * vendor);

const FishSoundComment * fish_sound_comment_first (FishSound * fsound);
const FishSoundComment *
fish_sound_comment_next (FishSound * fsound, const FishSoundComment * comment);

#endif 
