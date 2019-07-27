





#if !defined(MediaShutdownManager_h_)
#define MediaShutdownManager_h_

#include "nsIObserver.h"
#include "mozilla/Monitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/StaticPtr.h"
#include "nsIThread.h"
#include "nsCOMPtr.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"

namespace mozilla {

class MediaDecoder;






































class MediaShutdownManager : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  
  static MediaShutdownManager& Instance();

  
  
  void Register(MediaDecoder* aDecoder);

  
  
  
  void Unregister(MediaDecoder* aDecoder);

private:

  MediaShutdownManager();
  virtual ~MediaShutdownManager();

  void Shutdown();

  
  
  void EnsureCorrectShutdownObserverState();

  static StaticRefPtr<MediaShutdownManager> sInstance;

  
  
  
  nsTHashtable<nsRefPtrHashKey<MediaDecoder>> mDecoders;

  
  bool mIsObservingShutdown;

  bool mIsDoingXPCOMShutDown;
};

} 

#endif
