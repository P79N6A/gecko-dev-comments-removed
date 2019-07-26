





#ifndef mozilla_dom_audio_channel_agent_h__
#define mozilla_dom_audio_channel_agent_h__

#include "nsIAudioChannelAgent.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsWeakPtr.h"

#define NS_AUDIOCHANNELAGENT_CONTRACTID "@mozilla.org/audiochannelagent;1"

#define NS_AUDIOCHANNELAGENT_CID {0xf27688e2, 0x3dd7, 0x11e2, \
      {0x90, 0x4e, 0x10, 0xbf, 0x48, 0xd6, 0x4b, 0xd4}}

namespace mozilla {
namespace dom {


class AudioChannelAgent : public nsIAudioChannelAgent
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIAUDIOCHANNELAGENT

  NS_DECL_CYCLE_COLLECTION_CLASS(AudioChannelAgent)

  AudioChannelAgent();
  virtual void NotifyAudioChannelStateChanged();

private:
  virtual ~AudioChannelAgent();

  
  
  already_AddRefed<nsIAudioChannelAgentCallback> GetCallback();

  nsresult InitInternal(int32_t aAudioAgentType,
                        nsIAudioChannelAgentCallback* aCallback,
                        bool aUseWeakRef, bool aWithVideo=false);

  nsCOMPtr<nsIAudioChannelAgentCallback> mCallback;
  nsWeakPtr mWeakCallback;
  int32_t mAudioChannelType;
  bool mIsRegToService;
  bool mVisible;
  bool mWithVideo;
};

} 
} 
#endif

