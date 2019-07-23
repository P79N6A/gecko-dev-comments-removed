































#ifndef __FISH_SOUND_DEPRECATED_H__
#define __FISH_SOUND_DEPRECATED_H__





#ifdef __cplusplus
extern "C" {
#endif




















int fish_sound_set_interleave (FishSound * fsound, int interleave);







typedef FishSoundDecoded_Float FishSoundDecoded;















int fish_sound_set_decoded_callback (FishSound * fsound,
				     FishSoundDecoded decoded,
				     void * user_data);




















int fish_sound_set_interleave (FishSound * fsound, int interleave);











long fish_sound_encode (FishSound * fsound, float ** pcm, long frames);

#ifdef __cplusplus
}
#endif

#endif
