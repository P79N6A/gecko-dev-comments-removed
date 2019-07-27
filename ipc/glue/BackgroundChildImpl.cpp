



#include "BackgroundChildImpl.h"

#include "FileDescriptorSetChild.h"
#include "mozilla/Assertions.h"
#include "mozilla/dom/PBlobChild.h"
#include "mozilla/dom/indexedDB/PBackgroundIDBFactoryChild.h"
#include "mozilla/dom/ipc/BlobChild.h"
#include "mozilla/ipc/PBackgroundTestChild.h"
#include "nsTraceRefcnt.h"

namespace {

class TestChild MOZ_FINAL : public mozilla::ipc::PBackgroundTestChild
{
  friend class mozilla::ipc::BackgroundChildImpl;

  nsCString mTestArg;

  explicit TestChild(const nsCString& aTestArg)
    : mTestArg(aTestArg)
  {
    MOZ_COUNT_CTOR(TestChild);
  }

protected:
  ~TestChild()
  {
    MOZ_COUNT_DTOR(TestChild);
  }

public:
  virtual bool
  Recv__delete__(const nsCString& aTestArg) MOZ_OVERRIDE;
};

} 

namespace mozilla {
namespace ipc {





BackgroundChildImpl::
ThreadLocal::ThreadLocal()
  : mCurrentTransaction(nullptr)
#ifdef MOZ_ENABLE_PROFILER_SPS
  , mNextTransactionSerialNumber(1)
  , mNextRequestSerialNumber(1)
#endif
{
  
  MOZ_COUNT_CTOR(mozilla::ipc::BackgroundChildImpl::ThreadLocal);
}

BackgroundChildImpl::
ThreadLocal::~ThreadLocal()
{
  
  MOZ_COUNT_DTOR(mozilla::ipc::BackgroundChildImpl::ThreadLocal);
}





BackgroundChildImpl::BackgroundChildImpl()
{
  
  MOZ_COUNT_CTOR(mozilla::ipc::BackgroundChildImpl);
}

BackgroundChildImpl::~BackgroundChildImpl()
{
  
  MOZ_COUNT_DTOR(mozilla::ipc::BackgroundChildImpl);
}

void
BackgroundChildImpl::ProcessingError(Result aWhat)
{
  

  nsAutoCString abortMessage;

  switch (aWhat) {

#define HANDLE_CASE(_result)                                                   \
    case _result:                                                              \
      abortMessage.AssignLiteral(#_result);                                    \
      break

    HANDLE_CASE(MsgDropped);
    HANDLE_CASE(MsgNotKnown);
    HANDLE_CASE(MsgNotAllowed);
    HANDLE_CASE(MsgPayloadError);
    HANDLE_CASE(MsgProcessingError);
    HANDLE_CASE(MsgRouteError);
    HANDLE_CASE(MsgValueError);

#undef HANDLE_CASE

    default:
      MOZ_CRASH("Unknown error code!");
  }

  
  
  MOZ_ReportCrash(abortMessage.get(), __FILE__, __LINE__); MOZ_REALLY_CRASH();
}

void
BackgroundChildImpl::ActorDestroy(ActorDestroyReason aWhy)
{
  
}

PBackgroundTestChild*
BackgroundChildImpl::AllocPBackgroundTestChild(const nsCString& aTestArg)
{
  return new TestChild(aTestArg);
}

bool
BackgroundChildImpl::DeallocPBackgroundTestChild(PBackgroundTestChild* aActor)
{
  MOZ_ASSERT(aActor);

  delete static_cast<TestChild*>(aActor);
  return true;
}

BackgroundChildImpl::PBackgroundIDBFactoryChild*
BackgroundChildImpl::AllocPBackgroundIDBFactoryChild(
                                      const OptionalWindowId& aOptionalWindowId)
{
  MOZ_CRASH("PBackgroundIDBFactoryChild actors should be manually "
            "constructed!");
}

bool
BackgroundChildImpl::DeallocPBackgroundIDBFactoryChild(
                                             PBackgroundIDBFactoryChild* aActor)
{
  MOZ_ASSERT(aActor);

  delete aActor;
  return true;
}

auto
BackgroundChildImpl::AllocPBlobChild(const BlobConstructorParams& aParams)
  -> PBlobChild*
{
  MOZ_ASSERT(aParams.type() != BlobConstructorParams::T__None);

  return mozilla::dom::BlobChild::Create(this, aParams);
}

bool
BackgroundChildImpl::DeallocPBlobChild(PBlobChild* aActor)
{
  MOZ_ASSERT(aActor);

  mozilla::dom::BlobChild::Destroy(aActor);
  return true;
}

PFileDescriptorSetChild*
BackgroundChildImpl::AllocPFileDescriptorSetChild(
                                          const FileDescriptor& aFileDescriptor)
{
  return new FileDescriptorSetChild(aFileDescriptor);
}

bool
BackgroundChildImpl::DeallocPFileDescriptorSetChild(
                                                PFileDescriptorSetChild* aActor)
{
  MOZ_ASSERT(aActor);

  delete static_cast<FileDescriptorSetChild*>(aActor);
  return true;
}

} 
} 

bool
TestChild::Recv__delete__(const nsCString& aTestArg)
{
  MOZ_RELEASE_ASSERT(aTestArg == mTestArg,
                     "BackgroundTest message was corrupted!");

  return true;
}
