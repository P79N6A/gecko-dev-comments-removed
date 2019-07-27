





#ifndef mozilla_dom_SpeakerManagerServicechild_h__
#define mozilla_dom_SpeakerManagerServicechild_h__

#include "nsAutoPtr.h"
#include "nsISupports.h"
#include "SpeakerManagerService.h"

namespace mozilla {
namespace dom {



class SpeakerManagerServiceChild : public SpeakerManagerService
{
public:
  


  static SpeakerManagerService* GetSpeakerManagerService();
  



  static SpeakerManagerService* GetOrCreateSpeakerManagerService();
  static void Shutdown();
  virtual void ForceSpeaker(bool aEnable, bool aVisible) MOZ_OVERRIDE;
  virtual bool GetSpeakerStatus() MOZ_OVERRIDE;
  virtual void SetAudioChannelActive(bool aIsActive) MOZ_OVERRIDE;
  virtual void Notify() MOZ_OVERRIDE;
protected:
  SpeakerManagerServiceChild();
  virtual ~SpeakerManagerServiceChild();
};

} 
} 

#endif

