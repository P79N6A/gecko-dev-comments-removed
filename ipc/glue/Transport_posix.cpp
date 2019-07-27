






#include <unistd.h>

#include <string>

#include "chrome/common/child_process_info.h"

#include "mozilla/ipc/Transport.h"
#include "mozilla/ipc/FileDescriptor.h"

using namespace std;

using base::ProcessHandle;

namespace mozilla {
namespace ipc {

bool
CreateTransport(base::ProcessId ,
                TransportDescriptor* aOne, TransportDescriptor* aTwo)
{
  wstring id = IPC::Channel::GenerateVerifiedChannelID(std::wstring());
  
  Transport t(id, Transport::MODE_SERVER, nullptr);
  int fd1 = t.GetFileDescriptor();
  int fd2, dontcare;
  t.GetClientFileDescriptorMapping(&fd2, &dontcare);
  if (fd1 < 0 || fd2 < 0) {
    return false;
  }

  
  
  fd1 = dup(fd1);
  fd2 = dup(fd2);
  if (fd1 < 0 || fd2 < 0) {
    return false;
  }

  aOne->mFd = base::FileDescriptor(fd1, true);
  aTwo->mFd = base::FileDescriptor(fd2, true);
  return true;
}

Transport*
OpenDescriptor(const TransportDescriptor& aTd, Transport::Mode aMode)
{
  return new Transport(aTd.mFd.fd, aMode, nullptr);
}

Transport*
OpenDescriptor(const FileDescriptor& aFd, Transport::Mode aMode)
{
  return new Transport(aFd.PlatformHandle(), aMode, nullptr);
}

void
CloseDescriptor(const TransportDescriptor& aTd)
{
  close(aTd.mFd.fd);
}

} 
} 
