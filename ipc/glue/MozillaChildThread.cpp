






































#include "mozilla/ipc/MozillaChildThread.h"

#include "nsXPCOM.h"

namespace mozilla {
namespace ipc {

void
MozillaChildThread::OnControlMessageReceived(const IPC::Message& aMessage) {
  



}

void
MozillaChildThread::Init()
{
  ChildThread::Init();

  
  
  message_loop()->set_exception_restoration(true);

  NS_LogInit();
}

void
MozillaChildThread::CleanUp()
{
  NS_LogTerm();
}

} 
} 
