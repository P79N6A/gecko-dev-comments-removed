





































#include "nsAppShell.h"
#include <qapplication.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define NOTIFY_TOKEN 0xFA

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#include "prlog.h"
#endif

#ifdef PR_LOGGING
PRLogModuleInfo *gWidgetLog = nsnull;
PRLogModuleInfo *gWidgetFocusLog = nsnull;
PRLogModuleInfo *gWidgetIMLog = nsnull;
PRLogModuleInfo *gWidgetDrawLog = nsnull;
#endif

void nsAppShell::EventNativeCallback(int fd)
{
    unsigned char c;
    read (mPipeFDs[0], &c, sizeof(unsigned int));
    NS_ASSERTION(c == (unsigned char) NOTIFY_TOKEN, "wrong token");

    NativeEventCallback();
    return;
}

nsAppShell::~nsAppShell()
{
    if (mTag)
        mTag = 0;
    if (mPipeFDs[0])
        close(mPipeFDs[0]);
    if (mPipeFDs[1])
        close(mPipeFDs[1]);
}

nsresult
nsAppShell::Init()
{
#ifdef PR_LOGGING
    if (!gWidgetLog)
        gWidgetLog = PR_NewLogModule("Widget");
    if (!gWidgetFocusLog)
        gWidgetFocusLog = PR_NewLogModule("WidgetFocus");
    if (!gWidgetIMLog)
        gWidgetIMLog = PR_NewLogModule("WidgetIM");
    if (!gWidgetDrawLog)
        gWidgetDrawLog = PR_NewLogModule("WidgetDraw");
#endif

    int err = pipe(mPipeFDs);
    if (err)
        return NS_ERROR_OUT_OF_MEMORY;

    

    int flags = fcntl(mPipeFDs[0], F_GETFL, 0);
    if (flags == -1)
        goto failed;
    err = fcntl(mPipeFDs[0], F_SETFL, flags | O_NONBLOCK);
    if (err == -1)
        goto failed;
    flags = fcntl(mPipeFDs[1], F_GETFL, 0);
    if (flags == -1)
        goto failed;
    err = fcntl(mPipeFDs[1], F_SETFL, flags | O_NONBLOCK);
    if (err == -1)
        goto failed;

    mTag = new QSocketNotifier (mPipeFDs[0], QSocketNotifier::Read);
    connect (mTag, SIGNAL(activated(int)), SLOT(EventNativeCallback(int)));

    return nsBaseAppShell::Init();
failed:
    close(mPipeFDs[0]);
    close(mPipeFDs[1]);
    mPipeFDs[0] = mPipeFDs[1] = 0;
    return NS_ERROR_FAILURE;
}

void
nsAppShell::ScheduleNativeEventCallback()
{
  unsigned char buf [] = { NOTIFY_TOKEN };
  write (mPipeFDs[1], buf, 1);
}

PRBool
nsAppShell::ProcessNextNativeEvent(PRBool mayWait)
{
  QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents;

  if (mayWait)
    flags |= QEventLoop::WaitForMoreEvents;

  qApp->processEvents(flags);
  return PR_TRUE;
}
