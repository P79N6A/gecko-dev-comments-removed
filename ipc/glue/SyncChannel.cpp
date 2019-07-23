






































#include "mozilla/ipc/SyncChannel.h"
#include "mozilla/ipc/GeckoThread.h"

#include "nsDebug.h"

#ifdef OS_WIN
#include "nsServiceManagerUtils.h"
#include "nsStringGlue.h"
#include "nsIXULAppInfo.h"
#endif

using mozilla::MutexAutoLock;

template<>
struct RunnableMethodTraits<mozilla::ipc::SyncChannel>
{
    static void RetainCallee(mozilla::ipc::SyncChannel* obj) { }
    static void ReleaseCallee(mozilla::ipc::SyncChannel* obj) { }
};

namespace mozilla {
namespace ipc {

SyncChannel::SyncChannel(SyncListener* aListener)
  : AsyncChannel(aListener),
    mPendingReply(0),
    mProcessingSyncMessage(false)
#ifdef OS_WIN
  , mUIThreadId(0)
  , mEventLoopDepth(0)
#endif
{
  MOZ_COUNT_CTOR(SyncChannel);
}

SyncChannel::~SyncChannel()
{
    MOZ_COUNT_DTOR(SyncChannel);
    
}

bool
SyncChannel::Send(Message* msg, Message* reply)
{
    AssertWorkerThread();
    NS_ABORT_IF_FALSE(!ProcessingSyncMessage(),
                      "violation of sync handler invariant");
    NS_ABORT_IF_FALSE(msg->is_sync(), "can only Send() sync messages here");

    MutexAutoLock lock(mMutex);

    if (!Connected()) {
        ReportConnectionError("SyncChannel");
        return false;
    }

    mPendingReply = msg->type() + 1;
    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &SyncChannel::OnSend, msg));

    
    WaitForNotify();

    if (!Connected()) {
        ReportConnectionError("SyncChannel");
        return false;
    }

    
    
    
    
    

    
    NS_ABORT_IF_FALSE(mRecvd.is_sync() && mRecvd.is_reply() &&
                      (mPendingReply == mRecvd.type() || mRecvd.is_reply_error()),
                      "unexpected sync message");

    mPendingReply = 0;
    *reply = mRecvd;

    return true;
}

void
SyncChannel::OnDispatchMessage(const Message& msg)
{
    AssertWorkerThread();
    NS_ABORT_IF_FALSE(msg.is_sync(), "only sync messages here");
    NS_ABORT_IF_FALSE(!msg.is_reply(), "wasn't awaiting reply");

    Message* reply = 0;

    mProcessingSyncMessage = true;
    Result rv =
        static_cast<SyncListener*>(mListener)->OnMessageReceived(msg, reply);
    mProcessingSyncMessage = false;

    if (!MaybeHandleError(rv, "SyncChannel")) {
        
        delete reply;
        reply = new Message();
        reply->set_sync();
        reply->set_reply();
        reply->set_reply_error();
    }

    mIOLoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &SyncChannel::OnSend, reply));
}






void
SyncChannel::OnMessageReceived(const Message& msg)
{
    AssertIOThread();
    if (!msg.is_sync()) {
        return AsyncChannel::OnMessageReceived(msg);
    }

    MutexAutoLock lock(mMutex);

    if (!AwaitingSyncReply()) {
        
        mWorkerLoop->PostTask(
            FROM_HERE,
            NewRunnableMethod(this, &SyncChannel::OnDispatchMessage, msg));
    }
    else {
        
        mRecvd = msg;
        NotifyWorkerThread();
    }
}

void
SyncChannel::OnChannelError()
{
    AssertIOThread();
    {
        MutexAutoLock lock(mMutex);

        mChannelState = ChannelError;

        if (AwaitingSyncReply()) {
            NotifyWorkerThread();
        }
    }

    return AsyncChannel::OnChannelError();
}





namespace {
bool gPumpingMessages = false;
} 


bool
SyncChannel::IsPumpingMessages()
{
    return gPumpingMessages;
}

#ifdef OS_WIN









































namespace {

UINT gEventLoopMessage =
    RegisterWindowMessage(L"SyncChannel::RunWindowsEventLoop Message");

const wchar_t kOldWndProcProp[] = L"MozillaIPCOldWndProc";

PRUnichar gAppMessageWindowName[256] = { 0 };
PRInt32 gAppMessageWindowNameLength = 0;

nsTArray<HWND>* gNeuteredWindows = nsnull;

LRESULT CALLBACK
NeuteredWindowProc(HWND hwnd,
                   UINT uMsg,
                   WPARAM wParam,
                   LPARAM lParam)
{
    WNDPROC oldWndProc = (WNDPROC)GetProp(hwnd, kOldWndProcProp);
    if (!oldWndProc) {
        
        NS_ERROR("No old wndproc!");
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    
    
    
    
    

#ifdef DEBUG
    {
        printf("WARNING: Received nonqueued message 0x%x during a sync IPC "
               "message for window 0x%d", uMsg, hwnd);

        wchar_t className[256] = { 0 };
        if (GetClassNameW(hwnd, className, sizeof(className) - 1) > 0) {
            printf(" (\"%S\")", className);
        }

        printf(", sending it to DefWindowProc instead of the normal "
               "window procedure.\n");
    }
#endif
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static bool
WindowIsMozillaWindow(HWND hWnd)
{
    if (!IsWindow(hWnd)) {
        NS_WARNING("Window has died!");
        return false;
    }

    PRUnichar buffer[256] = { 0 };
    int length = GetClassNameW(hWnd, (wchar_t*)buffer, sizeof(buffer) - 1);
    if (length <= 0) {
        NS_WARNING("Failed to get class name!");
        return false;
    }

    nsDependentString className(buffer, length);
    if (StringBeginsWith(className, NS_LITERAL_STRING("Mozilla")) ||
        StringBeginsWith(className, NS_LITERAL_STRING("Gecko")) ||
        className.EqualsLiteral("nsToolkitClass") ||
        className.EqualsLiteral("nsAppShell:EventWindowClass")) {
        return true;
    }

    
    
    
    if (gAppMessageWindowNameLength == 0) {
        
        nsCOMPtr<nsIXULAppInfo> appInfo =
            do_GetService("@mozilla.org/xre/app-info;1");
        if (appInfo) {
            nsCAutoString appName;
            if (NS_SUCCEEDED(appInfo->GetName(appName))) {
                appName.Append("MessageWindow");
                nsDependentString windowName(gAppMessageWindowName);
                CopyUTF8toUTF16(appName, windowName);
                gAppMessageWindowNameLength = windowName.Length();
            }
        }

        
        if (gAppMessageWindowNameLength == 0) {
            gAppMessageWindowNameLength = -1;
        }
    }

    if (gAppMessageWindowNameLength != -1 &&
        className.Equals(nsDependentString(gAppMessageWindowName,
                                           gAppMessageWindowNameLength))) {
        return true;
    }

    return false;
}

bool
NeuterWindowProcedure(HWND hWnd)
{
    if (!WindowIsMozillaWindow(hWnd)) {
        
        return false;
    }

    NS_ASSERTION(!GetProp(hWnd, kOldWndProcProp),
                 "This should always be null!");

    
    
    
    SetLastError(ERROR_SUCCESS);

    LONG_PTR currentWndProc =
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)NeuteredWindowProc);
    if (!currentWndProc) {
        if (ERROR_SUCCESS == GetLastError()) {
            
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, currentWndProc);
        }
        return false;
    }

    NS_ASSERTION(currentWndProc != (LONG_PTR)NeuteredWindowProc,
                 "This shouldn't be possible!");

    if (!SetProp(hWnd, kOldWndProcProp, (HANDLE)currentWndProc)) {
        
        NS_WARNING("SetProp failed!");
        SetWindowLongPtr(hWnd, GWLP_WNDPROC, currentWndProc);
        RemoveProp(hWnd, kOldWndProcProp);
        return false;
    }

    return true;
}

void
RestoreWindowProcedure(HWND hWnd)
{
    NS_ASSERTION(WindowIsMozillaWindow(hWnd), "Not a mozilla window!");

    LONG_PTR oldWndProc = (LONG_PTR)RemoveProp(hWnd, kOldWndProcProp);
    if (oldWndProc) {
        NS_ASSERTION(oldWndProc != (LONG_PTR)NeuteredWindowProc,
                     "This shouldn't be possible!");

        LONG_PTR currentWndProc =
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, oldWndProc);
        NS_ASSERTION(currentWndProc == (LONG_PTR)NeuteredWindowProc,
                     "This should never be switched out from under us!");
    }
}

LRESULT CALLBACK
CallWindowProcedureHook(int nCode,
                        WPARAM wParam,
                        LPARAM lParam)
{
    if (nCode >= 0) {
        NS_ASSERTION(gNeuteredWindows, "This should never be null!");

        HWND hWnd = reinterpret_cast<CWPSTRUCT*>(lParam)->hwnd;

        if (!gNeuteredWindows->Contains(hWnd) && NeuterWindowProcedure(hWnd)) {
            if (!gNeuteredWindows->AppendElement(hWnd)) {
                NS_ERROR("Out of memory!");
                RestoreWindowProcedure(hWnd);
            }
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

} 

void
SyncChannel::RunWindowsEventLoop()
{
    mMutex.AssertCurrentThreadOwns();

    NS_ASSERTION(mEventLoopDepth >= 0, "Event loop depth mismatch!");

    HHOOK windowHook = NULL;

    nsAutoTArray<HWND, 20> neuteredWindows;

    if (++mEventLoopDepth == 1) {
        NS_ASSERTION(!gPumpingMessages, "Shouldn't be pumping already!");
        gPumpingMessages = true;

        if (!mUIThreadId) {
            mUIThreadId = GetCurrentThreadId();
        }
        NS_ASSERTION(mUIThreadId, "ThreadId should not be 0!");

        NS_ASSERTION(!gNeuteredWindows, "Should only set this once!");
        gNeuteredWindows = &neuteredWindows;

        windowHook = SetWindowsHookEx(WH_CALLWNDPROC, CallWindowProcedureHook,
                                      NULL, mUIThreadId);
        NS_ASSERTION(windowHook, "Failed to set hook!");
    }

    {
        MutexAutoUnlock unlock(mMutex);

        while (1) {
            
            
            
            
            
            
            
            DWORD result = MsgWaitForMultipleObjects(0, NULL, FALSE, INFINITE,
                                                     QS_ALLINPUT);
            if (result != WAIT_OBJECT_0) {
                NS_ERROR("Wait failed!");
                break;
            }

            
            
            
            
            
            
            
            
            bool haveSentMessagesPending = 
                HIWORD(GetQueueStatus(QS_SENDMESSAGE)) & QS_SENDMESSAGE;

            
            
            
            
            
            

            
            
            
            MSG msg = { 0 };
            if (PeekMessageW(&msg, (HWND)-1, gEventLoopMessage,
                             gEventLoopMessage, PM_REMOVE)) {
                break;
            }

            
            
            
            
            
            
            if (!PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE) &&
                !haveSentMessagesPending) {
                
                SwitchToThread();
            }
        }
    }

    NS_ASSERTION(mEventLoopDepth > 0, "Event loop depth mismatch!");

    if (--mEventLoopDepth == 0) {
        if (windowHook) {
            UnhookWindowsHookEx(windowHook);
        }

        NS_ASSERTION(gNeuteredWindows == &neuteredWindows, "Bad pointer!");
        gNeuteredWindows = nsnull;

        PRUint32 count = neuteredWindows.Length();
        for (PRUint32 index = 0; index < count; index++) {
            RestoreWindowProcedure(neuteredWindows[index]);
        }

        gPumpingMessages = false;
    }
}

void
SyncChannel::WaitForNotify()
{
    RunWindowsEventLoop();
}

void
SyncChannel::NotifyWorkerThread()
{
    mMutex.AssertCurrentThreadOwns();
    NS_ASSERTION(mUIThreadId, "This should have been set already!");
    if (!PostThreadMessage(mUIThreadId, gEventLoopMessage, 0, 0)) {
        NS_WARNING("Failed to post thread message!");
    }
}



#else

void
SyncChannel::WaitForNotify()
{
    mCvar.Wait();
}

void
SyncChannel::NotifyWorkerThread()
{
    mCvar.Notify();
}

#endif  


} 
} 
