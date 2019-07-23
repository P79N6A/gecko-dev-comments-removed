






































#include "nsToolkit.h"
#include "nsSwitchToUIThread.h"
#include "nsWidgetAtoms.h"


static MRESULT EXPENTRY nsToolkitWindowProc(HWND, ULONG, MPARAM, MPARAM);

NS_IMPL_ISUPPORTS1(nsToolkit, nsIToolkit)





static PRUintn gToolkitTLSIndex = 0;




PRBool gThreadState = PR_FALSE;

struct ThreadInitInfo {
    PRMonitor *monitor;
    nsToolkit *toolkit;
};

void PR_CALLBACK RunPump(void* arg)
{
    ThreadInitInfo *info = (ThreadInitInfo*)arg;
    ::PR_EnterMonitor(info->monitor);

    
    info->toolkit->CreateInternalWindow(PR_GetCurrentThread());

    gThreadState = PR_TRUE;

    ::PR_Notify(info->monitor);
    ::PR_ExitMonitor(info->monitor);

    delete info;

    
    QMSG qmsg;
    while (WinGetMsg((HAB)0, &qmsg, 0, 0, 0)) {
        WinDispatchMsg((HAB)0, &qmsg);
    }
}






nsToolkit::nsToolkit()  
{
    mGuiThread  = NULL;
    mDispatchWnd = 0;
    mMonitor = PR_NewMonitor();
}







nsToolkit::~nsToolkit()
{
    NS_PRECONDITION(::WinIsWindow((HAB)0, mDispatchWnd), "Invalid window handle");

    
    ::WinDestroyWindow(mDispatchWnd);
    mDispatchWnd = NULL;

    
    PR_SetThreadPrivate(gToolkitTLSIndex, nsnull);
    PR_DestroyMonitor( mMonitor);
}







void nsToolkit::CreateInternalWindow(PRThread *aThread)
{
    
    NS_PRECONDITION(aThread, "null thread");
    mGuiThread  = aThread;

    
    
    
    WinRegisterClass((HAB)0, "nsToolkitClass", nsToolkitWindowProc, NULL, 0);
    mDispatchWnd = ::WinCreateWindow(HWND_DESKTOP,
                                     "nsToolkitClass",
                                     "NetscapeDispatchWnd",
                                     WS_DISABLED,
                                     -50, -50,
                                     10, 10,
                                     HWND_DESKTOP,
                                     HWND_BOTTOM,
                                     0, 0, 0);
    VERIFY(mDispatchWnd);
}







void nsToolkit::CreateUIThread()
{
    PRMonitor *monitor = ::PR_NewMonitor();

    ::PR_EnterMonitor(monitor);

    ThreadInitInfo *ti = new ThreadInitInfo();
    ti->monitor = monitor;
    ti->toolkit = this;

    
    mGuiThread = ::PR_CreateThread(PR_SYSTEM_THREAD,
                                    RunPump,
                                    (void*)ti,
                                    PR_PRIORITY_NORMAL,
                                    PR_LOCAL_THREAD,
                                    PR_UNJOINABLE_THREAD,
                                    0);

    
    while(gThreadState == PR_FALSE) {
        ::PR_Wait(monitor, PR_INTERVAL_NO_TIMEOUT);
    }

    
    ::PR_ExitMonitor(monitor);
    ::PR_DestroyMonitor(monitor);
}






NS_METHOD nsToolkit::Init(PRThread *aThread)
{
    
    
    if (NULL != aThread) {
        CreateInternalWindow(aThread);
    } else {
        
        CreateUIThread();
    }

    nsWidgetAtoms::RegisterAtoms();

    return NS_OK;
}







MRESULT EXPENTRY nsToolkitWindowProc(HWND hWnd, ULONG msg, MPARAM mp1,
                                     MPARAM mp2)
{
    switch (msg) {
        case WM_CALLMETHOD:
        {
            MethodInfo *info = (MethodInfo *)mp2;
            return (MRESULT)info->Invoke();
        }
    }
    return ::WinDefWindowProc(hWnd, msg, mp1, mp2);
}









NS_METHOD NS_GetCurrentToolkit(nsIToolkit* *aResult)
{
  nsIToolkit* toolkit = nsnull;
  nsresult rv = NS_OK;
  PRStatus status;

  
  if (0 == gToolkitTLSIndex) {
    status = PR_NewThreadPrivateIndex(&gToolkitTLSIndex, NULL);
    if (PR_FAILURE == status) {
      rv = NS_ERROR_FAILURE;
    }
  }

  if (NS_SUCCEEDED(rv)) {
    toolkit = (nsIToolkit*)PR_GetThreadPrivate(gToolkitTLSIndex);

    
    
    
    if (!toolkit) {
      toolkit = new nsToolkit();

      if (!toolkit) {
        rv = NS_ERROR_OUT_OF_MEMORY;
      } else {
        NS_ADDREF(toolkit);
        toolkit->Init(PR_GetCurrentThread());
        
        
        
        
        PR_SetThreadPrivate(gToolkitTLSIndex, (void*)toolkit);
      }
    } else {
      NS_ADDREF(toolkit);
    }
    *aResult = toolkit;
  }

  return rv;
}
