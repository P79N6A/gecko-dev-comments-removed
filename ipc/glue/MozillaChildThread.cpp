






































#include "mozilla/ipc/MozillaChildThread.h"

#include "nsXPCOM.h"

#ifdef XP_WIN
#include <objbase.h>
#endif

namespace mozilla {
namespace ipc {

void
MozillaChildThread::OnControlMessageReceived(const IPC::Message& aMessage) {
  



}

void
MozillaChildThread::Init()
{
  ChildThread::Init();

#ifdef XP_WIN
  
  ::CoInitialize(NULL);
#endif

  
  
  message_loop()->set_exception_restoration(true);

  NS_LogInit();
}

void
MozillaChildThread::CleanUp()
{
  NS_LogTerm();

#ifdef XP_WIN
  ::CoUninitialize();
#endif
}

} 
} 
