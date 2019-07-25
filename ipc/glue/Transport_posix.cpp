







































#include <unistd.h>

#include <string>

#include "chrome/common/child_process_info.h"

#include "mozilla/ipc/Transport.h"

using namespace base;
using namespace std;

namespace mozilla {
namespace ipc {

bool
CreateTransport(ProcessHandle , ProcessHandle ,
                TransportDescriptor* aOne, TransportDescriptor* aTwo)
{
  
  
  
  wstring id = ChildProcessInfo::GenerateRandomChannelID(aOne);
  
  Transport t(id, Transport::MODE_SERVER, nsnull);
  int fd1 = t.GetServerFileDescriptor();
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

  aOne->mFd = FileDescriptor(fd1, true);
  aTwo->mFd = FileDescriptor(fd2, true);
  return true;
}

Transport*
OpenDescriptor(const TransportDescriptor& aTd, Transport::Mode aMode)
{
  return new Transport(aTd.mFd.fd, aMode, nsnull);
}

} 
} 
