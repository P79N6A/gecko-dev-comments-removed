





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

  



  void RegisterMediaElement(nsHTMLMediaElement* aMediaElement,
                            AudioChannelType aType);

  



  void UnregisterMediaElement(nsHTMLMediaElement* aMediaElement);

  


  virtual bool GetMuted(AudioChannelType aType, bool aElementHidden);

  void Notify();

protected:
  AudioChannelService();
  virtual ~AudioChannelService();

  bool ChannelsActiveWithHigherPriorityThan(AudioChannelType aType);

  nsDataHashtable< nsPtrHashKey<nsHTMLMediaElement>, AudioChannelType > mMediaElements;

  int32_t* mChannelCounters;
};

} 
} 

#endif
