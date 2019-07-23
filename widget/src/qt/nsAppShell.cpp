





































#include "nsAppShell.h"
#include <qapplication.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <glib.h>
#include <glib/gmain.h>

#include "prenv.h"

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

static int sPokeEvent;

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
#if (QT_VERSION >= QT_VERSION_CHECK(4, 4, 0))
    sPokeEvent = QEvent::registerEventType();
#endif
    return nsBaseAppShell::Init();
}

void
nsAppShell::ScheduleNativeEventCallback()
{
    QCoreApplication::postEvent(this,
                                new QEvent((QEvent::Type) sPokeEvent));
}


PRBool
nsAppShell::ProcessNextNativeEvent(PRBool mayWait)
{
    if (PR_GetEnv("QT_NO_GLIB")) {
        QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents;

        if (mayWait)
            flags |= QEventLoop::WaitForMoreEvents;

        qApp->processEvents(flags);
        return PR_TRUE;
    }

    return g_main_context_iteration(g_main_context_default(), mayWait);
}

bool
nsAppShell::event (QEvent *e)
{
    if (e->type() == sPokeEvent) {
        NativeEventCallback();
        return true;
    }

    return false;
}
