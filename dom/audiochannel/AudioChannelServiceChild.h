





#ifndef mozilla_dom_audiochannelservicechild_h__
#define mozilla_dom_audiochannelservicechild_h__

#include "nsAutoPtr.h"
#include "nsISupports.h"

#include "AudioChannelService.h"
#include "AudioChannelCommon.h"

namespace mozilla {
namespace dom {

class AudioChannelServiceChild : public AudioChannelService
{
public:

  



  static AudioChannelService*
  GetAudioChannelService();

  static void Shutdown();

  virtual void RegisterAudioChannelAgent(AudioChannelAgent* aAgent,
                                         AudioChannelType aType);
  virtual void UnregisterAudioChannelAgent(AudioChannelAgent* aAgent);

  


  virtual bool GetMuted(AudioChannelAgent* aAgent, bool aMozHidden);

protected:
  AudioChannelServiceChild();
  virtual ~AudioChannelServiceChild();
};

} 
} 

#endif

