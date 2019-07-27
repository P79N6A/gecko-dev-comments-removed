








#ifndef mozilla_image_src_DecodePool_h
#define mozilla_image_src_DecodePool_h

#include "mozilla/Mutex.h"
#include "mozilla/StaticPtr.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsIEventTarget.h"
#include "nsIObserver.h"
#include "nsRefPtr.h"

class nsIThread;
class nsIThreadPool;

namespace mozilla {
namespace image {

class Decoder;
class DecodePoolImpl;











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

  






  already_AddRefed<nsIEventTarget> GetIOEventTarget();

private:
  friend class DecodePoolWorker;

  DecodePool();
  virtual ~DecodePool();

  void Decode(Decoder* aDecoder);
  void NotifyDecodeComplete(Decoder* aDecoder);
  void NotifyProgress(Decoder* aDecoder);

  static StaticRefPtr<DecodePool> sSingleton;

  nsRefPtr<DecodePoolImpl>    mImpl;

  
  Mutex                     mMutex;
  nsCOMArray<nsIThread>     mThreads;
  nsCOMPtr<nsIThread>       mIOThread;
};

} 
} 

#endif 
