






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
  Flattenable *flattenable = aParam.mFence.get();
  size_t nbytes = flattenable->getFlattenedSize();
  size_t nfds = flattenable->getFdCount();

  char data[nbytes];
  int fds[nfds];
  flattenable->flatten(data, nbytes, fds, nfds);

  aMsg->WriteSize(nbytes);
  aMsg->WriteSize(nfds);

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
  size_t nfds;
  const char* data;

  if (!aMsg->ReadSize(aIter, &nbytes) ||
      !aMsg->ReadSize(aIter, &nfds) ||
      !aMsg->ReadBytes(aIter, &data, nbytes)) {
    return false;
  }

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
  Flattenable *flattenable = buffer.get();

  if (NO_ERROR == flattenable->unflatten(data, nbytes, fds, nfds)) {
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

} 
} 
