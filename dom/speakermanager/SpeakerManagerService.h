





#ifndef mozilla_dom_SpeakerManagerService_h__
#define mozilla_dom_SpeakerManagerService_h__

#include "nsAutoPtr.h"
#include "nsIObserver.h"
#include "nsTArray.h"
#include "SpeakerManager.h"
#include "nsIAudioManager.h"
#include "nsCheapSets.h"
#include "nsHashKeys.h"

namespace mozilla {
namespace dom {

class SpeakerManagerService : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  


  static SpeakerManagerService* GetSpeakerManagerService();
  



  static SpeakerManagerService* GetOrCreateSpeakerManagerService();
  virtual void ForceSpeaker(bool aEnable, bool aVisible);
  virtual bool GetSpeakerStatus();
  virtual void SetAudioChannelActive(bool aIsActive);
  
  void ForceSpeaker(bool enable, uint64_t aChildid);
  
  void RegisterSpeakerManager(SpeakerManager* aSpeakerManager)
  {
    mRegisteredSpeakerManagers.AppendElement(aSpeakerManager);
  }
  void UnRegisterSpeakerManager(SpeakerManager* aSpeakerManager)
  {
    mRegisteredSpeakerManagers.RemoveElement(aSpeakerManager);
  }
  


  static void Shutdown();

protected:
  SpeakerManagerService();

  virtual ~SpeakerManagerService();
  
  virtual void Notify();

  void TuruOnSpeaker(bool aEnable);

  nsTArray<nsRefPtr<SpeakerManager> > mRegisteredSpeakerManagers;
  
  nsCheapSet<nsUint64HashKey> mSpeakerStatusSet;
  
  bool mOrgSpeakerStatus;

  bool mVisible;
  
  
  friend class ContentParent;
  friend class ContentChild;
};

} 
} 

#endif
