






#include "base/message_loop.h"
#include "chrome/common/child_process_info.h"

#include "mozilla/ipc/Transport.h"
#include "mozilla/ipc/ProtocolUtils.h"

using namespace std;

using base::ProcessHandle;

namespace mozilla {
namespace ipc {

bool
CreateTransport(base::ProcessId aProcIdOne,
                TransportDescriptor* aOne, TransportDescriptor* aTwo)
{
  wstring id = IPC::Channel::GenerateVerifiedChannelID(std::wstring());
  
  Transport t(id, Transport::MODE_SERVER, nullptr);
  HANDLE serverPipe = t.GetServerPipeHandle();
  if (!serverPipe) {
    return false;
  }

  
  
  
  
  
  HANDLE serverDup;
  DWORD access = 0;
  DWORD options = DUPLICATE_SAME_ACCESS;
  if (!DuplicateHandle(serverPipe, aProcIdOne, &serverDup, access, options)) {
    return false;
  }

  aOne->mPipeName = aTwo->mPipeName = id;
  aOne->mServerPipe = serverDup;
  aTwo->mServerPipe = INVALID_HANDLE_VALUE;
  return true;
}

Transport*
OpenDescriptor(const TransportDescriptor& aTd, Transport::Mode aMode)
{
  return new Transport(aTd.mPipeName, aTd.mServerPipe, aMode, nullptr);
}

Transport*
OpenDescriptor(const FileDescriptor& aFd, Transport::Mode aMode)
{
  NS_NOTREACHED("Not implemented!");
  return nullptr;
}

void
CloseDescriptor(const TransportDescriptor& aTd)
{
  CloseHandle(aTd.mServerPipe);
}

} 
} 
