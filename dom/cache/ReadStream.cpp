





#include "mozilla/dom/cache/ReadStream.h"

#include "mozilla/unused.h"
#include "mozilla/dom/cache/CacheStreamControlChild.h"
#include "mozilla/dom/cache/CacheStreamControlParent.h"
#include "mozilla/dom/cache/CacheTypes.h"
#include "mozilla/ipc/FileDescriptor.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/SnappyUncompressInputStream.h"
#include "nsIAsyncInputStream.h"
#include "nsTArray.h"

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::unused;
using mozilla::ipc::FileDescriptor;






class ReadStream::Inner final : public ReadStream::Controllable
{
public:
  Inner(StreamControl* aControl, const nsID& aId,
        nsIInputStream* aStream);

  void
  Serialize(CacheReadStreamOrVoid* aReadStreamOut);

  void
  Serialize(CacheReadStream* aReadStreamOut);

  
  virtual void
  CloseStream() override;

  virtual void
  CloseStreamWithoutReporting() override;

  virtual bool
  MatchId(const nsID& aId) const override;

  
  NS_METHOD
  Close();

  NS_METHOD
  Available(uint64_t *aNumAvailableOut);

  NS_METHOD
  Read(char *aBuf, uint32_t aCount, uint32_t *aNumReadOut);

  NS_METHOD
  ReadSegments(nsWriteSegmentFun aWriter, void *aClosure, uint32_t aCount,
               uint32_t *aNumReadOut);

  NS_METHOD
  IsNonBlocking(bool *aNonBlockingOut);

private:
  class NoteClosedRunnable;
  class ForgetRunnable;

  ~Inner();

  void
  NoteClosed();

  void
  Forget();

  void
  NoteClosedOnOwningThread();

  void
  ForgetOnOwningThread();

  
  
  
  
  StreamControl* mControl;

  const nsID mId;
  nsCOMPtr<nsIInputStream> mStream;
  nsCOMPtr<nsIInputStream> mSnappyStream;
  nsCOMPtr<nsIThread> mOwningThread;

  enum State
  {
    Open,
    Closed,
    NumStates
  };
  Atomic<State> mState;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(cache::ReadStream::Inner, override)
};







class ReadStream::Inner::NoteClosedRunnable final : public nsCancelableRunnable
{
public:
  explicit NoteClosedRunnable(ReadStream::Inner* aStream)
    : mStream(aStream)
  { }

  NS_IMETHOD Run()
  {
    mStream->NoteClosedOnOwningThread();
    mStream = nullptr;
    return NS_OK;
  }

  
  
  NS_IMETHOD Cancel()
  {
    Run();
    return NS_OK;
  }

private:
  ~NoteClosedRunnable() { }

  nsRefPtr<ReadStream::Inner> mStream;
};








class ReadStream::Inner::ForgetRunnable final : public nsCancelableRunnable
{
public:
  explicit ForgetRunnable(ReadStream::Inner* aStream)
    : mStream(aStream)
  { }

  NS_IMETHOD Run()
  {
    mStream->ForgetOnOwningThread();
    mStream = nullptr;
    return NS_OK;
  }

  
  
  NS_IMETHOD Cancel()
  {
    Run();
    return NS_OK;
  }

private:
  ~ForgetRunnable() { }

  nsRefPtr<ReadStream::Inner> mStream;
};



ReadStream::Inner::Inner(StreamControl* aControl, const nsID& aId,
                         nsIInputStream* aStream)
  : mControl(aControl)
  , mId(aId)
  , mStream(aStream)
  , mSnappyStream(new SnappyUncompressInputStream(aStream))
  , mOwningThread(NS_GetCurrentThread())
  , mState(Open)
{
  MOZ_ASSERT(mStream);
  MOZ_ASSERT(mControl);
  mControl->AddReadStream(this);
}

void
ReadStream::Inner::Serialize(CacheReadStreamOrVoid* aReadStreamOut)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mOwningThread);
  MOZ_ASSERT(aReadStreamOut);
  CacheReadStream stream;
  Serialize(&stream);
  *aReadStreamOut = stream;
}

void
ReadStream::Inner::Serialize(CacheReadStream* aReadStreamOut)
{
  MOZ_ASSERT(NS_GetCurrentThread() == mOwningThread);
  MOZ_ASSERT(aReadStreamOut);
  MOZ_ASSERT(mState == Open);
  MOZ_ASSERT(mControl);

  
  
  aReadStreamOut->pushStreamChild() = nullptr;
  aReadStreamOut->pushStreamParent() = nullptr;

  aReadStreamOut->id() = mId;
  mControl->SerializeControl(aReadStreamOut);

  nsAutoTArray<FileDescriptor, 4> fds;
  SerializeInputStream(mStream, aReadStreamOut->params(), fds);

  mControl->SerializeFds(aReadStreamOut, fds);

  
  
  Forget();
}

void
ReadStream::Inner::CloseStream()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mOwningThread);
  Close();
}

void
ReadStream::Inner::CloseStreamWithoutReporting()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mOwningThread);
  Forget();
}

bool
ReadStream::Inner::MatchId(const nsID& aId) const
{
  MOZ_ASSERT(NS_GetCurrentThread() == mOwningThread);
  return mId.Equals(aId);
}

NS_IMETHODIMP
ReadStream::Inner::Close()
{
  
  nsresult rv = mStream->Close();
  NoteClosed();
  return rv;
}

NS_IMETHODIMP
ReadStream::Inner::Available(uint64_t* aNumAvailableOut)
{
  
  nsresult rv = mSnappyStream->Available(aNumAvailableOut);

  if (NS_FAILED(rv)) {
    Close();
  }

  return rv;
}

NS_IMETHODIMP
ReadStream::Inner::Read(char* aBuf, uint32_t aCount, uint32_t* aNumReadOut)
{
  
  MOZ_ASSERT(aNumReadOut);

  nsresult rv = mSnappyStream->Read(aBuf, aCount, aNumReadOut);

  if ((NS_FAILED(rv) && rv != NS_BASE_STREAM_WOULD_BLOCK) ||
      *aNumReadOut == 0) {
    Close();
  }

  return rv;
}

NS_IMETHODIMP
ReadStream::Inner::ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                                uint32_t aCount, uint32_t* aNumReadOut)
{
  
  MOZ_ASSERT(aNumReadOut);

  nsresult rv = mSnappyStream->ReadSegments(aWriter, aClosure, aCount,
                                            aNumReadOut);

  if ((NS_FAILED(rv) && rv != NS_BASE_STREAM_WOULD_BLOCK &&
                        rv != NS_ERROR_NOT_IMPLEMENTED) || *aNumReadOut == 0) {
    Close();
  }

  return rv;
}

NS_IMETHODIMP
ReadStream::Inner::IsNonBlocking(bool* aNonBlockingOut)
{
  
  return mSnappyStream->IsNonBlocking(aNonBlockingOut);
}

ReadStream::Inner::~Inner()
{
  
  MOZ_ASSERT(mState == Closed);
  MOZ_ASSERT(!mControl);
}

void
ReadStream::Inner::NoteClosed()
{
  
  if (mState == Closed) {
    return;
  }

  if (NS_GetCurrentThread() == mOwningThread) {
    NoteClosedOnOwningThread();
    return;
  }

  nsCOMPtr<nsIRunnable> runnable = new NoteClosedRunnable(this);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    mOwningThread->Dispatch(runnable, nsIThread::DISPATCH_NORMAL)));
}

void
ReadStream::Inner::Forget()
{
  
  if (mState == Closed) {
    return;
  }

  if (NS_GetCurrentThread() == mOwningThread) {
    ForgetOnOwningThread();
    return;
  }

  nsCOMPtr<nsIRunnable> runnable = new ForgetRunnable(this);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    mOwningThread->Dispatch(runnable, nsIThread::DISPATCH_NORMAL)));
}

void
ReadStream::Inner::NoteClosedOnOwningThread()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mOwningThread);

  
  if (!mState.compareExchange(Open, Closed)) {
    return;
  }

  MOZ_ASSERT(mControl);
  mControl->NoteClosed(this, mId);
  mControl = nullptr;
}

void
ReadStream::Inner::ForgetOnOwningThread()
{
  MOZ_ASSERT(NS_GetCurrentThread() == mOwningThread);

  
  if (!mState.compareExchange(Open, Closed)) {
    return;
  }

  MOZ_ASSERT(mControl);
  mControl->ForgetReadStream(this);
  mControl = nullptr;
}



NS_IMPL_ISUPPORTS(cache::ReadStream, nsIInputStream, ReadStream);


already_AddRefed<ReadStream>
ReadStream::Create(const CacheReadStreamOrVoid& aReadStreamOrVoid)
{
  if (aReadStreamOrVoid.type() == CacheReadStreamOrVoid::Tvoid_t) {
    return nullptr;
  }

  return Create(aReadStreamOrVoid.get_CacheReadStream());
}


already_AddRefed<ReadStream>
ReadStream::Create(const CacheReadStream& aReadStream)
{
  
  
  
  if (!aReadStream.controlChild() && !aReadStream.controlParent()) {
    return nullptr;
  }

  MOZ_ASSERT(!aReadStream.pushStreamChild());
  MOZ_ASSERT(!aReadStream.pushStreamParent());

  
  
  StreamControl* control;
  if (aReadStream.controlChild()) {
    auto actor = static_cast<CacheStreamControlChild*>(aReadStream.controlChild());
    control = actor;
  } else {
    auto actor = static_cast<CacheStreamControlParent*>(aReadStream.controlParent());
    control = actor;
  }
  MOZ_ASSERT(control);

  nsAutoTArray<FileDescriptor, 4> fds;
  control->DeserializeFds(aReadStream, fds);

  nsCOMPtr<nsIInputStream> stream =
    DeserializeInputStream(aReadStream.params(), fds);
  MOZ_ASSERT(stream);

  
#ifdef DEBUG
  nsCOMPtr<nsIAsyncInputStream> asyncStream = do_QueryInterface(stream);
  MOZ_ASSERT(!asyncStream);
#endif

  nsRefPtr<Inner> inner = new Inner(control, aReadStream.id(), stream);
  nsRefPtr<ReadStream> ref = new ReadStream(inner);
  return ref.forget();
}


already_AddRefed<ReadStream>
ReadStream::Create(PCacheStreamControlParent* aControl, const nsID& aId,
                   nsIInputStream* aStream)
{
  MOZ_ASSERT(aControl);
  auto actor = static_cast<CacheStreamControlParent*>(aControl);
  nsRefPtr<Inner> inner = new Inner(actor, aId, aStream);
  nsRefPtr<ReadStream> ref = new ReadStream(inner);
  return ref.forget();
}

void
ReadStream::Serialize(CacheReadStreamOrVoid* aReadStreamOut)
{
  mInner->Serialize(aReadStreamOut);
}

void
ReadStream::Serialize(CacheReadStream* aReadStreamOut)
{
  mInner->Serialize(aReadStreamOut);
}

ReadStream::ReadStream(ReadStream::Inner* aInner)
  : mInner(aInner)
{
  MOZ_ASSERT(mInner);
}

ReadStream::~ReadStream()
{
  
  
  mInner->Close();
}

NS_IMETHODIMP
ReadStream::Close()
{
  return mInner->Close();
}

NS_IMETHODIMP
ReadStream::Available(uint64_t* aNumAvailableOut)
{
  return mInner->Available(aNumAvailableOut);
}

NS_IMETHODIMP
ReadStream::Read(char* aBuf, uint32_t aCount, uint32_t* aNumReadOut)
{
  return mInner->Read(aBuf, aCount, aNumReadOut);
}

NS_IMETHODIMP
ReadStream::ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                         uint32_t aCount, uint32_t* aNumReadOut)
{
  return mInner->ReadSegments(aWriter, aClosure, aCount, aNumReadOut);
}

NS_IMETHODIMP
ReadStream::IsNonBlocking(bool* aNonBlockingOut)
{
  return mInner->IsNonBlocking(aNonBlockingOut);
}

} 
} 
} 
