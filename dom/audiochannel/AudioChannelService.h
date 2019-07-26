





#ifndef mozilla_dom_audiochannelservice_h__
#define mozilla_dom_audiochannelservice_h__

#include "nsAutoPtr.h"
#include "nsISupports.h"

#include "AudioChannelCommon.h"
#include "AudioChannelAgent.h"
#include "nsDataHashtable.h"

namespace mozilla {
namespace dom {

class AudioChannelService : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  



  static AudioChannelService*
  GetAudioChannelService();

  


  static void Shutdown();

  



  virtual void RegisterAudioChannelAgent(AudioChannelAgent* aAgent,
                                         AudioChannelType aType);

  



  virtual void UnregisterAudioChannelAgent(AudioChannelAgent* aAgent);

  


  virtual bool GetMuted(AudioChannelType aType, bool aElementHidden);

protected:
  void Notify();

  
  void RegisterType(AudioChannelType aType);
  void UnregisterType(AudioChannelType aType);

  AudioChannelService();
  virtual ~AudioChannelService();

  bool ChannelsActiveWithHigherPriorityThan(AudioChannelType aType);

  const char* ChannelName(AudioChannelType aType);

  nsDataHashtable< nsPtrHashKey<AudioChannelAgent>, AudioChannelType > mAgents;

  int32_t* mChannelCounters;

  AudioChannelType mCurrentHigherChannel;

  
  
  friend class ContentParent;
  friend class ContentChild;
};

} 
} 

#endif
