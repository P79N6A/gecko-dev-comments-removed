





#ifndef mozilla_dom_audiochannelservice_h__
#define mozilla_dom_audiochannelservice_h__

#include "nsAutoPtr.h"
#include "nsIObserver.h"

#include "AudioChannelCommon.h"
#include "AudioChannelAgent.h"
#include "nsDataHashtable.h"

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

  


  virtual bool GetMuted(AudioChannelType aType, bool aElementHidden);

protected:
  void Notify();

  
  void RegisterType(AudioChannelType aType, uint64_t aChildID);
  void UnregisterType(AudioChannelType aType, uint64_t aChildID);

  AudioChannelService();
  virtual ~AudioChannelService();

  bool ChannelsActiveWithHigherPriorityThan(AudioChannelType aType);

  const char* ChannelName(AudioChannelType aType);

  nsDataHashtable< nsPtrHashKey<AudioChannelAgent>, AudioChannelType > mAgents;

  nsTArray<uint64_t> mChannelCounters[AUDIO_CHANNEL_PUBLICNOTIFICATION+1];

  AudioChannelType mCurrentHigherChannel;

  
  
  friend class ContentParent;
  friend class ContentChild;
};

} 
} 

#endif
