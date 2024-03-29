






#include "nsAppShell.h"
#include <QGuiApplication>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <qabstracteventdispatcher.h>
#include <qthread.h>

#include "prenv.h"
#include "nsQAppInstance.h"

#ifdef MOZ_LOGGING
#include "mozilla/Logging.h"
#endif

PRLogModuleInfo *gWidgetLog = nullptr;
PRLogModuleInfo *gWidgetFocusLog = nullptr;
PRLogModuleInfo *gWidgetIMLog = nullptr;
PRLogModuleInfo *gWidgetDrawLog = nullptr;

static int sPokeEvent;

nsAppShell::~nsAppShell()
{
    nsQAppInstance::Release();
}

nsresult
nsAppShell::Init()
{
    if (!gWidgetLog)
        gWidgetLog = PR_NewLogModule("Widget");
    if (!gWidgetFocusLog)
        gWidgetFocusLog = PR_NewLogModule("WidgetFocus");
    if (!gWidgetIMLog)
        gWidgetIMLog = PR_NewLogModule("WidgetIM");
    if (!gWidgetDrawLog)
        gWidgetDrawLog = PR_NewLogModule("WidgetDraw");

    sPokeEvent = QEvent::registerEventType();

    nsQAppInstance::AddRef();

    return nsBaseAppShell::Init();
}

void
nsAppShell::ScheduleNativeEventCallback()
{
    QCoreApplication::postEvent(this,
                                new QEvent((QEvent::Type) sPokeEvent));
}


bool
nsAppShell::ProcessNextNativeEvent(bool mayWait)
{
    QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents;

    if (mayWait)
        flags |= QEventLoop::WaitForMoreEvents;

    QAbstractEventDispatcher *dispatcher =  QAbstractEventDispatcher::instance(QThread::currentThread());
    if (!dispatcher)
        return false;

    return dispatcher->processEvents(flags) ? true : false;
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
