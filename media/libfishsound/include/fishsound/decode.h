































#ifndef __FISH_SOUND_DECODE_H__
#define __FISH_SOUND_DECODE_H__

#ifdef __cplusplus
extern "C" {
#endif


















typedef int (*FishSoundDecoded_Float) (FishSound * fsound, float * pcm[],
				       long frames, void * user_data);














typedef int (*FishSoundDecoded_FloatIlv) (FishSound * fsound, float ** pcm,
					  long frames, void * user_data);









int fish_sound_set_decoded_float (FishSound * fsound,
				  FishSoundDecoded_Float decoded,
				  void * user_data);









int fish_sound_set_decoded_float_ilv (FishSound * fsound,
				      FishSoundDecoded_FloatIlv decoded,
				      void * user_data);


















long fish_sound_decode (FishSound * fsound, unsigned char * buf, long bytes);

#ifdef __cplusplus
}
#endif

#endif
