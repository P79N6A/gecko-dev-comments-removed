








#ifndef MOZILLA_IMAGELIB_DECODEPOOL_H_
#define MOZILLA_IMAGELIB_DECODEPOOL_H_

#include "mozilla/Mutex.h"
#include "mozilla/StaticPtr.h"
#include <mozilla/TypedEnum.h>
#include "nsCOMPtr.h"
#include "nsIEventTarget.h"
#include "nsIObserver.h"

class nsIThreadPool;

namespace mozilla {
namespace image {

class Decoder;
class RasterImage;











class DecodePool : public nsIObserver
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static DecodePool* Singleton();

  
  void AsyncDecode(Decoder* aDecoder);

  




  void SyncDecodeIfSmall(Decoder* aDecoder);

  




  void SyncDecodeIfPossible(Decoder* aDecoder);

  






  already_AddRefed<nsIEventTarget> GetEventTarget();

  






  already_AddRefed<nsIRunnable> CreateDecodeWorker(Decoder* aDecoder);

private:
  friend class DecodeWorker;
  friend class NotifyDecodeCompleteWorker;

  DecodePool();
  virtual ~DecodePool();

  void Decode(Decoder* aDecoder);
  void NotifyDecodeComplete(Decoder* aDecoder);
  void NotifyProgress(Decoder* aDecoder);

  static StaticRefPtr<DecodePool> sSingleton;

  
  
  
  Mutex                     mThreadPoolMutex;
  nsCOMPtr<nsIThreadPool>   mThreadPool;
};

} 
} 

#endif 
