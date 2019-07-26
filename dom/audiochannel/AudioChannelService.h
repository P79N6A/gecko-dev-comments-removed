





#ifndef mozilla_dom_audiochannelservice_h__
#define mozilla_dom_audiochannelservice_h__

#include "nsAutoPtr.h"
#include "nsIObserver.h"

#include "AudioChannelCommon.h"
#include "AudioChannelAgent.h"
#include "nsClassHashtable.h"

namespace mozilla {
namespace dom {

class AudioChannelService : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  



  static AudioChannelService*
  GetAudioChannelService();

  


  static void Shutdown();

  



  virtual void RegisterAudioChannelAgent(AudioChannelAgent* aAgent,
                                         AudioChannelType aType);

  



  virtual void UnregisterAudioChannelAgent(AudioChannelAgent* aAgent);

  


  virtual bool GetMuted(AudioChannelAgent* aAgent, bool aElementHidden);

  



  virtual bool ContentChannelIsActive();

protected:
  void Notify();

  


  void SendAudioChannelChangedNotification();

  
  void RegisterType(AudioChannelType aType, uint64_t aChildID);
  void UnregisterType(AudioChannelType aType, bool aElementHidden,
                      uint64_t aChildID);

  bool GetMutedInternal(AudioChannelType aType, uint64_t aChildID,
                        bool aElementHidden, bool aElementWasHidden);

  AudioChannelService();
  virtual ~AudioChannelService();

  enum AudioChannelInternalType {
    AUDIO_CHANNEL_INT_NORMAL = 0,
    AUDIO_CHANNEL_INT_NORMAL_HIDDEN,
    AUDIO_CHANNEL_INT_CONTENT,
    AUDIO_CHANNEL_INT_CONTENT_HIDDEN,
    AUDIO_CHANNEL_INT_NOTIFICATION,
    AUDIO_CHANNEL_INT_ALARM,
    AUDIO_CHANNEL_INT_TELEPHONY,
    AUDIO_CHANNEL_INT_RINGER,
    AUDIO_CHANNEL_INT_PUBLICNOTIFICATION,
    AUDIO_CHANNEL_INT_LAST
  };

  bool ChannelsActiveWithHigherPriorityThan(AudioChannelInternalType aType);

  bool HasMoreThanOneContentChannelHidden();

  const char* ChannelName(AudioChannelType aType);

  AudioChannelInternalType GetInternalType(AudioChannelType aType,
                                           bool aElementHidden);

  class AudioChannelAgentData {
  public:
    AudioChannelAgentData(AudioChannelType aType,
                          bool aElementHidden,
                          bool aMuted)
    : mType(aType)
    , mElementHidden(aElementHidden)
    , mMuted(aMuted)
    {}

    AudioChannelType mType;
    bool mElementHidden;
    bool mMuted;
  };

  static PLDHashOperator
  NotifyEnumerator(AudioChannelAgent* aAgent,
                   AudioChannelAgentData* aData, void *aUnused);

  nsClassHashtable< nsPtrHashKey<AudioChannelAgent>, AudioChannelAgentData > mAgents;

  nsTArray<uint64_t> mChannelCounters[AUDIO_CHANNEL_INT_LAST];

  AudioChannelType mCurrentHigherChannel;

  
  
  friend class ContentParent;
  friend class ContentChild;
};

} 
} 

#endif
