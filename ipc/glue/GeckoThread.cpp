



































#include "mozilla/ipc/GeckoThread.h"

#include "chrome/common/notification_service.h"

#include "nsXPCOM.h"

#include "nsRegion.h"

#if defined(OS_WIN)
#include <objbase.h>
#endif

using mozilla::ipc::GeckoThread;
using mozilla::ipc::BrowserProcessSubThread;

void
GeckoThread::OnControlMessageReceived(const IPC::Message& aMessage) {
  



}

void
GeckoThread::Init()
{
  ChildThread::Init();

  
  
  message_loop()->set_exception_restoration(true);

  NS_LogInit();
  mXREEmbed.Start();
}

void
GeckoThread::CleanUp()
{
  mXREEmbed.Stop();
  NS_LogTerm();
}






static const char* kBrowserThreadNames[BrowserProcessSubThread::ID_COUNT] = {
  "Gecko_IOThread",  



#if defined(OS_LINUX)
  "Gecko_Background_X11Thread",  
#endif
};

Lock BrowserProcessSubThread::sLock;
BrowserProcessSubThread* BrowserProcessSubThread::sBrowserThreads[ID_COUNT] = {
  NULL,  



#if defined(OS_LINUX)
  NULL,  
#endif
};

BrowserProcessSubThread::BrowserProcessSubThread(ID aId) :
    base::Thread(kBrowserThreadNames[aId]),
    mIdentifier(aId),
    mNotificationService(NULL)
{
    Init();
    AutoLock lock(sLock);
    DCHECK(aId >= 0 && aId < ID_COUNT);
    DCHECK(sBrowserThreads[aId] == NULL);
    sBrowserThreads[aId] = this;
}

BrowserProcessSubThread::~BrowserProcessSubThread()
{
    Stop();
    CleanUp();
    {AutoLock lock(sLock);
        sBrowserThreads[mIdentifier] = NULL;
    }

}

void
BrowserProcessSubThread::Init()
{
#if defined(OS_WIN)
  
  CoInitialize(NULL);
#endif
  mNotificationService = new NotificationService();
}

void
BrowserProcessSubThread::CleanUp()
{
  delete mNotificationService;
  mNotificationService = NULL;

#if defined(OS_WIN)
  
  
  CoUninitialize();
#endif
}


MessageLoop*
BrowserProcessSubThread::GetMessageLoop(ID aId) {
  AutoLock lock(sLock);
  DCHECK(aId >= 0 && aId < ID_COUNT);

  if (sBrowserThreads[aId])
    return sBrowserThreads[aId]->message_loop();

  return NULL;
}
