





#include "mozilla/dom/cache/CachePushStreamChild.h"

#include "mozilla/unused.h"
#include "nsIAsyncInputStream.h"
#include "nsICancelableRunnable.h"
#include "nsIThread.h"
#include "nsStreamUtils.h"

namespace mozilla {
namespace dom {
namespace cache {

class CachePushStreamChild::Callback final : public nsIInputStreamCallback
                                           , public nsICancelableRunnable
{
public:
  explicit Callback(CachePushStreamChild* aActor)
    : mActor(aActor)
    , mOwningThread(NS_GetCurrentThread())
  {
    MOZ_ASSERT(mActor);
  }

  NS_IMETHOD
  OnInputStreamReady(nsIAsyncInputStream* aStream) override
  {
    
    if (mOwningThread == NS_GetCurrentThread()) {
      return Run();
    }

    
    
    
    nsresult rv = mOwningThread->Dispatch(this, nsIThread::DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to dispatch stream readable event to owning thread");
    }

    return NS_OK;
  }

  NS_IMETHOD
  Run() override
  {
    MOZ_ASSERT(mOwningThread == NS_GetCurrentThread());
    if (mActor) {
      mActor->OnStreamReady(this);
    }
    return NS_OK;
  }

  NS_IMETHOD
  Cancel() override
  {
    
    
    
    return NS_OK;
  }

  void
  ClearActor()
  {
    MOZ_ASSERT(mOwningThread == NS_GetCurrentThread());
    MOZ_ASSERT(mActor);
    mActor = nullptr;
  }

private:
  ~Callback()
  {
    

    
    MOZ_ASSERT(!mActor);
  }

  CachePushStreamChild* mActor;
  nsCOMPtr<nsIThread> mOwningThread;

  NS_DECL_THREADSAFE_ISUPPORTS
};

NS_IMPL_ISUPPORTS(CachePushStreamChild::Callback, nsIInputStreamCallback,
                                                  nsIRunnable,
                                                  nsICancelableRunnable);

CachePushStreamChild::CachePushStreamChild(Feature* aFeature,
                                           nsIAsyncInputStream* aStream)
  : mStream(aStream)
  , mClosed(false)
{
  MOZ_ASSERT(mStream);
  MOZ_ASSERT_IF(!NS_IsMainThread(), aFeature);
  SetFeature(aFeature);
}

CachePushStreamChild::~CachePushStreamChild()
{
  NS_ASSERT_OWNINGTHREAD(CachePushStreamChild);
  MOZ_ASSERT(mClosed);
  MOZ_ASSERT(!mCallback);
}

void
CachePushStreamChild::Start()
{
  DoRead();
}

void
CachePushStreamChild::StartDestroy()
{
  
  
  
}

void
CachePushStreamChild::ActorDestroy(ActorDestroyReason aReason)
{
  NS_ASSERT_OWNINGTHREAD(CachePushStreamChild);

  
  
  
  if (!mClosed) {
    mStream->CloseWithStatus(NS_ERROR_ABORT);
    mClosed = true;
  }

  if (mCallback) {
    mCallback->ClearActor();
    mCallback = nullptr;
  }

  RemoveFeature();
}

void
CachePushStreamChild::DoRead()
{
  NS_ASSERT_OWNINGTHREAD(CachePushStreamChild);
  MOZ_ASSERT(!mClosed);
  MOZ_ASSERT(!mCallback);

  
  
  
  
  static const uint64_t kMaxBytesPerMessage = 32 * 1024;
  static_assert(kMaxBytesPerMessage <= static_cast<uint64_t>(UINT32_MAX),
                "kMaxBytesPerMessage must cleanly cast to uint32_t");

  while (!mClosed) {
    
    
    
    
    nsCString buffer;

    uint64_t available = 0;
    nsresult rv = mStream->Available(&available);
    if (NS_FAILED(rv)) {
      OnEnd(rv);
      return;
    }

    if (available == 0) {
      Wait();
      return;
    }

    uint32_t expectedBytes =
      static_cast<uint32_t>(std::min(available, kMaxBytesPerMessage));

    buffer.SetLength(expectedBytes);

    uint32_t bytesRead = 0;
    rv = mStream->Read(buffer.BeginWriting(), buffer.Length(), &bytesRead);
    buffer.SetLength(bytesRead);

    
    if (!buffer.IsEmpty()) {
      unused << SendBuffer(buffer);
    }

    if (rv == NS_BASE_STREAM_WOULD_BLOCK) {
      Wait();
      return;
    }

    
    if (NS_FAILED(rv) || buffer.IsEmpty()) {
      OnEnd(rv);
      return;
    }
  }
}

void
CachePushStreamChild::Wait()
{
  NS_ASSERT_OWNINGTHREAD(CachePushStreamChild);
  MOZ_ASSERT(!mClosed);
  MOZ_ASSERT(!mCallback);

  
  
  mCallback = new Callback(this);
  nsresult rv = mStream->AsyncWait(mCallback, 0, 0, nullptr);
  if (NS_FAILED(rv)) {
    OnEnd(rv);
    return;
  }
}

void
CachePushStreamChild::OnStreamReady(Callback* aCallback)
{
  NS_ASSERT_OWNINGTHREAD(CachePushStreamChild);
  MOZ_ASSERT(mCallback);
  MOZ_ASSERT(aCallback == mCallback);
  mCallback->ClearActor();
  mCallback = nullptr;
  DoRead();
}

void
CachePushStreamChild::OnEnd(nsresult aRv)
{
  NS_ASSERT_OWNINGTHREAD(CachePushStreamChild);
  MOZ_ASSERT(aRv != NS_BASE_STREAM_WOULD_BLOCK);

  if (mClosed) {
    return;
  }

  mClosed = true;

  mStream->CloseWithStatus(aRv);

  if (aRv == NS_BASE_STREAM_CLOSED) {
    aRv = NS_OK;
  }

  
  unused << SendClose(aRv);
}

} 
} 
} 
