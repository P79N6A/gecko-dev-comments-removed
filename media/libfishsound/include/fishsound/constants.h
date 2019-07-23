































#ifndef __FISH_SOUND_CONSTANTS_H__
#define __FISH_SOUND_CONSTANTS_H__






typedef enum _FishSoundMode {
  
  FISH_SOUND_DECODE = 0x10,

  
  FISH_SOUND_ENCODE = 0x20
} FishSoundMode;


typedef enum _FishSoundCodecID {
  
  FISH_SOUND_UNKNOWN = 0x00,

  
  FISH_SOUND_VORBIS  = 0x01,

  
  FISH_SOUND_SPEEX   = 0x02,

  
  FISH_SOUND_FLAC    = 0x03
} FishSoundCodecID;


typedef enum _FishSoundStopCtl {
  
  FISH_SOUND_CONTINUE = 0,
  
  
  FISH_SOUND_STOP_OK  = 1,
  
  
  FISH_SOUND_STOP_ERR = -1
} FishSoundStopCtl;


typedef enum _FishSoundCommand {
  
  FISH_SOUND_COMMAND_NOP                = 0x0000,

  
  FISH_SOUND_GET_INFO                   = 0x1000,

  
  FISH_SOUND_GET_INTERLEAVE             = 0x2000,

  
  FISH_SOUND_SET_INTERLEAVE             = 0x2001,

  FISH_SOUND_SET_ENCODE_VBR             = 0x4000,
  
  FISH_SOUND_COMMAND_MAX
} FishSoundCommand;


typedef enum _FishSoundError {
  
  FISH_SOUND_OK                         = 0,

  
  FISH_SOUND_ERR_GENERIC                = -1,

  
  FISH_SOUND_ERR_BAD                    = -2,

  
  FISH_SOUND_ERR_INVALID                = -3,

  
  FISH_SOUND_ERR_DISABLED               = -10,

  
  FISH_SOUND_ERR_SHORT_IDENTIFY         = -20,

  
  FISH_SOUND_ERR_COMMENT_INVALID        = -21
} FishSoundError;

#endif 
