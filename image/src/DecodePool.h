








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

MOZ_BEGIN_ENUM_CLASS(DecodeStrategy, uint8_t)
  
  
  
  
  
  SYNC,

  
  
  
  
  
  ASYNC
MOZ_END_ENUM_CLASS(DecodeStrategy)

MOZ_BEGIN_ENUM_CLASS(DecodeStatus, uint8_t)
  INACTIVE,
  PENDING,
  ACTIVE,
  WORK_DONE,
  STOPPED
MOZ_END_ENUM_CLASS(DecodeStatus)

MOZ_BEGIN_ENUM_CLASS(DecodeUntil, uint8_t)
  TIME,
  SIZE,
  DONE_BYTES
MOZ_END_ENUM_CLASS(DecodeUntil)

MOZ_BEGIN_ENUM_CLASS(ShutdownReason, uint8_t)
  DONE,
  NOT_NEEDED,
  FATAL_ERROR
MOZ_END_ENUM_CLASS(ShutdownReason)















class DecodePool : public nsIObserver
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static DecodePool* Singleton();

  


  void RequestDecode(RasterImage* aImage);

  



  void DecodeABitOf(RasterImage* aImage, DecodeStrategy aStrategy);

  






  static void StopDecoding(RasterImage* aImage);

  







  nsresult DecodeUntilSizeAvailable(RasterImage* aImage);

  





  already_AddRefed<nsIEventTarget> GetEventTarget();

  





  nsresult DecodeSomeOfImage(RasterImage* aImage,
                             DecodeStrategy aStrategy,
                             DecodeUntil aDecodeUntil = DecodeUntil::TIME,
                             uint32_t bytesToDecode = 0);

private:
  DecodePool();
  virtual ~DecodePool();

  static StaticRefPtr<DecodePool> sSingleton;

  
  
  
  Mutex                     mThreadPoolMutex;
  nsCOMPtr<nsIThreadPool>   mThreadPool;
};

} 
} 

#endif 
