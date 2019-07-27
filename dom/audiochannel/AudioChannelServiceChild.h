





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

  



  static AudioChannelService* GetAudioChannelService();

  




  static AudioChannelService* GetOrCreateAudioChannelService();

  static void Shutdown();

  virtual void RegisterAudioChannelAgent(AudioChannelAgent* aAgent,
                                         AudioChannel aChannel,
                                         bool aWithVideo);
  virtual void UnregisterAudioChannelAgent(AudioChannelAgent* aAgent);

  



  virtual AudioChannelState GetState(AudioChannelAgent* aAgent,
                                     bool aElementHidden);

  virtual void SetDefaultVolumeControlChannel(int32_t aChannel,
                                              bool aHidden);

protected:
  AudioChannelServiceChild();
  virtual ~AudioChannelServiceChild();
};

} 
} 

#endif

