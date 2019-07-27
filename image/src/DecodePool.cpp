




#include "DecodePool.h"

#include <algorithm>

#include "mozilla/ClearOnShutdown.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIObserverService.h"
#include "nsIThreadPool.h"
#include "nsProxyRelease.h"
#include "nsXPCOMCIDInternal.h"
#include "prsystem.h"

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

#include "gfxPrefs.h"

#include "Decoder.h"
#include "RasterImage.h"

using std::max;
using std::min;

namespace mozilla {
namespace image {





class NotifyProgressWorker : public nsRunnable
{
public:
  



  static void Dispatch(RasterImage* aImage)
  {
    nsCOMPtr<nsIRunnable> worker = new NotifyProgressWorker(aImage);
    NS_DispatchToMainThread(worker);
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    ReentrantMonitorAutoEnter lock(mImage->mDecodingMonitor);

    mImage->FinishedSomeDecoding(ShutdownReason::DONE);

    return NS_OK;
  }

private:
  explicit NotifyProgressWorker(RasterImage* aImage)
    : mImage(aImage)
  { }

  nsRefPtr<RasterImage> mImage;
};

class DecodeWorker : public nsRunnable
{
public:
  explicit DecodeWorker(RasterImage* aImage)
    : mImage(aImage)
  { }

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    ReentrantMonitorAutoEnter lock(mImage->mDecodingMonitor);

    
    if (mImage->mDecodeStatus == DecodeStatus::STOPPED) {
      NotifyProgressWorker::Dispatch(mImage);
      return NS_OK;
    }

    
    if (!mImage->mDecoder || mImage->IsDecodeFinished()) {
      NotifyProgressWorker::Dispatch(mImage);
      return NS_OK;
    }

    mImage->mDecodeStatus = DecodeStatus::ACTIVE;

    size_t oldByteCount = mImage->mDecoder->BytesDecoded();

    
    
    DecodeUntil type = NS_IsMainThread() ? DecodeUntil::TIME
                                         : DecodeUntil::DONE_BYTES;

    size_t maxBytes = mImage->mSourceData.Length() -
                      mImage->mDecoder->BytesDecoded();
    DecodePool::Singleton()->DecodeSomeOfImage(mImage, type, maxBytes);

    size_t bytesDecoded = mImage->mDecoder->BytesDecoded() - oldByteCount;

    mImage->mDecodeStatus = DecodeStatus::WORK_DONE;

    if (mImage->mDecoder &&
        !mImage->mError &&
        !mImage->mPendingError &&
        !mImage->IsDecodeFinished() &&
        bytesDecoded < maxBytes &&
        bytesDecoded > 0) {
      
      
      DecodePool::Singleton()->RequestDecode(mImage);
    } else {
      
      NotifyProgressWorker::Dispatch(mImage);
    }

    return NS_OK;
  }

protected:
  virtual ~DecodeWorker()
  {
    if (gfxPrefs::ImageMTDecodingEnabled()) {
      
      nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
      NS_WARN_IF_FALSE(mainThread, "Couldn't get the main thread!");
      if (mainThread) {
        
        RasterImage* rawImg = nullptr;
        mImage.swap(rawImg);
        DebugOnly<nsresult> rv = NS_ProxyRelease(mainThread, NS_ISUPPORTS_CAST(ImageResource*, rawImg));
        MOZ_ASSERT(NS_SUCCEEDED(rv), "Failed to proxy release to main thread");
      }
    }
  }

private:
  nsRefPtr<RasterImage> mImage;
};

#ifdef MOZ_NUWA_PROCESS

class RIDThreadPoolListener MOZ_FINAL : public nsIThreadPoolListener
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSITHREADPOOLLISTENER
  
  RIDThreadPoolListener() { }

private:
  ~RIDThreadPoolListener() { }
};

NS_IMPL_ISUPPORTS(RIDThreadPoolListener, nsIThreadPoolListener)

NS_IMETHODIMP
RIDThreadPoolListener::OnThreadCreated()
{
    if (IsNuwaProcess()) {
        NuwaMarkCurrentThread(static_cast<void(*)(void*)>(nullptr), nullptr);
    }
    return NS_OK;
}

NS_IMETHODIMP
RIDThreadPoolListener::OnThreadShuttingDown()
{
    return NS_OK;
}

#endif 






 StaticRefPtr<DecodePool> DecodePool::sSingleton;

NS_IMPL_ISUPPORTS(DecodePool, nsIObserver)

 DecodePool*
DecodePool::Singleton()
{
  if (!sSingleton) {
    MOZ_ASSERT(NS_IsMainThread());
    sSingleton = new DecodePool();
    ClearOnShutdown(&sSingleton);
  }

  return sSingleton;
}

already_AddRefed<nsIEventTarget>
DecodePool::GetEventTarget()
{
  nsCOMPtr<nsIEventTarget> target = do_QueryInterface(mThreadPool);
  return target.forget();
}

DecodePool::DecodePool()
  : mThreadPoolMutex("Thread Pool")
{
  if (gfxPrefs::ImageMTDecodingEnabled()) {
    mThreadPool = do_CreateInstance(NS_THREADPOOL_CONTRACTID);
    if (mThreadPool) {
      mThreadPool->SetName(NS_LITERAL_CSTRING("ImageDecoder"));
      int32_t prefLimit = gfxPrefs::ImageMTDecodingLimit();
      uint32_t limit;
      if (prefLimit <= 0) {
        limit = max(PR_GetNumberOfProcessors(), 2) - 1;
      } else {
        limit = static_cast<uint32_t>(prefLimit);
      }

      mThreadPool->SetThreadLimit(limit);
      mThreadPool->SetIdleThreadLimit(limit);

#ifdef MOZ_NUWA_PROCESS
      if (IsNuwaProcess()) {
        mThreadPool->SetListener(new RIDThreadPoolListener());
      }
#endif

      nsCOMPtr<nsIObserverService> obsSvc = services::GetObserverService();
      if (obsSvc) {
        obsSvc->AddObserver(this, "xpcom-shutdown-threads", false);
      }
    }
  }
}

DecodePool::~DecodePool()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must shut down DecodePool on main thread!");
}

NS_IMETHODIMP
DecodePool::Observe(nsISupports*, const char* aTopic, const char16_t*)
{
  MOZ_ASSERT(strcmp(aTopic, "xpcom-shutdown-threads") == 0, "Unexpected topic");

  nsCOMPtr<nsIThreadPool> threadPool;

  {
    MutexAutoLock threadPoolLock(mThreadPoolMutex);
    threadPool = mThreadPool;
    mThreadPool = nullptr;
  }

  if (threadPool) {
    threadPool->Shutdown();
  }

  return NS_OK;
}

void
DecodePool::RequestDecode(RasterImage* aImage)
{
  MOZ_ASSERT(aImage->mDecoder);
  aImage->mDecodingMonitor.AssertCurrentThreadIn();

  if (aImage->mDecodeStatus == DecodeStatus::PENDING ||
      aImage->mDecodeStatus == DecodeStatus::ACTIVE) {
    
    
    return;
  }

  aImage->mDecodeStatus = DecodeStatus::PENDING;
  nsCOMPtr<nsIRunnable> worker = new DecodeWorker(aImage);

  MutexAutoLock threadPoolLock(mThreadPoolMutex);
  if (!gfxPrefs::ImageMTDecodingEnabled() || !mThreadPool) {
    NS_DispatchToMainThread(worker);
  } else {
    mThreadPool->Dispatch(worker, nsIEventTarget::DISPATCH_NORMAL);
  }
}

void
DecodePool::DecodeABitOf(RasterImage* aImage)
{
  MOZ_ASSERT(NS_IsMainThread());
  aImage->mDecodingMonitor.AssertCurrentThreadIn();

  
  if (aImage->mDecodeStatus == DecodeStatus::WORK_DONE) {
    aImage->FinishedSomeDecoding();
  }

  DecodeSomeOfImage(aImage);

  aImage->FinishedSomeDecoding();

  
  
  if (aImage->mDecoder &&
      !aImage->mError &&
      !aImage->IsDecodeFinished() &&
      aImage->mSourceData.Length() > aImage->mDecoder->BytesDecoded()) {
    RequestDecode(aImage);
  }
}

 void
DecodePool::StopDecoding(RasterImage* aImage)
{
  aImage->mDecodingMonitor.AssertCurrentThreadIn();

  
  
  aImage->mDecodeStatus = DecodeStatus::STOPPED;
}

nsresult
DecodePool::DecodeUntilSizeAvailable(RasterImage* aImage)
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter lock(aImage->mDecodingMonitor);

  
  if (aImage->mDecodeStatus == DecodeStatus::WORK_DONE) {
    nsresult rv = aImage->FinishedSomeDecoding();
    if (NS_FAILED(rv)) {
      aImage->DoError();
      return rv;
    }
  }

  nsresult rv = DecodeSomeOfImage(aImage, DecodeUntil::SIZE);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return aImage->FinishedSomeDecoding();
}

nsresult
DecodePool::DecodeSomeOfImage(RasterImage* aImage,
                              DecodeUntil aDecodeUntil ,
                              uint32_t bytesToDecode )
{
  MOZ_ASSERT(aImage->mInitialized, "Worker active for uninitialized container");
  aImage->mDecodingMonitor.AssertCurrentThreadIn();

  
  
  if (aImage->mError) {
    return NS_OK;
  }

  
  
  if (aImage->mPendingError) {
    return NS_OK;
  }

  
  
  if (!aImage->mDecoder || aImage->mDecoded) {
    return NS_OK;
  }

  nsRefPtr<Decoder> decoderKungFuDeathGrip = aImage->mDecoder;

  uint32_t maxBytes;
  if (aImage->mDecoder->IsSizeDecode()) {
    
    
    maxBytes = aImage->mSourceData.Length();
  } else {
    
    
    
    maxBytes = gfxPrefs::ImageMemDecodeBytesAtATime();
  }

  if (bytesToDecode == 0) {
    bytesToDecode = aImage->mSourceData.Length() - aImage->mDecoder->BytesDecoded();
  }

  TimeStamp deadline = TimeStamp::Now() +
                       TimeDuration::FromMilliseconds(gfxPrefs::ImageMemMaxMSBeforeYield());

  
  
  
  
  
  while (aImage->mSourceData.Length() > aImage->mDecoder->BytesDecoded() &&
         bytesToDecode > 0 &&
         !aImage->IsDecodeFinished() &&
         !(aDecodeUntil == DecodeUntil::SIZE && aImage->mHasSize)) {
    uint32_t chunkSize = min(bytesToDecode, maxBytes);
    nsresult rv = aImage->DecodeSomeData(chunkSize);
    if (NS_FAILED(rv)) {
      aImage->DoError();
      return rv;
    }

    bytesToDecode -= chunkSize;

    
    
    if (aDecodeUntil == DecodeUntil::TIME && TimeStamp::Now() >= deadline) {
      break;
    }
  }

  return NS_OK;
}

} 
} 
