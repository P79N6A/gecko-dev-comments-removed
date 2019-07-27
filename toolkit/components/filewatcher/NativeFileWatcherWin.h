



#ifndef mozilla_nativefilewatcher_h__
#define mozilla_nativefilewatcher_h__

#include "nsINativeFileWatcher.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsThreadUtils.h"


#include <windows.h>

namespace mozilla {

class NativeFileWatcherService final : public nsINativeFileWatcherService,
                                           public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSINATIVEFILEWATCHERSERVICE
  NS_DECL_NSIOBSERVER

  NativeFileWatcherService();

  nsresult Init();

private:
  
  HANDLE mIOCompletionPort;
  nsCOMPtr<nsIThread> mIOThread;

  
  nsCOMPtr<nsIRunnable> mWorkerIORunnable;

  nsresult Uninit();
  void WakeUpWorkerThread();

  
  ~NativeFileWatcherService();
  NativeFileWatcherService(const NativeFileWatcherService& other) = delete;
  void operator=(const NativeFileWatcherService& other) = delete;
};

} 

#endif 
