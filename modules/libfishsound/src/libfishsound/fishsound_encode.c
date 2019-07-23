































#include "config.h"

#include <stdio.h>
#include <string.h>

#include "private.h"

int
fish_sound_set_encoded_callback (FishSound * fsound,
				 FishSoundEncoded encoded,
				 void * user_data)
{
  if (fsound == NULL) return -1;

#if FS_ENCODE
  fsound->callback.encoded = (void *)encoded;
  fsound->user_data = user_data;
#else
  return FISH_SOUND_ERR_DISABLED;
#endif

  return 0;
}

long fish_sound_encode_float (FishSound * fsound, float * pcm[], long frames)
{
  if (fsound == NULL) return -1;

#if FS_ENCODE
  if (fsound->codec && fsound->codec->encode_f)
    return fsound->codec->encode_f (fsound, pcm, frames);
#else
  return FISH_SOUND_ERR_DISABLED;
#endif

  return 0;
}

long fish_sound_encode_float_ilv (FishSound * fsound, float ** pcm,
				  long frames)
{
  if (fsound == NULL) return -1;

#if FS_ENCODE
  if (fsound->codec && fsound->codec->encode_f_ilv)
    return fsound->codec->encode_f_ilv (fsound, pcm, frames);
#else
  return FISH_SOUND_ERR_DISABLED;
#endif

  return 0;
}

#ifndef FS_DISABLE_DEPRECATED
long
fish_sound_encode (FishSound * fsound, float ** pcm, long frames)
{
  if (fsound == NULL) return -1;

#if FS_ENCODE
  if (fsound->interleave) {
    if (fsound->codec && fsound->codec->encode_f_ilv)
      return fsound->codec->encode_f_ilv (fsound, pcm, frames);
  } else {
    if (fsound->codec && fsound->codec->encode_f)
      return fsound->codec->encode_f (fsound, pcm, frames);
  }
#else
  return FISH_SOUND_ERR_DISABLED;
#endif

  return 0;
}
#endif 
