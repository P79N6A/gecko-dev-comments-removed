





#include "mozilla/dom/cache/ReadStream.h"

#include "mozilla/unused.h"
#include "mozilla/dom/cache/CacheStreamControlChild.h"
#include "mozilla/dom/cache/CacheStreamControlParent.h"
#include "mozilla/dom/cache/PCacheStreamControlChild.h"
#include "mozilla/dom/cache/PCacheStreamControlParent.h"
#include "mozilla/dom/cache/PCacheTypes.h"
#include "mozilla/ipc/FileDescriptor.h"
#include "mozilla/ipc/FileDescriptorSetChild.h"
#include "mozilla/ipc/FileDescriptorSetParent.h"
#include "mozilla/ipc/InputStreamParams.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "mozilla/ipc/PBackgroundParent.h"
#include "mozilla/ipc/PFileDescriptorSetChild.h"
#include "mozilla/ipc/PFileDescriptorSetParent.h"
#include "mozilla/SnappyUncompressInputStream.h"
#include "nsIAsyncInputStream.h"
#include "nsTArray.h"

namespace {

using mozilla::unused;
using mozilla::void_t;
using mozilla::dom::cache::CacheStreamControlChild;
using mozilla::dom::cache::CacheStreamControlParent;
using mozilla::dom::cache::PCacheReadStream;
using mozilla::dom::cache::PCacheStreamControlChild;
using mozilla::dom::cache::PCacheStreamControlParent;
using mozilla::dom::cache::ReadStream;
using mozilla::ipc::FileDescriptor;
using mozilla::ipc::PFileDescriptorSetChild;
using mozilla::ipc::PFileDescriptorSetParent;









class ReadStreamChild MOZ_FINAL : public ReadStream
{
public:
  ReadStreamChild(PCacheStreamControlChild* aControl, const nsID& aId,
                  nsIInputStream* aStream)
    : ReadStream(aId, aStream)
    , mControl(static_cast<CacheStreamControlChild*>(aControl))
  {
    MOZ_ASSERT(mControl);
    mControl->AddListener(this);
  }

  virtual ~ReadStreamChild()
  {
    NS_ASSERT_OWNINGTHREAD(ReadStream);

    NoteClosed();
  }

  virtual void NoteClosedOnOwningThread() MOZ_OVERRIDE
  {
    NS_ASSERT_OWNINGTHREAD(ReadStream);

    if (mClosed) {
      return;
    }

    mClosed = true;
    mControl->RemoveListener(this);
    mControl->NoteClosed(mId);
  }

  virtual void ForgetOnOwningThread() MOZ_OVERRIDE
  {
    NS_ASSERT_OWNINGTHREAD(ReadStream);

    if (mClosed) {
      return;
    }

    mClosed = true;
    mControl->RemoveListener(this);
  }

  virtual void SerializeControl(PCacheReadStream* aReadStreamOut) MOZ_OVERRIDE
  {
    MOZ_ASSERT(aReadStreamOut);
    MOZ_ASSERT(!mClosed);
    aReadStreamOut->controlParent() = nullptr;
    aReadStreamOut->controlChild() = mControl;
  }

  virtual void
  SerializeFds(PCacheReadStream* aReadStreamOut,
               const nsTArray<FileDescriptor>& fds) MOZ_OVERRIDE
  {
    MOZ_ASSERT(!mClosed);
    PFileDescriptorSetChild* fdSet = nullptr;
    if (!fds.IsEmpty()) {
      fdSet = mControl->Manager()->SendPFileDescriptorSetConstructor(fds[0]);
      for (uint32_t i = 1; i < fds.Length(); ++i) {
        unused << fdSet->SendAddFileDescriptor(fds[i]);
      }
    }

    if (fdSet) {
      aReadStreamOut->fds() = fdSet;
    } else {
      aReadStreamOut->fds() = void_t();
    }
  }

private:
  CacheStreamControlChild* mControl;
};



class ReadStreamParent MOZ_FINAL : public ReadStream
{
public:
  ReadStreamParent(PCacheStreamControlParent* aControl, const nsID& aId,
                  nsIInputStream* aStream)
    : ReadStream(aId, aStream)
    , mControl(static_cast<CacheStreamControlParent*>(aControl))
  {
    MOZ_ASSERT(mControl);
    mControl->AddListener(this);
  }

  virtual ~ReadStreamParent()
  {
    NS_ASSERT_OWNINGTHREAD(ReadStream);

    NoteClosed();
  }

  virtual void NoteClosedOnOwningThread() MOZ_OVERRIDE
  {
    NS_ASSERT_OWNINGTHREAD(ReadStream);

    if (mClosed) {
      return;
    }

    mClosed = true;
    mControl->RemoveListener(this);
    
    mControl->RecvNoteClosed(mId);
    mControl = nullptr;
  }

  virtual void ForgetOnOwningThread() MOZ_OVERRIDE
  {
    NS_ASSERT_OWNINGTHREAD(ReadStream);

    if (mClosed) {
      return;
    }

    mClosed = true;
    
    mControl->RemoveListener(this);
    mControl = nullptr;
  }

  virtual void SerializeControl(PCacheReadStream* aReadStreamOut) MOZ_OVERRIDE
  {
    MOZ_ASSERT(aReadStreamOut);
    MOZ_ASSERT(!mClosed);
    MOZ_ASSERT(mControl);
    aReadStreamOut->controlChild() = nullptr;
    aReadStreamOut->controlParent() = mControl;
  }

  virtual void
  SerializeFds(PCacheReadStream* aReadStreamOut,
               const nsTArray<FileDescriptor>& fds) MOZ_OVERRIDE
  {
    MOZ_ASSERT(!mClosed);
    MOZ_ASSERT(mControl);
    PFileDescriptorSetParent* fdSet = nullptr;
    if (!fds.IsEmpty()) {
      fdSet = mControl->Manager()->SendPFileDescriptorSetConstructor(fds[0]);
      for (uint32_t i = 1; i < fds.Length(); ++i) {
        unused << fdSet->SendAddFileDescriptor(fds[i]);
      }
    }

    if (fdSet) {
      aReadStreamOut->fds() = fdSet;
    } else {
      aReadStreamOut->fds() = void_t();
    }
  }

private:
  CacheStreamControlParent* mControl;
};



} 

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::unused;
using mozilla::ipc::FileDescriptor;
using mozilla::ipc::FileDescriptorSetChild;
using mozilla::ipc::FileDescriptorSetParent;
using mozilla::ipc::InputStreamParams;
using mozilla::ipc::OptionalFileDescriptorSet;
using mozilla::ipc::PFileDescriptorSetChild;





class ReadStream::NoteClosedRunnable MOZ_FINAL : public nsCancelableRunnable
{
public:
  explicit NoteClosedRunnable(ReadStream* aStream)
    : mStream(aStream)
  { }

  NS_IMETHOD Run()
  {
    mStream->NoteClosedOnOwningThread();
    return NS_OK;
  }

  
  
  NS_IMETHOD Cancel()
  {
    Run();
    return NS_OK;
  }

private:
  ~NoteClosedRunnable() { }

  nsRefPtr<ReadStream> mStream;
};






class ReadStream::ForgetRunnable MOZ_FINAL : public nsCancelableRunnable
{
public:
  explicit ForgetRunnable(ReadStream* aStream)
    : mStream(aStream)
  { }

  NS_IMETHOD Run()
  {
    mStream->ForgetOnOwningThread();
    return NS_OK;
  }

  
  
  NS_IMETHOD Cancel()
  {
    Run();
    return NS_OK;
  }

private:
  ~ForgetRunnable() { }

  nsRefPtr<ReadStream> mStream;
};

NS_IMPL_ISUPPORTS(mozilla::dom::cache::ReadStream, nsIInputStream,
                                                   ReadStream);


already_AddRefed<ReadStream>
ReadStream::Create(const PCacheReadStreamOrVoid& aReadStreamOrVoid)
{
  if (aReadStreamOrVoid.type() == PCacheReadStreamOrVoid::Tvoid_t) {
    return nullptr;
  }

  return Create(aReadStreamOrVoid.get_PCacheReadStream());
}


already_AddRefed<ReadStream>
ReadStream::Create(const PCacheReadStream& aReadStream)
{
  
  
  
  if (!aReadStream.controlChild() && !aReadStream.controlParent()) {
    return nullptr;
  }

  nsAutoTArray<FileDescriptor, 4> fds;
  if (aReadStream.fds().type() ==
      OptionalFileDescriptorSet::TPFileDescriptorSetChild) {

    FileDescriptorSetChild* fdSetActor =
      static_cast<FileDescriptorSetChild*>(aReadStream.fds().get_PFileDescriptorSetChild());
    MOZ_ASSERT(fdSetActor);

    fdSetActor->ForgetFileDescriptors(fds);
    MOZ_ASSERT(!fds.IsEmpty());

    unused << fdSetActor->Send__delete__(fdSetActor);
  } else if (aReadStream.fds().type() ==
      OptionalFileDescriptorSet::TPFileDescriptorSetParent) {

    FileDescriptorSetParent* fdSetActor =
      static_cast<FileDescriptorSetParent*>(aReadStream.fds().get_PFileDescriptorSetParent());
    MOZ_ASSERT(fdSetActor);

    fdSetActor->ForgetFileDescriptors(fds);
    MOZ_ASSERT(!fds.IsEmpty());

    if (!fdSetActor->Send__delete__(fdSetActor)) {
      
      NS_WARNING("Cache failed to delete fd set actor.");
    }
  }

  nsCOMPtr<nsIInputStream> stream =
    DeserializeInputStream(aReadStream.params(), fds);
  MOZ_ASSERT(stream);

  
#ifdef DEBUG
  nsCOMPtr<nsIAsyncInputStream> asyncStream = do_QueryInterface(stream);
  MOZ_ASSERT(!asyncStream);
#endif

  nsRefPtr<ReadStream> ref;

  if (aReadStream.controlChild()) {
    ref = new ReadStreamChild(aReadStream.controlChild(), aReadStream.id(),
                              stream);
  } else {
    ref = new ReadStreamParent(aReadStream.controlParent(), aReadStream.id(),
                               stream);
  }

  return ref.forget();
}


already_AddRefed<ReadStream>
ReadStream::Create(PCacheStreamControlParent* aControl, const nsID& aId,
                   nsIInputStream* aStream)
{
  nsRefPtr<ReadStream> ref = new ReadStreamParent(aControl, aId, aStream);
  return ref.forget();
}

void
ReadStream::Serialize(PCacheReadStreamOrVoid* aReadStreamOut)
{
  MOZ_ASSERT(aReadStreamOut);
  PCacheReadStream stream;
  Serialize(&stream);
  *aReadStreamOut = stream;
}

void
ReadStream::Serialize(PCacheReadStream* aReadStreamOut)
{
  MOZ_ASSERT(aReadStreamOut);
  MOZ_ASSERT(!mClosed);

  aReadStreamOut->id() = mId;
  SerializeControl(aReadStreamOut);

  nsAutoTArray<FileDescriptor, 4> fds;
  SerializeInputStream(mStream, aReadStreamOut->params(), fds);

  SerializeFds(aReadStreamOut, fds);

  
  
  Forget();
}

void
ReadStream::CloseStream()
{
  Close();
}

void
ReadStream::CloseStreamWithoutReporting()
{
  Forget();
}

bool
ReadStream::MatchId(const nsID& aId) const
{
  return mId.Equals(aId);
}

ReadStream::ReadStream(const nsID& aId, nsIInputStream* aStream)
  : mId(aId)
  , mStream(aStream)
  , mSnappyStream(new SnappyUncompressInputStream(aStream))
  , mOwningThread(NS_GetCurrentThread())
  , mClosed(false)
{
  MOZ_ASSERT(mStream);
}

ReadStream::~ReadStream()
{
  NS_ASSERT_OWNINGTHREAD(ReadStream);

  
  
  
  MOZ_ASSERT(mClosed);
}

void
ReadStream::NoteClosed()
{
  if (mClosed) {
    return;
  }

  if (NS_GetCurrentThread() == mOwningThread) {
    NoteClosedOnOwningThread();
    return;
  }

  nsCOMPtr<nsIRunnable> runnable = new NoteClosedRunnable(this);
  nsresult rv = mOwningThread->Dispatch(runnable, nsIThread::DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch Cache ReadStream NoteClosed() runnable.");
  }
}

void
ReadStream::Forget()
{
  if (mClosed) {
    return;
  }

  if (NS_GetCurrentThread() == mOwningThread) {
    ForgetOnOwningThread();
    return;
  }

  nsCOMPtr<nsIRunnable> runnable = new ForgetRunnable(this);
  nsresult rv = mOwningThread->Dispatch(runnable, nsIThread::DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to dispatch Cache ReadStream Forget() runnable.");
  }
}

NS_IMETHODIMP
ReadStream::Close()
{
  NoteClosed();
  return mStream->Close();
}

NS_IMETHODIMP
ReadStream::Available(uint64_t* aNumAvailableOut)
{
  nsresult rv = mSnappyStream->Available(aNumAvailableOut);

  if (NS_FAILED(rv)) {
    NoteClosed();
  }

  return rv;
}

NS_IMETHODIMP
ReadStream::Read(char* aBuf, uint32_t aCount, uint32_t* aNumReadOut)
{
  MOZ_ASSERT(aNumReadOut);

  nsresult rv = mSnappyStream->Read(aBuf, aCount, aNumReadOut);

  
  

  if ((NS_FAILED(rv) && rv != NS_BASE_STREAM_WOULD_BLOCK) ||
      *aNumReadOut == 0) {
    NoteClosed();
  }

  return rv;
}

NS_IMETHODIMP
ReadStream::ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                         uint32_t aCount, uint32_t* aNumReadOut)
{
  MOZ_ASSERT(aNumReadOut);

  nsresult rv = mSnappyStream->ReadSegments(aWriter, aClosure, aCount,
                                            aNumReadOut);

  
  

  if ((NS_FAILED(rv) && rv != NS_BASE_STREAM_WOULD_BLOCK &&
                        rv != NS_ERROR_NOT_IMPLEMENTED) || *aNumReadOut == 0) {
    NoteClosed();
  }

  return rv;
}

NS_IMETHODIMP
ReadStream::IsNonBlocking(bool* aNonBlockingOut)
{
  return mSnappyStream->IsNonBlocking(aNonBlockingOut);
}

} 
} 
} 
