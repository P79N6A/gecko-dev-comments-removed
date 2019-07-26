















#include <cubeb/cubeb-stdint.h>









typedef int32_t status_t;




struct Buffer {
  uint32_t    flags;
  int         channelCount;
  int         format;
  size_t      frameCount;
  size_t      size;
  union {
    void*       raw;
    short*      i16;
    int8_t*     i8;
  };
};

enum event_type {
  EVENT_MORE_DATA = 0,
  EVENT_UNDERRUN = 1,
  EVENT_LOOP_END = 2,
  EVENT_MARKER = 3,
  EVENT_NEW_POS = 4,
  EVENT_BUFFER_END = 5
};







#define AUDIO_STREAM_TYPE_MUSIC 3

enum {
  AUDIO_CHANNEL_OUT_FRONT_LEFT_ICS  = 0x1,
  AUDIO_CHANNEL_OUT_FRONT_RIGHT_ICS = 0x2,
  AUDIO_CHANNEL_OUT_MONO_ICS     = AUDIO_CHANNEL_OUT_FRONT_LEFT_ICS,
  AUDIO_CHANNEL_OUT_STEREO_ICS   = (AUDIO_CHANNEL_OUT_FRONT_LEFT_ICS | AUDIO_CHANNEL_OUT_FRONT_RIGHT_ICS)
} AudioTrack_ChannelMapping_ICS;

enum {
  AUDIO_CHANNEL_OUT_FRONT_LEFT_Legacy = 0x4,
  AUDIO_CHANNEL_OUT_FRONT_RIGHT_Legacy = 0x8,
  AUDIO_CHANNEL_OUT_MONO_Legacy = AUDIO_CHANNEL_OUT_FRONT_LEFT_Legacy,
  AUDIO_CHANNEL_OUT_STEREO_Legacy = (AUDIO_CHANNEL_OUT_FRONT_LEFT_Legacy | AUDIO_CHANNEL_OUT_FRONT_RIGHT_Legacy)
} AudioTrack_ChannelMapping_Legacy;

typedef enum {
  AUDIO_FORMAT_PCM = 0x00000000,
  AUDIO_FORMAT_PCM_SUB_16_BIT = 0x1,
  AUDIO_FORMAT_PCM_16_BIT = (AUDIO_FORMAT_PCM | AUDIO_FORMAT_PCM_SUB_16_BIT),
} AudioTrack_SampleType;

