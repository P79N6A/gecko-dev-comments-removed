





#ifndef mozilla_dom_audiochannelservicechild_h__
#define mozilla_dom_audiochannelservicechild_h__

#include "nsAutoPtr.h"
#include "nsISupports.h"

#include "AudioChannelService.h"
#include "AudioChannelCommon.h"
#include "nsHTMLMediaElement.h"

namespace mozilla {
namespace dom {

class AudioChannelServiceChild : public AudioChannelService
{
public:

  



  static AudioChannelService*
  GetAudioChannelService();

  static void Shutdown();

  virtual void RegisterMediaElement(nsHTMLMediaElement* aMediaElement,
                                    AudioChannelType aType);
  virtual void UnregisterMediaElement(nsHTMLMediaElement* aMediaElement);

  


  virtual bool GetMuted(AudioChannelType aType, bool aMozHidden);

protected:
  AudioChannelServiceChild();
  virtual ~AudioChannelServiceChild();
};

} 
} 

#endif

