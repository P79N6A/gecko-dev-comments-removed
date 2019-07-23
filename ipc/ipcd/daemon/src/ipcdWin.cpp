




































#include <windows.h>

#include "prthread.h"

#include "ipcConfig.h"
#include "ipcLog.h"
#include "ipcMessage.h"
#include "ipcClient.h"
#include "ipcModuleReg.h"
#include "ipcdPrivate.h"
#include "ipcd.h"
#include "ipcm.h"




ipcClient *ipcClients = NULL;
int        ipcClientCount = 0;

static ipcClient ipcClientArray[IPC_MAX_CLIENTS];

static HWND   ipcHwnd = NULL;
static PRBool ipcShutdown = PR_FALSE;

#define IPC_PURGE_TIMER_ID 1
#define IPC_WM_SENDMSG  (WM_USER + 1)
#define IPC_WM_SHUTDOWN (WM_USER + 2)





static void
RemoveClient(ipcClient *client)
{
    LOG(("RemoveClient\n"));

    int clientIndex = client - ipcClientArray;

    client->Finalize();

    
    
    
    int fromIndex = ipcClientCount - 1;
    int toIndex = clientIndex;
    if (toIndex != fromIndex)
        memcpy(&ipcClientArray[toIndex], &ipcClientArray[fromIndex], sizeof(ipcClient));

    memset(&ipcClientArray[fromIndex], 0, sizeof(ipcClient));

    --ipcClientCount;
    LOG(("  num clients = %u\n", ipcClientCount));

    if (ipcClientCount == 0) {
        LOG(("  shutting down...\n"));
        KillTimer(ipcHwnd, IPC_PURGE_TIMER_ID);
        PostQuitMessage(0);
        ipcShutdown = PR_TRUE;
    }
}

static void
PurgeStaleClients()
{
    if (ipcClientCount == 0)
        return;

    LOG(("PurgeStaleClients [num-clients=%u]\n", ipcClientCount));
    
    
    
    
    char wName[IPC_CLIENT_WINDOW_NAME_MAXLEN];
    for (int i=ipcClientCount-1; i>=0; --i) {
        ipcClient *client = &ipcClientArray[i];

        LOG(("  checking client at index %u [client-id=%u pid=%u]\n", 
            i, client->ID(), client->PID()));

        IPC_GetClientWindowName(client->PID(), wName);

        

        HWND hwnd = FindWindow(IPC_CLIENT_WINDOW_CLASS, wName);
        if (!hwnd) {
            LOG(("  client window not found; removing client!\n"));
            RemoveClient(client);
        }
    }
}

static ipcClient *
AddClient(HWND hwnd, PRUint32 pid)
{
    LOG(("AddClient\n"));

    
    
    
    
    PurgeStaleClients();

    if (ipcClientCount == IPC_MAX_CLIENTS) {
        LOG(("  reached maximum client count!\n"));
        return NULL;
    }

    ipcClient *client = &ipcClientArray[ipcClientCount];
    client->Init();
    client->SetHwnd(hwnd);
    client->SetPID(pid);    

    ++ipcClientCount;
    LOG(("  num clients = %u\n", ipcClientCount));

    if (ipcClientCount == 1)
        SetTimer(ipcHwnd, IPC_PURGE_TIMER_ID, 1000, NULL);

    return client;
}

static ipcClient *
GetClientByPID(PRUint32 pid)
{
    for (int i=0; i<ipcClientCount; ++i) {
        if (ipcClientArray[i].PID() == pid)
            return &ipcClientArray[i];
    }
    return NULL;
}





static void
ProcessMsg(HWND hwnd, PRUint32 pid, const ipcMessage *msg)
{
    LOG(("ProcessMsg [pid=%u len=%u]\n", pid, msg->MsgLen()));

    ipcClient *client = GetClientByPID(pid);

    if (client) {
        
        
        
        
        if (msg->Target().Equals(IPCM_TARGET) &&
            IPCM_GetType(msg) == IPCM_MSG_REQ_CLIENT_HELLO) {
            RemoveClient(client);
            client = NULL;
        }
    }

    if (client == NULL) {
        client = AddClient(hwnd, pid);
        if (!client)
            return;
    }

    IPC_DispatchMsg(client, msg);
}



PRStatus
IPC_PlatformSendMsg(ipcClient  *client, ipcMessage *msg)
{
    LOG(("IPC_PlatformSendMsg [clientID=%u clientPID=%u]\n",
        client->ID(), client->PID()));

    
    

    WPARAM wParam = (WPARAM) client->Hwnd();
    LPARAM lParam = (LPARAM) msg;
    if (!PostMessage(ipcHwnd, IPC_WM_SENDMSG, wParam, lParam)) {
        LOG(("PostMessage failed\n"));
        delete msg;
        return PR_FAILURE;
    }
    return PR_SUCCESS;
}





static LRESULT CALLBACK
WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LOG(("got message [msg=%x wparam=%x lparam=%x]\n", uMsg, wParam, lParam));

    if (uMsg == WM_COPYDATA) {
        if (ipcShutdown) {
            LOG(("ignoring message b/c daemon is shutting down\n"));
            return TRUE;
        }
        COPYDATASTRUCT *cd = (COPYDATASTRUCT *) lParam;
        if (cd && cd->lpData) {
            ipcMessage msg;
            PRUint32 bytesRead;
            PRBool complete;
            
            PRStatus rv = msg.ReadFrom((const char *) cd->lpData, cd->cbData,
                                       &bytesRead, &complete);
            if (rv == PR_SUCCESS && complete) {
                
                
                
                ProcessMsg((HWND) wParam, (PRUint32) cd->dwData, &msg);
            }
            else
                LOG(("ignoring malformed message\n"));
        }
        return TRUE;
    }

    if (uMsg == IPC_WM_SENDMSG) {
        HWND hWndDest = (HWND) wParam;
        ipcMessage *msg = (ipcMessage *) lParam;

        COPYDATASTRUCT cd;
        cd.dwData = GetCurrentProcessId();
        cd.cbData = (DWORD) msg->MsgLen();
        cd.lpData = (PVOID) msg->MsgBuf();

        LOG(("calling SendMessage...\n"));
        SendMessage(hWndDest, WM_COPYDATA, (WPARAM) hWnd, (LPARAM) &cd);
        LOG(("  done.\n"));

        delete msg;
        return 0;
    }

    if (uMsg == WM_TIMER) {
        PurgeStaleClients();
        return 0;
    }

#if 0
    if (uMsg == IPC_WM_SHUTDOWN) {
        
        
        
        
        
        if (ipcClientCount == 0) {
            ipcShutdown = PR_TRUE;
            PostQuitMessage(0);
        }
        return 0;
    }
#endif

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}





static HANDLE ipcSyncEvent;

static PRBool
AcquireLock()
{
    ipcSyncEvent = CreateEvent(NULL, FALSE, FALSE,
                               IPC_SYNC_EVENT_NAME);
    if (!ipcSyncEvent) {
        LOG(("CreateEvent failed [%u]\n", GetLastError()));
        return PR_FALSE;
    }

    
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        LOG(("  lock already set; exiting...\n"));
        return PR_FALSE;
    }
    
    LOG(("  acquired lock\n"));
    return PR_TRUE;
}

static void
ReleaseLock()
{
    if (ipcSyncEvent) {
        LOG(("releasing lock...\n"));
        CloseHandle(ipcSyncEvent);
        ipcSyncEvent = NULL;
    }
}





#ifdef DEBUG
int
main()
#else
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
    IPC_InitLog("###");

    LOG(("daemon started...\n"));

    if (!AcquireLock()) {
        
        
        IPC_NotifyParent();
        return 0;
    }

    
    memset(ipcClientArray, 0, sizeof(ipcClientArray));
    ipcClients = ipcClientArray;
    ipcClientCount = 0;

    
    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.lpszClassName = IPC_WINDOW_CLASS;

    RegisterClass(&wc);

    ipcHwnd = CreateWindow(IPC_WINDOW_CLASS, IPC_WINDOW_NAME,
                           0, 0, 0, 10, 10, NULL, NULL, NULL, NULL);

    
    IPC_NotifyParent();

    if (!ipcHwnd)
        return -1;

    
    {
        char path[MAX_PATH];
        GetModuleFileName(NULL, path, sizeof(path));
        IPC_InitModuleReg(path);
    }

    LOG(("entering message loop...\n"));
    MSG msg;
    while (GetMessage(&msg, ipcHwnd, 0, 0))
        DispatchMessage(&msg);

    
    IPC_ShutdownModuleReg();

    
    
    
    
    
    ReleaseLock();

    
    

    LOG(("destroying message window...\n"));
    DestroyWindow(ipcHwnd);
    ipcHwnd = NULL;

    LOG(("exiting\n"));
    return 0;
}
