





#ifndef mozilla_dom_audiochannelservice_h__
#define mozilla_dom_audiochannelservice_h__

#include "nsAutoPtr.h"
#include "nsISupports.h"

#include "AudioChannelCommon.h"
#include "nsHTMLMediaElement.h"

namespace mozilla {
namespace dom {

class AudioChannelService : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  



  static AudioChannelService*
  GetAudioChannelService();

  


  static void Shutdown();

  



  virtual void RegisterMediaElement(nsHTMLMediaElement* aMediaElement,
                                    AudioChannelType aType);

  



  virtual void UnregisterMediaElement(nsHTMLMediaElement* aMediaElement);

  


  virtual bool GetMuted(AudioChannelType aType, bool aElementHidden);

protected:
  void Notify();

  
  void RegisterType(AudioChannelType aType);
  void UnregisterType(AudioChannelType aType);

  AudioChannelService();
  virtual ~AudioChannelService();

  bool ChannelsActiveWithHigherPriorityThan(AudioChannelType aType);

  const char* ChannelName(AudioChannelType aType);

  nsDataHashtable< nsPtrHashKey<nsHTMLMediaElement>, AudioChannelType > mMediaElements;

  int32_t* mChannelCounters;

  AudioChannelType mCurrentHigherChannel;

  
  
  friend class ContentParent;
  friend class ContentChild;
};

} 
} 

#endif
