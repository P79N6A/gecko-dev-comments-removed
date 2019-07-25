





































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "nsSocketTransportService2.h"
#include "nsSocketTransport2.h"
#include "nsReadableUtils.h"
#include "nsNetError.h"
#include "prnetdb.h"
#include "prerror.h"
#include "plstr.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsServiceManagerUtils.h"
#include "nsIOService.h"

#include "mozilla/FunctionTimer.h"

using namespace mozilla;

#if defined(PR_LOGGING)
PRLogModuleInfo *gSocketTransportLog = nsnull;
#endif

nsSocketTransportService *gSocketTransportService = nsnull;
PRThread                 *gSocketThread           = nsnull;

#define SEND_BUFFER_PREF "network.tcp.sendbuffer"
#define SOCKET_LIMIT_TARGET 550U
#define SOCKET_LIMIT_MIN     50U




nsSocketTransportService::nsSocketTransportService()
    : mThread(nsnull)
    , mThreadEvent(nsnull)
    , mAutodialEnabled(PR_FALSE)
    , mLock("nsSocketTransportService::mLock")
    , mInitialized(PR_FALSE)
    , mShuttingDown(PR_FALSE)
    , mActiveListSize(SOCKET_LIMIT_MIN)
    , mIdleListSize(SOCKET_LIMIT_MIN)
    , mActiveCount(0)
    , mIdleCount(0)
    , mSendBufferSize(0)
{
#if defined(PR_LOGGING)
    gSocketTransportLog = PR_NewLogModule("nsSocketTransport");
#endif

    NS_ASSERTION(NS_IsMainThread(), "wrong thread");

    PR_CallOnce(&gMaxCountInitOnce, DiscoverMaxCount);
    mActiveList = (SocketContext *)
        moz_xmalloc(sizeof(SocketContext) * mActiveListSize);
    mIdleList = (SocketContext *)
        moz_xmalloc(sizeof(SocketContext) * mIdleListSize);
    mPollList = (PRPollDesc *)
        moz_xmalloc(sizeof(PRPollDesc) * (mActiveListSize + 1));

    NS_ASSERTION(!gSocketTransportService, "must not instantiate twice");
    gSocketTransportService = this;
}

nsSocketTransportService::~nsSocketTransportService()
{
    NS_ASSERTION(NS_IsMainThread(), "wrong thread");
    NS_ASSERTION(!mInitialized, "not shutdown properly");
    
    if (mThreadEvent)
        PR_DestroyPollableEvent(mThreadEvent);

    moz_free(mActiveList);
    moz_free(mIdleList);
    moz_free(mPollList);
    gSocketTransportService = nsnull;
}




already_AddRefed<nsIThread>
nsSocketTransportService::GetThreadSafely()
{
    MutexAutoLock lock(mLock);
    nsIThread* result = mThread;
    NS_IF_ADDREF(result);
    return result;
}

NS_IMETHODIMP
nsSocketTransportService::Dispatch(nsIRunnable *event, PRUint32 flags)
{
    SOCKET_LOG(("STS dispatch [%p]\n", event));

    nsCOMPtr<nsIThread> thread = GetThreadSafely();
    NS_ENSURE_TRUE(thread, NS_ERROR_NOT_INITIALIZED);
    nsresult rv = thread->Dispatch(event, flags);
    if (rv == NS_ERROR_UNEXPECTED) {
        
        
        rv = NS_ERROR_NOT_INITIALIZED;
    }
    return rv;
}

NS_IMETHODIMP
nsSocketTransportService::IsOnCurrentThread(PRBool *result)
{
    nsCOMPtr<nsIThread> thread = GetThreadSafely();
    NS_ENSURE_TRUE(thread, NS_ERROR_NOT_INITIALIZED);
    return thread->IsOnCurrentThread(result);
}




NS_IMETHODIMP
nsSocketTransportService::NotifyWhenCanAttachSocket(nsIRunnable *event)
{
    SOCKET_LOG(("nsSocketTransportService::NotifyWhenCanAttachSocket\n"));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (CanAttachSocket()) {
        return Dispatch(event, NS_DISPATCH_NORMAL);
    }

    mPendingSocketQ.PutEvent(event);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::AttachSocket(PRFileDesc *fd, nsASocketHandler *handler)
{
    SOCKET_LOG(("nsSocketTransportService::AttachSocket [handler=%x]\n", handler));

    NS_ASSERTION(PR_GetCurrentThread() == gSocketThread, "wrong thread");

    if (!CanAttachSocket()) {
        return NS_ERROR_NOT_AVAILABLE;
    }

    SocketContext sock;
    sock.mFD = fd;
    sock.mHandler = handler;
    sock.mElapsedTime = 0;

    nsresult rv = AddToIdleList(&sock);
    if (NS_SUCCEEDED(rv))
        NS_ADDREF(handler);
    return rv;
}

nsresult
nsSocketTransportService::DetachSocket(SocketContext *listHead, SocketContext *sock)
{
    SOCKET_LOG(("nsSocketTransportService::DetachSocket [handler=%x]\n", sock->mHandler));
    NS_ABORT_IF_FALSE((listHead == mActiveList) || (listHead == mIdleList),
                      "DetachSocket invalid head");

    
    sock->mHandler->OnSocketDetached(sock->mFD);

    
    sock->mFD = nsnull;
    NS_RELEASE(sock->mHandler);

    if (listHead == mActiveList)
        RemoveFromPollList(sock);
    else
        RemoveFromIdleList(sock);

    
    
    
    
    
    nsCOMPtr<nsIRunnable> event;
    if (mPendingSocketQ.GetPendingEvent(getter_AddRefs(event))) {
        
        return Dispatch(event, NS_DISPATCH_NORMAL);
    }
    return NS_OK;
}

nsresult
nsSocketTransportService::AddToPollList(SocketContext *sock)
{
    NS_ABORT_IF_FALSE(!(((PRUint32)(sock - mActiveList)) < mActiveListSize),
                      "AddToPollList Socket Already Active");

    SOCKET_LOG(("nsSocketTransportService::AddToPollList [handler=%x]\n", sock->mHandler));
    if (mActiveCount == mActiveListSize) {
        SOCKET_LOG(("  Active List size of %d met\n", mActiveCount));
        if (!GrowActiveList()) {
            NS_ERROR("too many active sockets");
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }
    
    mActiveList[mActiveCount] = *sock;
    mActiveCount++;

    mPollList[mActiveCount].fd = sock->mFD;
    mPollList[mActiveCount].in_flags = sock->mHandler->mPollFlags;
    mPollList[mActiveCount].out_flags = 0;

    SOCKET_LOG(("  active=%u idle=%u\n", mActiveCount, mIdleCount));
    return NS_OK;
}

void
nsSocketTransportService::RemoveFromPollList(SocketContext *sock)
{
    SOCKET_LOG(("nsSocketTransportService::RemoveFromPollList [handler=%x]\n", sock->mHandler));

    PRUint32 index = sock - mActiveList;
    NS_ABORT_IF_FALSE(index < mActiveListSize, "invalid index");

    SOCKET_LOG(("  index=%u mActiveCount=%u\n", index, mActiveCount));

    if (index != mActiveCount-1) {
        mActiveList[index] = mActiveList[mActiveCount-1];
        mPollList[index+1] = mPollList[mActiveCount];
    }
    mActiveCount--;

    SOCKET_LOG(("  active=%u idle=%u\n", mActiveCount, mIdleCount));
}

nsresult
nsSocketTransportService::AddToIdleList(SocketContext *sock)
{
    NS_ABORT_IF_FALSE(!(((PRUint32)(sock - mIdleList)) < mIdleListSize),
                      "AddToIdlelList Socket Already Idle");

    SOCKET_LOG(("nsSocketTransportService::AddToIdleList [handler=%x]\n", sock->mHandler));
    if (mIdleCount == mIdleListSize) {
        SOCKET_LOG(("  Idle List size of %d met\n", mIdleCount));
        if (!GrowIdleList()) {
            NS_ERROR("too many idle sockets");
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }

    mIdleList[mIdleCount] = *sock;
    mIdleCount++;

    SOCKET_LOG(("  active=%u idle=%u\n", mActiveCount, mIdleCount));
    return NS_OK;
}

void
nsSocketTransportService::RemoveFromIdleList(SocketContext *sock)
{
    SOCKET_LOG(("nsSocketTransportService::RemoveFromIdleList [handler=%x]\n", sock->mHandler));

    PRUint32 index = sock - mIdleList;
    NS_ASSERTION(index < mIdleListSize, "invalid index in idle list");

    if (index != mIdleCount-1)
        mIdleList[index] = mIdleList[mIdleCount-1];
    mIdleCount--;

    SOCKET_LOG(("  active=%u idle=%u\n", mActiveCount, mIdleCount));
}

void
nsSocketTransportService::MoveToIdleList(SocketContext *sock)
{
    nsresult rv = AddToIdleList(sock);
    if (NS_FAILED(rv))
        DetachSocket(mActiveList, sock);
    else
        RemoveFromPollList(sock);
}

void
nsSocketTransportService::MoveToPollList(SocketContext *sock)
{
    nsresult rv = AddToPollList(sock);
    if (NS_FAILED(rv))
        DetachSocket(mIdleList, sock);
    else
        RemoveFromIdleList(sock);
}

PRBool
nsSocketTransportService::GrowActiveList()
{
    PRInt32 toAdd = gMaxCount - mActiveListSize;
    if (toAdd > 100)
        toAdd = 100;
    if (toAdd < 1)
        return PR_FALSE;
    
    mActiveListSize += toAdd;
    mActiveList = (SocketContext *)
        moz_xrealloc(mActiveList, sizeof(SocketContext) * mActiveListSize);
    mPollList = (PRPollDesc *)
        moz_xrealloc(mPollList, sizeof(PRPollDesc) * (mActiveListSize + 1));
    return PR_TRUE;
}

PRBool
nsSocketTransportService::GrowIdleList()
{
    PRInt32 toAdd = gMaxCount - mIdleListSize;
    if (toAdd > 100)
        toAdd = 100;
    if (toAdd < 1)
        return PR_FALSE;

    mIdleListSize += toAdd;
    mIdleList = (SocketContext *)
        moz_xrealloc(mIdleList, sizeof(SocketContext) * mIdleListSize);
    return PR_TRUE;
}

PRIntervalTime
nsSocketTransportService::PollTimeout()
{
    if (mActiveCount == 0)
        return NS_SOCKET_POLL_TIMEOUT;

    
    PRUint32 minR = PR_UINT16_MAX;
    for (PRUint32 i=0; i<mActiveCount; ++i) {
        const SocketContext &s = mActiveList[i];
        
        
        PRUint32 r = (s.mElapsedTime < s.mHandler->mPollTimeout)
          ? s.mHandler->mPollTimeout - s.mElapsedTime
          : 0;
        if (r < minR)
            minR = r;
    }
    SOCKET_LOG(("poll timeout: %lu\n", minR));
    return PR_SecondsToInterval(minR);
}

PRInt32
nsSocketTransportService::Poll(PRBool wait, PRUint32 *interval)
{
    PRPollDesc *pollList;
    PRUint32 pollCount;
    PRIntervalTime pollTimeout;

    if (mPollList[0].fd) {
        mPollList[0].out_flags = 0;
        pollList = mPollList;
        pollCount = mActiveCount + 1;
        pollTimeout = PollTimeout();
    }
    else {
        
        pollCount = mActiveCount;
        if (pollCount)
            pollList = &mPollList[1];
        else
            pollList = nsnull;
        pollTimeout = PR_MillisecondsToInterval(25);
    }

    if (!wait)
        pollTimeout = PR_INTERVAL_NO_WAIT;

    PRIntervalTime ts = PR_IntervalNow();

    SOCKET_LOG(("    timeout = %i milliseconds\n",
         PR_IntervalToMilliseconds(pollTimeout)));
    PRInt32 rv = PR_Poll(pollList, pollCount, pollTimeout);

    PRIntervalTime passedInterval = PR_IntervalNow() - ts;

    SOCKET_LOG(("    ...returned after %i milliseconds\n",
         PR_IntervalToMilliseconds(passedInterval))); 

    *interval = PR_IntervalToSeconds(passedInterval);
    return rv;
}




NS_IMPL_THREADSAFE_ISUPPORTS6(nsSocketTransportService,
                              nsISocketTransportService,
                              nsIEventTarget,
                              nsIThreadObserver,
                              nsIRunnable,
                              nsPISocketTransportService,
                              nsIObserver)


NS_IMETHODIMP
nsSocketTransportService::Init()
{
    NS_TIME_FUNCTION;

    if (!NS_IsMainThread()) {
        NS_ERROR("wrong thread");
        return NS_ERROR_UNEXPECTED;
    }

    if (mInitialized)
        return NS_OK;

    if (mShuttingDown)
        return NS_ERROR_UNEXPECTED;

    
    if (gIOService->IsOffline() && !gIOService->IsComingOnline())
        return NS_ERROR_OFFLINE;

    if (!mThreadEvent) {
        mThreadEvent = PR_NewPollableEvent();
        
        
        
        
        
        
        
        
        
        
        if (!mThreadEvent) {
            NS_WARNING("running socket transport thread without a pollable event");
            SOCKET_LOG(("running socket transport thread without a pollable event"));
        }
    }
    
    NS_TIME_FUNCTION_MARK("Created thread");

    nsCOMPtr<nsIThread> thread;
    nsresult rv = NS_NewThread(getter_AddRefs(thread), this);
    if (NS_FAILED(rv)) return rv;
    
    {
        MutexAutoLock lock(mLock);
        
        thread.swap(mThread);
    }

    nsCOMPtr<nsIPrefBranch2> tmpPrefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (tmpPrefService) 
        tmpPrefService->AddObserver(SEND_BUFFER_PREF, this, PR_FALSE);
    UpdatePrefs();

    NS_TIME_FUNCTION_MARK("UpdatePrefs");

    mInitialized = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
nsSocketTransportService::Shutdown()
{
    SOCKET_LOG(("nsSocketTransportService::Shutdown\n"));

    NS_ENSURE_STATE(NS_IsMainThread());

    if (!mInitialized)
        return NS_OK;

    if (mShuttingDown)
        return NS_ERROR_UNEXPECTED;

    {
        MutexAutoLock lock(mLock);

        
        mShuttingDown = PR_TRUE;

        if (mThreadEvent)
            PR_SetPollableEvent(mThreadEvent);
        
    }

    
    mThread->Shutdown();
    {
        MutexAutoLock lock(mLock);
        
        
        mThread = nsnull;
    }

    nsCOMPtr<nsIPrefBranch2> tmpPrefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (tmpPrefService) 
        tmpPrefService->RemoveObserver(SEND_BUFFER_PREF, this);

    mInitialized = PR_FALSE;
    mShuttingDown = PR_FALSE;

    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::CreateTransport(const char **types,
                                          PRUint32 typeCount,
                                          const nsACString &host,
                                          PRInt32 port,
                                          nsIProxyInfo *proxyInfo,
                                          nsISocketTransport **result)
{
    NS_ENSURE_TRUE(mInitialized, NS_ERROR_OFFLINE);
    NS_ENSURE_TRUE(port >= 0 && port <= 0xFFFF, NS_ERROR_ILLEGAL_VALUE);

    nsSocketTransport *trans = new nsSocketTransport();
    if (!trans)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(trans);

    nsresult rv = trans->Init(types, typeCount, host, port, proxyInfo);
    if (NS_FAILED(rv)) {
        NS_RELEASE(trans);
        return rv;
    }

    *result = trans;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::GetAutodialEnabled(PRBool *value)
{
    *value = mAutodialEnabled;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::SetAutodialEnabled(PRBool value)
{
    mAutodialEnabled = value;
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::OnDispatchedEvent(nsIThreadInternal *thread)
{
    MutexAutoLock lock(mLock);
    if (mThreadEvent)
        PR_SetPollableEvent(mThreadEvent);
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::OnProcessNextEvent(nsIThreadInternal *thread,
                                             PRBool mayWait, PRUint32 depth)
{
    
    
    
    if (depth > 1)
        return NS_OK;

    
    DoPollIteration(PR_FALSE);

    PRBool val;
    while (mayWait && NS_SUCCEEDED(thread->HasPendingEvents(&val)) && !val)
        DoPollIteration(PR_TRUE);

    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::AfterProcessNextEvent(nsIThreadInternal* thread,
                                                PRUint32 depth)
{
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::Run()
{
    SOCKET_LOG(("STS thread init\n"));

    gSocketThread = PR_GetCurrentThread();

    
    mPollList[0].fd = mThreadEvent;
    mPollList[0].in_flags = PR_POLL_READ;
    mPollList[0].out_flags = 0;

    nsIThread *thread = NS_GetCurrentThread();

    
    nsCOMPtr<nsIThreadInternal> threadInt = do_QueryInterface(thread);
    threadInt->SetObserver(this);

    for (;;) {
        
        NS_ProcessPendingEvents(thread);

        
        {
            MutexAutoLock lock(mLock);
            if (mShuttingDown)
                break;
        }

        
        NS_ProcessNextEvent(thread);
    }

    SOCKET_LOG(("STS shutting down thread\n"));

    
    PRInt32 i;
    for (i=mActiveCount-1; i>=0; --i)
        DetachSocket(mActiveList, &mActiveList[i]);
    for (i=mIdleCount-1; i>=0; --i)
        DetachSocket(mIdleList, &mIdleList[i]);

    
    
    NS_ProcessPendingEvents(thread);

    gSocketThread = nsnull;

    SOCKET_LOG(("STS thread exit\n"));
    return NS_OK;
}

nsresult
nsSocketTransportService::DoPollIteration(PRBool wait)
{
    SOCKET_LOG(("STS poll iter [%d]\n", wait));

    PRInt32 i, count;

    
    
    
    PRBool pollError = PR_FALSE;

    
    
    
    
    
    
    count = mIdleCount;
    for (i=mActiveCount-1; i>=0; --i) {
        
        SOCKET_LOG(("  active [%u] { handler=%x condition=%x pollflags=%hu }\n", i,
            mActiveList[i].mHandler,
            mActiveList[i].mHandler->mCondition,
            mActiveList[i].mHandler->mPollFlags));
        
        if (NS_FAILED(mActiveList[i].mHandler->mCondition))
            DetachSocket(mActiveList, &mActiveList[i]);
        else {
            PRUint16 in_flags = mActiveList[i].mHandler->mPollFlags;
            if (in_flags == 0)
                MoveToIdleList(&mActiveList[i]);
            else {
                
                mPollList[i+1].in_flags = in_flags;
                mPollList[i+1].out_flags = 0;
            }
        }
    }
    for (i=count-1; i>=0; --i) {
        
        SOCKET_LOG(("  idle [%u] { handler=%x condition=%x pollflags=%hu }\n", i,
            mIdleList[i].mHandler,
            mIdleList[i].mHandler->mCondition,
            mIdleList[i].mHandler->mPollFlags));
        
        if (NS_FAILED(mIdleList[i].mHandler->mCondition))
            DetachSocket(mIdleList, &mIdleList[i]);
        else if (mIdleList[i].mHandler->mPollFlags != 0)
            MoveToPollList(&mIdleList[i]);
    }

    SOCKET_LOG(("  calling PR_Poll [active=%u idle=%u]\n", mActiveCount, mIdleCount));

    
    PRUint32 pollInterval;

    PRInt32 n = Poll(wait, &pollInterval);
    if (n < 0) {
        SOCKET_LOG(("  PR_Poll error [%d]\n", PR_GetError()));
        pollError = PR_TRUE;
    }
    else {
        
        
        
        for (i=0; i<PRInt32(mActiveCount); ++i) {
            PRPollDesc &desc = mPollList[i+1];
            SocketContext &s = mActiveList[i];
            if (n > 0 && desc.out_flags != 0) {
                s.mElapsedTime = 0;
                s.mHandler->OnSocketReady(desc.fd, desc.out_flags);
            }
            
            else if (s.mHandler->mPollTimeout != PR_UINT16_MAX) {
                
                if (NS_UNLIKELY(pollInterval > (PR_UINT16_MAX - s.mElapsedTime)))
                    s.mElapsedTime = PR_UINT16_MAX;
                else
                    s.mElapsedTime += PRUint16(pollInterval);
                
                if (s.mElapsedTime >= s.mHandler->mPollTimeout) {
                    s.mElapsedTime = 0;
                    s.mHandler->OnSocketReady(desc.fd, -1);
                }
            }
        }

        
        
        
        
        for (i=mActiveCount-1; i>=0; --i) {
            if (NS_FAILED(mActiveList[i].mHandler->mCondition))
                DetachSocket(mActiveList, &mActiveList[i]);
        }

        if (n != 0 && mPollList[0].out_flags == PR_POLL_READ) {
            
            if (PR_WaitForPollableEvent(mThreadEvent) != PR_SUCCESS) {
                
                
                
                
                
                
                {
                    MutexAutoLock lock(mLock);
                    PR_DestroyPollableEvent(mThreadEvent);
                    mThreadEvent = PR_NewPollableEvent();
                }
                if (!mThreadEvent) {
                    NS_WARNING("running socket transport thread without "
                               "a pollable event");
                    SOCKET_LOG(("running socket transport thread without "
                         "a pollable event"));
                }
                mPollList[0].fd = mThreadEvent;
                
                
                mPollList[0].out_flags = 0;
            }
        }
    }

    return NS_OK;
}

nsresult
nsSocketTransportService::UpdatePrefs()
{
    mSendBufferSize = 0;
    
    nsCOMPtr<nsIPrefBranch2> tmpPrefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (tmpPrefService) {
        PRInt32 bufferSize;
        nsresult rv = tmpPrefService->GetIntPref(SEND_BUFFER_PREF, &bufferSize);
        if (NS_SUCCEEDED(rv) && bufferSize > 0)
            mSendBufferSize = bufferSize;
    }
    
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::Observe(nsISupports *subject,
                                  const char *topic,
                                  const PRUnichar *data)
{
    if (!strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
        UpdatePrefs();
    }
    return NS_OK;
}

NS_IMETHODIMP
nsSocketTransportService::GetSendBufferSize(PRInt32 *value)
{
    *value = mSendBufferSize;
    return NS_OK;
}




#if defined(XP_WIN)
#include <windows.h>
#elif defined(XP_UNIX) && !defined(AIX) && !defined(NEXTSTEP) && !defined(QNX)
#include <sys/resource.h>
#endif

PRStatus
nsSocketTransportService::DiscoverMaxCount()
{
    gMaxCount = SOCKET_LIMIT_MIN;

#if defined(XP_UNIX) && !defined(AIX) && !defined(NEXTSTEP) && !defined(QNX)
    
    
    
    
    
    

    struct rlimit rlimitData;
    if (getrlimit(RLIMIT_NOFILE, &rlimitData) == -1)
        return PR_SUCCESS;
    if (rlimitData.rlim_cur >=  SOCKET_LIMIT_TARGET + 250) {
        gMaxCount = SOCKET_LIMIT_TARGET;
        return PR_SUCCESS;
    }

    PRInt32 maxallowed = rlimitData.rlim_max;
    if (maxallowed == -1) {                       
        maxallowed = SOCKET_LIMIT_TARGET + 250;
    } else if ((PRUint32)maxallowed < SOCKET_LIMIT_MIN + 250) {
        return PR_SUCCESS;
    } else if ((PRUint32)maxallowed > SOCKET_LIMIT_TARGET + 250) {
        maxallowed = SOCKET_LIMIT_TARGET + 250;
    }

    rlimitData.rlim_cur = maxallowed;
    setrlimit(RLIMIT_NOFILE, &rlimitData);
    if (getrlimit(RLIMIT_NOFILE, &rlimitData) != -1)
        if (rlimitData.rlim_cur > SOCKET_LIMIT_MIN + 250)
            gMaxCount = rlimitData.rlim_cur - 250;

#elif defined(XP_WIN) && !defined(WIN_CE)
    
    
    

    OSVERSIONINFO osInfo = { sizeof(OSVERSIONINFO) };
    if (GetVersionEx(&osInfo)) {
        PRInt32 version = 
            (osInfo.dwMajorVersion & 0xff) << 8 | 
            (osInfo.dwMinorVersion & 0xff);
        if (version >= 0x501)                    
            gMaxCount = SOCKET_LIMIT_TARGET;
    }
#else
    
#endif

    return PR_SUCCESS;
}
