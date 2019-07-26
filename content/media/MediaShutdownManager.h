





#if !defined(MediaShutdownManager_h_)
#define MediaShutdownManager_h_

#include "nsIObserver.h"
#include "mozilla/Monitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/StaticPtr.h"
#include "nsIThread.h"
#include "nsCOMPtr.h"
#include "nsTHashtable.h"

namespace mozilla {

class MediaDecoder;
class StateMachineThread;








































class MediaShutdownManager : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  
  static MediaShutdownManager& Instance();

  
  
  void Register(MediaDecoder* aDecoder);

  
  
  
  void Unregister(MediaDecoder* aDecoder);

  
  
  
  
  void Register(StateMachineThread* aThread);

  
  
  
  
  void Unregister(StateMachineThread* aThread);

private:

  MediaShutdownManager();
  virtual ~MediaShutdownManager();

  void Shutdown();

  
  
  void EnsureCorrectShutdownObserverState();

  static StaticRefPtr<MediaShutdownManager> sInstance;

  
  
  nsTHashtable<nsPtrHashKey<MediaDecoder>> mDecoders;

  
  
  
  
  
  nsTHashtable<nsRefPtrHashKey<StateMachineThread>> mStateMachineThreads;

  
  bool mIsObservingShutdown;

  bool mIsDoingXPCOMShutDown;
};














class StateMachineThread {
public:
  StateMachineThread();
  ~StateMachineThread();

  NS_INLINE_DECL_REFCOUNTING(StateMachineThread);

  
  nsresult Init();

  
  
  nsIThread* GetThread();

  
  
  
  
  void Shutdown();

  
  
  
  void SpinUntilShutdownComplete();

private:
  void ShutdownThread();
  nsCOMPtr<nsIThread> mThread;
};

} 

#endif
