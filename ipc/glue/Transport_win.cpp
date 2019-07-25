







































#include "base/message_loop.h"
#include "chrome/common/child_process_info.h"

#include "mozilla/ipc/Transport.h"

using namespace base;
using namespace std;

namespace mozilla {
namespace ipc {

bool
CreateTransport(ProcessHandle aProcOne, ProcessHandle ,
                TransportDescriptor* aOne, TransportDescriptor* aTwo)
{
  
  
  wstring id = ChildProcessInfo::GenerateRandomChannelID(aOne);
  
  Transport t(id, Transport::MODE_SERVER, nsnull);
  HANDLE serverPipe = t.GetServerPipeHandle();
  if (!serverPipe) {
    return false;
  }

  
  
  
  
  
  HANDLE serverDup;
  DWORD access = 0;
  DWORD options = DUPLICATE_SAME_ACCESS;
  if (!DuplicateHandle(GetCurrentProcess(), serverPipe, aProcOne,
                       &serverDup,
                       access,
                       FALSE,
                       options)) {
    return false;
  }

  aOne->mPipeName = aTwo->mPipeName = id;
  aOne->mServerPipe = serverDup;
  aTwo->mServerPipe = 0;
  return true;
}

Transport*
OpenDescriptor(const TransportDescriptor& aTd, Transport::Mode aMode)
{
  return new Transport(aTd.mPipeName, aTd.mServerPipe, aMode, nsnull);
}

} 
} 
