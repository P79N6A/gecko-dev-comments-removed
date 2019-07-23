




































#include <windows.h>

#include "prprf.h"
#include "prmon.h"
#include "prthread.h"
#include "plevent.h"

#include "nsIServiceManager.h"
#include "nsIEventQueue.h"
#include "nsIEventQueueService.h"
#include "nsAutoLock.h"

#include "ipcConfig.h"
#include "ipcLog.h"
#include "ipcConnection.h"
#include "ipcm.h"















#define IPC_WM_SENDMSG    (WM_USER + 0x1)
#define IPC_WM_CALLBACK   (WM_USER + 0x2)
#define IPC_WM_SHUTDOWN   (WM_USER + 0x3)

static nsresult       ipcThreadStatus = NS_OK;
static PRThread      *ipcThread = NULL;
static PRMonitor     *ipcMonitor = NULL;
static HWND           ipcDaemonHwnd = NULL;
static HWND           ipcLocalHwnd = NULL;
static PRBool         ipcShutdown = PR_FALSE; 





static LRESULT CALLBACK
ipcThreadWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LOG(("got message [msg=%x wparam=%x lparam=%x]\n", uMsg, wParam, lParam));

    if (uMsg == WM_COPYDATA) {
        COPYDATASTRUCT *cd = (COPYDATASTRUCT *) lParam;
        if (cd && cd->lpData) {
            ipcMessage *msg = new ipcMessage();
            PRUint32 bytesRead;
            PRBool complete;
            PRStatus rv = msg->ReadFrom((const char *) cd->lpData, cd->cbData,
                                        &bytesRead, &complete);
            if (rv == PR_SUCCESS && complete)
                IPC_OnMessageAvailable(msg); 
            else {
                LOG(("  unable to deliver message [complete=%u]\n", complete));
                delete msg;
            }
        }
        return TRUE;
    }

    if (uMsg == IPC_WM_SENDMSG) {
        ipcMessage *msg = (ipcMessage *) lParam;
        if (msg) {
            LOG(("  sending message...\n"));
            COPYDATASTRUCT cd;
            cd.dwData = GetCurrentProcessId();
            cd.cbData = (DWORD) msg->MsgLen();
            cd.lpData = (PVOID) msg->MsgBuf();
            SendMessageA(ipcDaemonHwnd, WM_COPYDATA, (WPARAM) hWnd, (LPARAM) &cd);
            LOG(("  done.\n"));
            delete msg;
        }
        return 0;
    }

    if (uMsg == IPC_WM_CALLBACK) {
        ipcCallbackFunc func = (ipcCallbackFunc) wParam;
        void *arg = (void *) lParam;
        (func)(arg);
        return 0;
    }

    if (uMsg == IPC_WM_SHUTDOWN) {
        IPC_OnConnectionEnd(NS_OK);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}





static void
ipcThreadFunc(void *arg)
{
    LOG(("entering message thread\n"));

    DWORD pid = GetCurrentProcessId();

    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = ipcThreadWindowProc;
    wc.lpszClassName = IPC_CLIENT_WINDOW_CLASS;
    RegisterClass(&wc);

    char wName[sizeof(IPC_CLIENT_WINDOW_NAME_PREFIX) + 20];
    PR_snprintf(wName, sizeof(wName), "%s%u", IPC_CLIENT_WINDOW_NAME_PREFIX, pid);

    ipcLocalHwnd = CreateWindow(IPC_CLIENT_WINDOW_CLASS, wName,
                                0, 0, 0, 10, 10, NULL, NULL, NULL, NULL);

    {
        nsAutoMonitor mon(ipcMonitor);
        if (!ipcLocalHwnd)
            ipcThreadStatus = NS_ERROR_FAILURE;
        mon.Notify();
    }

    if (ipcLocalHwnd) {
        MSG msg;
        while (GetMessage(&msg, ipcLocalHwnd, 0, 0))
            DispatchMessage(&msg);

        ipcShutdown = PR_TRUE; 

        DestroyWindow(ipcLocalHwnd);
        ipcLocalHwnd = NULL;
    }

    LOG(("exiting message thread\n"));
    return;
}

static PRStatus
ipcThreadInit()
{
    if (ipcThread)
        return PR_FAILURE;

    ipcShutdown = PR_FALSE;

    ipcMonitor = PR_NewMonitor();
    if (!ipcMonitor)
        return PR_FAILURE;

    
    ipcThread = PR_CreateThread(PR_USER_THREAD, ipcThreadFunc, NULL,
                                PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                                PR_JOINABLE_THREAD, 0);
    if (!ipcThread) {
        NS_WARNING("thread creation failed");
        PR_DestroyMonitor(ipcMonitor);
        ipcMonitor = NULL;
        return PR_FAILURE;
    }

    
    {
        nsAutoMonitor mon(ipcMonitor);
        while (!ipcLocalHwnd && NS_SUCCEEDED(ipcThreadStatus))
            mon.Wait();
    }

    if (NS_FAILED(ipcThreadStatus)) {
        NS_WARNING("message thread failed");
        return PR_FAILURE;
    }

    return PR_SUCCESS;
}

static PRStatus
ipcThreadShutdown()
{
    if (PR_AtomicSet(&ipcShutdown, PR_TRUE) == PR_FALSE) {
        LOG(("posting IPC_WM_SHUTDOWN message\n"));
        PostMessage(ipcLocalHwnd, IPC_WM_SHUTDOWN, 0, 0);
    }

    LOG(("joining w/ message thread...\n"));
    PR_JoinThread(ipcThread);
    ipcThread = NULL;

    
    
    

    PR_DestroyMonitor(ipcMonitor);
    ipcMonitor = NULL;

    return PR_SUCCESS;
}





nsresult
IPC_Disconnect()
{
    LOG(("IPC_Disconnect\n"));

    
    
    if (!ipcDaemonHwnd)
        return NS_ERROR_NOT_INITIALIZED;

    if (ipcThread)
        ipcThreadShutdown();

    
    ipcDaemonHwnd = NULL;
    return NS_OK;
}

nsresult
IPC_Connect(const char *daemonPath)
{
    LOG(("IPC_Connect\n"));

    NS_ENSURE_TRUE(ipcDaemonHwnd == NULL, NS_ERROR_ALREADY_INITIALIZED);
    nsresult rv;

    ipcDaemonHwnd = FindWindow(IPC_WINDOW_CLASS, IPC_WINDOW_NAME);
    if (!ipcDaemonHwnd) {
        LOG(("  daemon does not appear to be running\n"));
        
        
        
        rv = IPC_SpawnDaemon(daemonPath);
        if (NS_FAILED(rv))
            return rv;

        ipcDaemonHwnd = FindWindow(IPC_WINDOW_CLASS, IPC_WINDOW_NAME);
        if (!ipcDaemonHwnd)
            return NS_ERROR_FAILURE;
    }

    
    
    
    if (!ipcThread && ipcThreadInit() != PR_SUCCESS) {
        ipcDaemonHwnd = NULL;
        return NS_ERROR_FAILURE;
    }

    return NS_OK;
}

nsresult
IPC_SendMsg(ipcMessage *msg)
{
    LOG(("IPC_SendMsg\n"));

    if (ipcShutdown) {
        LOG(("unable to send message b/c message thread is shutdown\n"));
        goto loser;
    }
    if (!PostMessage(ipcLocalHwnd, IPC_WM_SENDMSG, 0, (LPARAM) msg)) {
        LOG(("  PostMessage failed w/ error = %u\n", GetLastError()));
        goto loser;
    }
    return NS_OK;
loser:
    delete msg;
    return NS_ERROR_FAILURE;
}

nsresult
IPC_DoCallback(ipcCallbackFunc func, void *arg)
{
    LOG(("IPC_DoCallback\n"));

    if (ipcShutdown) {
        LOG(("unable to send message b/c message thread is shutdown\n"));
        return NS_ERROR_FAILURE;
    }
    if (!PostMessage(ipcLocalHwnd, IPC_WM_CALLBACK, (WPARAM) func, (LPARAM) arg)) {
        LOG(("  PostMessage failed w/ error = %u\n", GetLastError()));
        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}
