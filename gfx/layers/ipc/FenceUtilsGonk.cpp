






#include "GLContext.h"
#include "mozilla/unused.h"
#include "nsXULAppAPI.h"

#include "FenceUtilsGonk.h"

using namespace android;
using namespace mozilla::layers;

using base::FileDescriptor;

namespace IPC {

void
ParamTraits<FenceHandle>::Write(Message* aMsg,
                                const paramType& aParam)
{
#if ANDROID_VERSION >= 19
  sp<Fence> flattenable = aParam.mFence;
#else
  Flattenable *flattenable = aParam.mFence.get();
#endif
  size_t nbytes = flattenable->getFlattenedSize();
  size_t nfds = flattenable->getFdCount();
  char data[nbytes];
  int fds[nfds];

#if ANDROID_VERSION >= 19
  
  void *pdata = (void *)data;
  int *pfds = fds;

  flattenable->flatten(pdata, nbytes, pfds, nfds);

  
  
  
  
  nbytes = flattenable->getFlattenedSize();
  nfds = flattenable->getFdCount();
#else
  flattenable->flatten(data, nbytes, fds, nfds);
#endif
  aMsg->WriteSize(nbytes);
  aMsg->WriteSize(nfds);
  aMsg->WriteBytes(data, nbytes);
  for (size_t n = 0; n < nfds; ++n) {
    
    
    
    aMsg->WriteFileDescriptor(FileDescriptor(dup(fds[n]), true));
  }
}

bool
ParamTraits<FenceHandle>::Read(const Message* aMsg,
                               void** aIter, paramType* aResult)
{
  size_t nbytes;
  size_t nfds;
  const char* data;

  if (!aMsg->ReadSize(aIter, &nbytes) ||
      !aMsg->ReadSize(aIter, &nfds) ||
      !aMsg->ReadBytes(aIter, &data, nbytes)) {
    return false;
  }

  
  
  if (nfds > aMsg->num_fds()) {
    return false;
  }
  int fds[nfds];

  for (size_t n = 0; n < nfds; ++n) {
    FileDescriptor fd;
    if (!aMsg->ReadFileDescriptor(aIter, &fd)) {
      return false;
    }
    
    
    
    
    
    fds[n] = fd.fd;
  }

  sp<Fence> buffer(new Fence());
#if ANDROID_VERSION >= 19
  
  void const *pdata = (void const *)data;
  int const *pfds = fds;

  if (NO_ERROR == buffer->unflatten(pdata, nbytes, pfds, nfds)) {
#else
  Flattenable *flattenable = buffer.get();

  if (NO_ERROR == flattenable->unflatten(data, nbytes, fds, nfds)) {
#endif
    aResult->mFence = buffer;
    return true;
  }
  return false;
}

} 

namespace mozilla {
namespace layers {

FenceHandle::FenceHandle()
  : mFence(android::Fence::NO_FENCE)
{
}

FenceHandle::FenceHandle(const sp<Fence>& aFence)
  : mFence(aFence)
{
}

void
FenceHandle::Merge(const FenceHandle& aFenceHandle)
{
  if (!aFenceHandle.IsValid()) {
    return;
  }

  if (!IsValid()) {
    mFence = aFenceHandle.mFence;
  } else {
    android::sp<android::Fence> mergedFence = android::Fence::merge(
                  android::String8::format("FenceHandle"),
                  mFence, aFenceHandle.mFence);
    if (!mergedFence.get()) {
      
      
      
      mFence = aFenceHandle.mFence;
      return;
    }
    mFence = mergedFence;
  }
}

} 
} 
