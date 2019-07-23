































#include "config.h"

#include <stdio.h>
#include <string.h>

#include "private.h"

static int
fs_decode_update (FishSound * fsound, int interleave)
{
  int ret = 0;

  if (fsound->codec && fsound->codec->update)
    ret = fsound->codec->update (fsound, interleave);

  if (ret >= 0) {
    fsound->interleave = interleave;
  }

  return ret;
}

int fish_sound_set_decoded_float (FishSound * fsound,
				  FishSoundDecoded_Float decoded,
				  void * user_data)
{
  int ret = 0;

  if (fsound == NULL) return FISH_SOUND_ERR_BAD;

#if FS_DECODE
  ret = fs_decode_update (fsound, 0);

  if (ret >= 0) {
    fsound->callback.decoded_float = decoded;
    fsound->user_data = user_data;
  }
#else
  return FISH_SOUND_ERR_DISABLED;
#endif

  return ret;
}

int fish_sound_set_decoded_float_ilv (FishSound * fsound,
				      FishSoundDecoded_FloatIlv decoded,
				      void * user_data)
{
  int ret = 0;

  if (fsound == NULL) return FISH_SOUND_ERR_BAD;

#if FS_DECODE
  ret = fs_decode_update (fsound, 1);

  if (ret >= 0) {
    fsound->callback.decoded_float_ilv = decoded;
    fsound->user_data = user_data;
  }
#else
  return FISH_SOUND_ERR_DISABLED;
#endif

  return ret;
}

long
fish_sound_decode (FishSound * fsound, unsigned char * buf, long bytes)
{
  int format;

  if (fsound == NULL) return FISH_SOUND_ERR_BAD;

#if FS_DECODE
  if (fsound->info.format == FISH_SOUND_UNKNOWN) {
    format = fish_sound_identify (buf, bytes);
    if (format == FISH_SOUND_UNKNOWN) return -1;

    fish_sound_set_format (fsound, format);
  }

  

  if (fsound->codec && fsound->codec->decode)
    return fsound->codec->decode (fsound, buf, bytes);
#else
  return FISH_SOUND_ERR_DISABLED;
#endif

  return 0;
}



int fish_sound_set_decoded_callback (FishSound * fsound,
				     FishSoundDecoded_Float decoded,
				     void * user_data)
{
  if (fsound == NULL) return -1;

  return fsound->interleave ?
    fish_sound_set_decoded_float_ilv (fsound, decoded, user_data) :
    fish_sound_set_decoded_float (fsound, decoded, user_data);
}
