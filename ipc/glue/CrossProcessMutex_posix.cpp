




#include "CrossProcessMutex.h"
#include "mozilla/unused.h"
#include "nsDebug.h"
#include "nsISupportsImpl.h"

#ifdef OS_MACOSX
#include "nsCocoaFeatures.h"
#endif

namespace {

struct MutexData {
  pthread_mutex_t mMutex;
  mozilla::Atomic<int32_t> mCount;
};

}

namespace mozilla {

static void
InitMutex(pthread_mutex_t* mMutex)
{
  pthread_mutexattr_t mutexAttributes;
  pthread_mutexattr_init(&mutexAttributes);
  
  if (pthread_mutexattr_settype(&mutexAttributes, PTHREAD_MUTEX_RECURSIVE)) {
    MOZ_CRASH();
  }
  if (pthread_mutexattr_setpshared(&mutexAttributes, PTHREAD_PROCESS_SHARED)) {
    MOZ_CRASH();
  }

  if (pthread_mutex_init(mMutex, &mutexAttributes)) {
    MOZ_CRASH();
  }
}

CrossProcessMutex::CrossProcessMutex(const char*)
    : mMutex(nullptr)
    , mCount(nullptr)
{
#ifdef OS_MACOSX
  if (!nsCocoaFeatures::OnLionOrLater()) {
    
    
    
    MOZ_CRASH();
  }
#endif

  mSharedBuffer = new ipc::SharedMemoryBasic;
  if (!mSharedBuffer->Create(sizeof(MutexData))) {
    MOZ_CRASH();
  }

  if (!mSharedBuffer->Map(sizeof(MutexData))) {
    MOZ_CRASH();
  }

  MutexData* data = static_cast<MutexData*>(mSharedBuffer->memory());

  if (!data) {
    MOZ_CRASH();
  }

  mMutex = &(data->mMutex);
  mCount = &(data->mCount);

  *mCount = 1;
  InitMutex(mMutex);

  MOZ_COUNT_CTOR(CrossProcessMutex);
}

CrossProcessMutex::CrossProcessMutex(CrossProcessMutexHandle aHandle)
    : mMutex(nullptr)
    , mCount(nullptr)
{
  if (!ipc::SharedMemoryBasic::IsHandleValid(aHandle)) {
    MOZ_CRASH();
  }

  mSharedBuffer = new ipc::SharedMemoryBasic(aHandle);

  if (!mSharedBuffer->Map(sizeof(MutexData))) {
    MOZ_CRASH();
  }

  MutexData* data = static_cast<MutexData*>(mSharedBuffer->memory());

  if (!data) {
    MOZ_CRASH();
  }

  mMutex = &(data->mMutex);
  mCount = &(data->mCount);
  int32_t count = (*mCount)++;

  if (count == 0) {
    
    
    InitMutex(mMutex);
  }

  MOZ_COUNT_CTOR(CrossProcessMutex);
}

CrossProcessMutex::~CrossProcessMutex()
{
  int32_t count = --(*mCount);

  if (count == 0) {
    
    unused << pthread_mutex_destroy(mMutex);
  }

  MOZ_COUNT_DTOR(CrossProcessMutex);
}

void
CrossProcessMutex::Lock()
{
  MOZ_ASSERT(*mCount > 0, "Attempting to lock mutex with zero ref count");
  pthread_mutex_lock(mMutex);
}

void
CrossProcessMutex::Unlock()
{
  MOZ_ASSERT(*mCount > 0, "Attempting to unlock mutex with zero ref count");
  pthread_mutex_unlock(mMutex);
}

CrossProcessMutexHandle
CrossProcessMutex::ShareToProcess(base::ProcessId aTargetPid)
{
  CrossProcessMutexHandle result = ipc::SharedMemoryBasic::NULLHandle();

  if (mSharedBuffer && !mSharedBuffer->ShareToProcess(aTargetPid, &result)) {
    MOZ_CRASH();
  }

  return result;
}

}
