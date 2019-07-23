





































#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <gdk/gdkwindow.h>
#include "nsCommonWidget.h"
#include "nsAppShell.h"
#include "prlog.h"
#include "prenv.h"

#define NOTIFY_TOKEN 0xFA

#ifdef PR_LOGGING
PRLogModuleInfo *gWidgetLog = nsnull;
PRLogModuleInfo *gWidgetFocusLog = nsnull;
PRLogModuleInfo *gWidgetIMLog = nsnull;
PRLogModuleInfo *gWidgetDrawLog = nsnull;
#endif

 gboolean
nsAppShell::EventProcessorCallback(GIOChannel *source, 
                                   GIOCondition condition,
                                   gpointer data)
{
    nsAppShell *self = NS_STATIC_CAST(nsAppShell *, data);

    unsigned char c;
    read(self->mPipeFDs[0], &c, 1);
    NS_ASSERTION(c == (unsigned char) NOTIFY_TOKEN, "wrong token");

    self->NativeEventCallback();
    return TRUE;
}

nsAppShell::~nsAppShell()
{
    if (mTag)
        g_source_remove(mTag);
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

    GIOChannel *ioc;

    if (PR_GetEnv("MOZ_DEBUG_PAINTS"))
        gdk_window_set_debug_updates(TRUE);

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

    ioc = g_io_channel_unix_new(mPipeFDs[0]);
    mTag = g_io_add_watch_full(ioc, G_PRIORITY_DEFAULT, G_IO_IN,
                               EventProcessorCallback, this, nsnull);
    g_io_channel_unref(ioc);

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
    unsigned char buf[] = { NOTIFY_TOKEN };
    write(mPipeFDs[1], buf, 1);
}

PRBool
nsAppShell::ProcessNextNativeEvent(PRBool mayWait)
{
    return g_main_context_iteration(NULL, mayWait);
}
