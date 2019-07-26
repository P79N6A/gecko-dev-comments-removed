





#ifndef mozilla_dom_audio_channel_agent_h__
#define mozilla_dom_audio_channel_agent_h__

#include "nsIAudioChannelAgent.h"
#include "nsCOMPtr.h"

#define NS_AUDIOCHANNELAGENT_CONTRACTID "@mozilla.org/audiochannelagent;1"

#define NS_AUDIOCHANNELAGENT_CID {0xf27688e2, 0x3dd7, 0x11e2, \
      {0x90, 0x4e, 0x10, 0xbf, 0x48, 0xd6, 0x4b, 0xd4}}

namespace mozilla {
namespace dom {


class AudioChannelAgent : public nsIAudioChannelAgent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIAUDIOCHANNELAGENT

  AudioChannelAgent();
  virtual void NotifyAudioChannelStateChanged();

private:
  virtual ~AudioChannelAgent();
  nsCOMPtr<nsIAudioChannelAgentCallback> mCallback;
  int32_t mAudioChannelType;
  bool mIsRegToService;
  bool mVisible;
};

} 
} 
#endif

