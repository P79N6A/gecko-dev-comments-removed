








#ifndef mozilla_image_src_DecodePool_h
#define mozilla_image_src_DecodePool_h

#include "mozilla/Mutex.h"
#include "mozilla/StaticPtr.h"
#include "nsCOMPtr.h"
#include "nsIEventTarget.h"
#include "nsIObserver.h"

class nsIThread;
class nsIThreadPool;

namespace mozilla {
namespace image {

class Decoder;











class DecodePool : public nsIObserver
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER

  
  static void Initialize();

  
  static DecodePool* Singleton();

  
  void AsyncDecode(Decoder* aDecoder);

  




  void SyncDecodeIfSmall(Decoder* aDecoder);

  




  void SyncDecodeIfPossible(Decoder* aDecoder);

  






  already_AddRefed<nsIEventTarget> GetEventTarget();

  






  already_AddRefed<nsIEventTarget> GetIOEventTarget();

  






  already_AddRefed<nsIRunnable> CreateDecodeWorker(Decoder* aDecoder);

private:
  friend class DecodeWorker;

  DecodePool();
  virtual ~DecodePool();

  void Decode(Decoder* aDecoder);
  void NotifyDecodeComplete(Decoder* aDecoder);
  void NotifyProgress(Decoder* aDecoder);

  static StaticRefPtr<DecodePool> sSingleton;

  
  Mutex                     mMutex;
  nsCOMPtr<nsIThreadPool>   mThreadPool;
  nsCOMPtr<nsIThread>       mIOThread;
};

} 
} 

#endif 
