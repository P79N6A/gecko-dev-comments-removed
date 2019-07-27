






#include "GLContext.h"
#include "mozilla/unused.h"
#include "nsXULAppAPI.h"

#include "FenceUtilsGonk.h"

using namespace android;
using namespace base;
using namespace mozilla::layers;

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
  aMsg->WriteBytes(data, nbytes);
  for (size_t n = 0; n < nfds; ++n) {
    
    
    
    aMsg->WriteFileDescriptor(FileDescriptor(fds[n], false));
  }
}

bool
ParamTraits<FenceHandle>::Read(const Message* aMsg,
                               void** aIter, paramType* aResult)
{
  size_t nbytes;
  const char* data;

  if (!aMsg->ReadSize(aIter, &nbytes) ||
      !aMsg->ReadBytes(aIter, &data, nbytes)) {
    return false;
  }

  size_t nfds = aMsg->num_fds();
  int fds[nfds];

  for (size_t n = 0; n < nfds; ++n) {
    FileDescriptor fd;
    if (!aMsg->ReadFileDescriptor(aIter, &fd)) {
      return false;
    }
    
    
    
    
    
    bool sameProcess = (XRE_GetProcessType() == GeckoProcessType_Default);
    int dupFd = sameProcess ? dup(fd.fd) : fd.fd;
    fds[n] = dupFd;
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

void
ParamTraits<FenceHandleFromChild>::Write(Message* aMsg,
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
  aMsg->WriteBytes(data, nbytes);
  for (size_t n = 0; n < nfds; ++n) {
    
    
    
    
    
    bool sameProcess = (XRE_GetProcessType() == GeckoProcessType_Default);
    int dupFd = sameProcess ? dup(fds[n]) : fds[n];
    

    
    
    
    aMsg->WriteFileDescriptor(FileDescriptor(dupFd, false));
  }
}

bool
ParamTraits<FenceHandleFromChild>::Read(const Message* aMsg,
                                        void** aIter, paramType* aResult)
{
  size_t nbytes;
  const char* data;

  if (!aMsg->ReadSize(aIter, &nbytes) ||
      !aMsg->ReadBytes(aIter, &data, nbytes)) {
    return false;
  }

  size_t nfds = aMsg->num_fds();
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

FenceHandle::FenceHandle(const sp<Fence>& aFence)
  : mFence(aFence)
{
}

FenceHandle::FenceHandle(const FenceHandleFromChild& aFenceHandle) {
  mFence = aFenceHandle.mFence;
}

FenceHandleFromChild::FenceHandleFromChild(const sp<Fence>& aFence)
  : mFence(aFence)
{
}

} 
} 
