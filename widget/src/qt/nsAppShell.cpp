





































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

void nsAppShell::EventNativeCallback()
{
    NativeEventCallback();
    return;
}

nsAppShell::~nsAppShell()
{
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

    connect (this, SIGNAL(activated()), SLOT(EventNativeCallback()));
    return nsBaseAppShell::Init();
}

void
nsAppShell::ScheduleNativeEventCallback()
{
  emit activated();
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
