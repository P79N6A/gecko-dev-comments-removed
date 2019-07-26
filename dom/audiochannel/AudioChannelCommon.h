





#ifndef mozilla_dom_audiochannelcommon_h__
#define mozilla_dom_audiochannelcommon_h__

namespace mozilla {
namespace dom {



enum AudioChannelType {
  AUDIO_CHANNEL_DEFAULT = -1,
  AUDIO_CHANNEL_NORMAL = 0,
  AUDIO_CHANNEL_CONTENT,
  AUDIO_CHANNEL_NOTIFICATION,
  AUDIO_CHANNEL_ALARM,
  AUDIO_CHANNEL_TELEPHONY,
  AUDIO_CHANNEL_RINGER,
  AUDIO_CHANNEL_PUBLICNOTIFICATION,
  AUDIO_CHANNEL_LAST
};

enum AudioChannelState {
  AUDIO_CHANNEL_STATE_NORMAL = 0,
  AUDIO_CHANNEL_STATE_MUTED,
  AUDIO_CHANNEL_STATE_FADED,
  AUDIO_CHANNEL_STATE_LAST
};
} 
} 

#endif

