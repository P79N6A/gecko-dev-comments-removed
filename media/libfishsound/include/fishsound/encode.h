































#ifndef __FISH_SOUND_ENCODE_H__
#define __FISH_SOUND_ENCODE_H__

#ifdef __cplusplus
extern "C" {
#endif
















typedef int (*FishSoundEncoded) (FishSound * fsound, unsigned char * buf,
				 long bytes, void * user_data);









int fish_sound_set_encoded_callback (FishSound * fsound,
				     FishSoundEncoded encoded,
				     void * user_data);










long fish_sound_encode_float (FishSound * fsound, float * pcm[], long frames);










long fish_sound_encode_float_ilv (FishSound * fsound, float ** pcm,
				  long frames);

#ifdef __cplusplus
}
#endif

#endif
