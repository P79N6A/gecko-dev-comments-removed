




#include "CrossProcessMutex.h"
#include "mozilla/unused.h"
#include "nsDebug.h"
#include "nsTraceRefcnt.h"

namespace {

struct MutexData {
  pthread_mutex_t mMutex;
  mozilla::Atomic<int32_t> mCount;
};

}

namespace mozilla {

CrossProcessMutex::CrossProcessMutex(const char*)
    : mSharedBuffer(nullptr)
    , mMutex(nullptr)
    , mCount(nullptr)
{
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

  MOZ_COUNT_CTOR(CrossProcessMutex);
}

CrossProcessMutex::CrossProcessMutex(CrossProcessMutexHandle aHandle)
    : mSharedBuffer(nullptr)
    , mMutex(nullptr)
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
  (*mCount)++;

  MOZ_COUNT_CTOR(CrossProcessMutex);
}

CrossProcessMutex::~CrossProcessMutex()
{
  int32_t count = --(*mCount);

  if (count == 0) {
    
    unused << pthread_mutex_destroy(mMutex);
  }

  delete mSharedBuffer;
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
CrossProcessMutex::ShareToProcess(base::ProcessHandle aHandle)
{
  CrossProcessMutexHandle result = ipc::SharedMemoryBasic::NULLHandle();

  if (mSharedBuffer && !mSharedBuffer->ShareToProcess(aHandle, &result)) {
    MOZ_CRASH();
  }

  return result;
}

}
