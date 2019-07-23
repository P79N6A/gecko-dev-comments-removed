































#ifndef __FISH_SOUND_COMMENT_H__
#define __FISH_SOUND_COMMENT_H__




















































#include <fishsound/fishsound.h>




typedef struct {
  
  char * name;

  
  char * value;
} FishSoundComment;

#ifdef __cplusplus
extern "C" {
#endif








const char *
fish_sound_comment_get_vendor (FishSound * fsound);








const FishSoundComment *
fish_sound_comment_first (FishSound * fsound);








const FishSoundComment *
fish_sound_comment_next (FishSound * fsound, const FishSoundComment * comment);










const FishSoundComment *
fish_sound_comment_first_byname (FishSound * fsound, char * name);











const FishSoundComment *
fish_sound_comment_next_byname (FishSound * fsound,
				const FishSoundComment * comment);









int
fish_sound_comment_add (FishSound * fsound, FishSoundComment * comment);










int
fish_sound_comment_add_byname (FishSound * fsound, const char * name,
			       const char * value);










int
fish_sound_comment_remove (FishSound * fsound, FishSoundComment * comment);









int
fish_sound_comment_remove_byname (FishSound * fsound, char * name);

#ifdef __cplusplus
}
#endif

#endif
