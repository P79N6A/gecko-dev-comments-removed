





































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "nsSocketTransportService2.h"
#include "nsSocketTransport2.h"
#include "nsReadableUtils.h"
#include "nsAutoLock.h"
#include "nsNetError.h"
#include "prnetdb.h"
#include "prlock.h"
#include "prerror.h"
#include "plstr.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsServiceManagerUtils.h"

#if defined(PR_LOGGING)
PRLogModuleInfo *gSocketTransportLog = nsnull;
#endif

nsSocketTransportService *gSocketTransportService = nsnull;
PRThread                 *gSocketThread           = nsnull;

#define SEND_BUFFER_PREF "network.tcp.sendbuffer"




nsSocketTransportService::nsSocketTransportService()
    : mThread(nsnull)
    , mThreadEvent(nsnull)
    , mAutodialEnabled(PR_FALSE)
    , mLock(PR_NewLock())
    , mInitialized(PR_FALSE)
    , mShuttingDown(PR_FALSE)
    , mActiveCount(0)
    , mIdleCount(0)
    , mSendBufferSize(0)
{
#if defined(PR_LOGGING)
    gSocketTransportLog = PR_NewLogModule("nsSocketTransport");
#endif

    NS_ASSERTION(NS_IsMainThread(), "wrong thread");

    NS_ASSERTION(!gSocketTransportService, "must not instantiate twice");
    gSocketTransportService = this;
}

nsSocketTransportService::~nsSocketTransportService()
{
    NS_ASSERTION(NS_IsMainThread(), "wrong thread");
    NS_ASSERTION(!mInitialized, "not shutdown properly");

    if (mLock)
        PR_DestroyLock(mLock);
    
    if (mThreadEvent)
        PR_DestroyPollableEvent(mThreadEvent);

    gSocketTransportService = nsnull;
}




already_AddRefed<nsIThread>
nsSocketTransportService::GetThreadSafely()
{
    nsAutoLock lock(mLock);
    nsIThread* result = mThread;
    NS_IF_ADDREF(result);
    return result;
}

NS_IMETHODIMP
nsSocketTransportService::Dispatch(nsIRunnable *event, PRUint32 flags)
{
    LOG(("STS dispatch [%p]\n", event));

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
    LOG(("nsSocketTransportService::NotifyWhenCanAttachSocket\n"));

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
    LOG(("nsSocketTransportService::AttachSocket [handler=%x]\n", handler));

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
nsSocketTransportService::DetachSocket(SocketContext *sock)
{
    LOG(("nsSocketTransportService::DetachSocket [handler=%x]\n", sock->mHandler));

    
    sock->mHandler->OnSocketDetached(sock->mFD);

    
    sock->mFD = nsnull;
    NS_RELEASE(sock->mHandler);

    
    PRUint32 index = sock - mActiveList;
    if (index < NS_SOCKET_MAX_COUNT)
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
    LOG(("nsSocketTransportService::AddToPollList [handler=%x]\n", sock->mHandler));

    if (mActiveCount == NS_SOCKET_MAX_COUNT) {
        NS_ERROR("too many active sockets");
        return NS_ERROR_UNEXPECTED;
    }

    mActiveList[mActiveCount] = *sock;
    mActiveCount++;

    mPollList[mActiveCount].fd = sock->mFD;
    mPollList[mActiveCount].in_flags = sock->mHandler->mPollFlags;
    mPollList[mActiveCount].out_flags = 0;

    LOG(("  active=%u idle=%u\n", mActiveCount, mIdleCount));
    return NS_OK;
}

void
nsSocketTransportService::RemoveFromPollList(SocketContext *sock)
{
    LOG(("nsSocketTransportService::RemoveFromPollList [handler=%x]\n", sock->mHandler));

    PRUint32 index = sock - mActiveList;
    NS_ASSERTION(index < NS_SOCKET_MAX_COUNT, "invalid index");

    LOG(("  index=%u mActiveCount=%u\n", index, mActiveCount));

    if (index != mActiveCount-1) {
        mActiveList[index] = mActiveList[mActiveCount-1];
        mPollList[index+1] = mPollList[mActiveCount];
    }
    mActiveCount--;

    LOG(("  active=%u idle=%u\n", mActiveCount, mIdleCount));
}

nsresult
nsSocketTransportService::AddToIdleList(SocketContext *sock)
{
    LOG(("nsSocketTransportService::AddToIdleList [handler=%x]\n", sock->mHandler));

    if (mIdleCount == NS_SOCKET_MAX_COUNT) {
        NS_ERROR("too many idle sockets");
        return NS_ERROR_UNEXPECTED;
    }

    mIdleList[mIdleCount] = *sock;
    mIdleCount++;

    LOG(("  active=%u idle=%u\n", mActiveCount, mIdleCount));
    return NS_OK;
}

void
nsSocketTransportService::RemoveFromIdleList(SocketContext *sock)
{
    LOG(("nsSocketTransportService::RemoveFromIdleList [handler=%x]\n", sock->mHandler));

    PRUint32 index = sock - &mIdleList[0];
    NS_ASSERTION(index < NS_SOCKET_MAX_COUNT, "invalid index");

    if (index != mIdleCount-1)
        mIdleList[index] = mIdleList[mIdleCount-1];
    mIdleCount--;

    LOG(("  active=%u idle=%u\n", mActiveCount, mIdleCount));
}

void
nsSocketTransportService::MoveToIdleList(SocketContext *sock)
{
    nsresult rv = AddToIdleList(sock);
    if (NS_FAILED(rv))
        DetachSocket(sock);
    else
        RemoveFromPollList(sock);
}

void
nsSocketTransportService::MoveToPollList(SocketContext *sock)
{
    nsresult rv = AddToPollList(sock);
    if (NS_FAILED(rv))
        DetachSocket(sock);
    else
        RemoveFromIdleList(sock);
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
    LOG(("poll timeout: %lu\n", minR));
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

    LOG(("    timeout = %i milliseconds\n",
         PR_IntervalToMilliseconds(pollTimeout)));
    PRInt32 rv = PR_Poll(pollList, pollCount, pollTimeout);

    PRIntervalTime passedInterval = PR_IntervalNow() - ts;

    LOG(("    ...returned after %i milliseconds\n",
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
    NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

    if (!NS_IsMainThread()) {
        NS_ERROR("wrong thread");
        return NS_ERROR_UNEXPECTED;
    }

    if (mInitialized)
        return NS_OK;

    if (mShuttingDown)
        return NS_ERROR_UNEXPECTED;

    if (!mThreadEvent) {
        mThreadEvent = PR_NewPollableEvent();
        
        
        
        
        
        
        
        
        
        
        if (!mThreadEvent) {
            NS_WARNING("running socket transport thread without a pollable event");
            LOG(("running socket transport thread without a pollable event"));
        }
    }

    nsCOMPtr<nsIThread> thread;
    nsresult rv = NS_NewThread(getter_AddRefs(thread), this);
    if (NS_FAILED(rv)) return rv;
    
    {
        nsAutoLock lock(mLock);
        
        thread.swap(mThread);
    }

    nsCOMPtr<nsIPrefBranch2> tmpPrefService = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (tmpPrefService) 
        tmpPrefService->AddObserver(SEND_BUFFER_PREF, this, PR_FALSE);
    UpdatePrefs();

    mInitialized = PR_TRUE;
    return NS_OK;
}


NS_IMETHODIMP
nsSocketTransportService::Shutdown()
{
    LOG(("nsSocketTransportService::Shutdown\n"));

    NS_ENSURE_STATE(NS_IsMainThread());

    if (!mInitialized)
        return NS_OK;

    if (mShuttingDown)
        return NS_ERROR_UNEXPECTED;

    {
        nsAutoLock lock(mLock);

        
        mShuttingDown = PR_TRUE;

        if (mThreadEvent)
            PR_SetPollableEvent(mThreadEvent);
        
    }

    
    mThread->Shutdown();
    {
        nsAutoLock lock(mLock);
        
        
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
    nsAutoLock lock(mLock);
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
    LOG(("STS thread init\n"));

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
            nsAutoLock lock(mLock);
            if (mShuttingDown)
                break;
        }

        
        NS_ProcessNextEvent(thread);
    }

    LOG(("STS shutting down thread\n"));

    
    PRInt32 i;
    for (i=mActiveCount-1; i>=0; --i)
        DetachSocket(&mActiveList[i]);
    for (i=mIdleCount-1; i>=0; --i)
        DetachSocket(&mIdleList[i]);

    
    
    NS_ProcessPendingEvents(thread);

    gSocketThread = nsnull;

    LOG(("STS thread exit\n"));
    return NS_OK;
}

nsresult
nsSocketTransportService::DoPollIteration(PRBool wait)
{
    LOG(("STS poll iter [%d]\n", wait));

    PRInt32 i, count;

    
    
    
    PRBool pollError = PR_FALSE;

    
    
    
    
    
    
    count = mIdleCount;
    for (i=mActiveCount-1; i>=0; --i) {
        
        LOG(("  active [%u] { handler=%x condition=%x pollflags=%hu }\n", i,
            mActiveList[i].mHandler,
            mActiveList[i].mHandler->mCondition,
            mActiveList[i].mHandler->mPollFlags));
        
        if (NS_FAILED(mActiveList[i].mHandler->mCondition))
            DetachSocket(&mActiveList[i]);
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
        
        LOG(("  idle [%u] { handler=%x condition=%x pollflags=%hu }\n", i,
            mIdleList[i].mHandler,
            mIdleList[i].mHandler->mCondition,
            mIdleList[i].mHandler->mPollFlags));
        
        if (NS_FAILED(mIdleList[i].mHandler->mCondition))
            DetachSocket(&mIdleList[i]);
        else if (mIdleList[i].mHandler->mPollFlags != 0)
            MoveToPollList(&mIdleList[i]);
    }

    LOG(("  calling PR_Poll [active=%u idle=%u]\n", mActiveCount, mIdleCount));

    
    PRUint32 pollInterval;

    PRInt32 n = Poll(wait, &pollInterval);
    if (n < 0) {
        LOG(("  PR_Poll error [%d]\n", PR_GetError()));
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
                DetachSocket(&mActiveList[i]);
        }

        if (n != 0 && mPollList[0].out_flags == PR_POLL_READ) {
            
            if (PR_WaitForPollableEvent(mThreadEvent) != PR_SUCCESS) {
                
                
                
                
                
                
                {
                    nsAutoLock lock(mLock);
                    PR_DestroyPollableEvent(mThreadEvent);
                    mThreadEvent = PR_NewPollableEvent();
                }
                if (!mThreadEvent) {
                    NS_WARNING("running socket transport thread without "
                               "a pollable event");
                    LOG(("running socket transport thread without "
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


