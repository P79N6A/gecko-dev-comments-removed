



































#include "nsIEventTarget.h"
#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsIObserver.h"
#include "nsAutoLock.h"
#include "nsCOMPtr.h"
#include "prclist.h"
#include "prlog.h"

#if defined(PR_LOGGING)



static PRLogModuleInfo *gIOThreadPoolLog = nsnull;
#endif
#define LOG(args) PR_LOG(gIOThreadPoolLog, PR_LOG_DEBUG, args)


#define MAX_THREADS 4



#define IDLE_TIMEOUT PR_SecondsToInterval(60)

#define PLEVENT_FROM_LINK(_link) \
    ((PLEvent*) ((char*) (_link) - offsetof(PLEvent, link)))











class nsIOThreadPool : public nsIEventTarget
                     , public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIEVENTTARGET
    NS_DECL_NSIOBSERVER

    nsresult Init();
    void     Shutdown();

private:
    virtual ~nsIOThreadPool();

    PR_STATIC_CALLBACK(void) ThreadFunc(void *);

    
    PRLock    *mLock;
    PRCondVar *mIdleThreadCV;   
    PRCondVar *mExitThreadCV;   
    PRUint32   mNumThreads;     
    PRUint32   mNumIdleThreads; 
    PRCList    mEventQ;         
    PRBool     mShutdown;       
};

NS_IMPL_THREADSAFE_ISUPPORTS2(nsIOThreadPool, nsIEventTarget, nsIObserver)

nsresult
nsIOThreadPool::Init()
{
#if defined(PR_LOGGING)
    if (!gIOThreadPoolLog)
        gIOThreadPoolLog = PR_NewLogModule("nsIOThreadPool");
#endif

    mNumThreads = 0;
    mNumIdleThreads = 0;
    mShutdown = PR_FALSE;

    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;

    mIdleThreadCV = PR_NewCondVar(mLock);
    if (!mIdleThreadCV)
        return NS_ERROR_OUT_OF_MEMORY;

    mExitThreadCV = PR_NewCondVar(mLock);
    if (!mExitThreadCV)
        return NS_ERROR_OUT_OF_MEMORY;

    PR_INIT_CLIST(&mEventQ);

    
    nsCOMPtr<nsIObserverService> os = do_GetService("@mozilla.org/observer-service;1");
    if (os)
        os->AddObserver(this, "xpcom-shutdown-threads", PR_FALSE);
    return NS_OK;
}

nsIOThreadPool::~nsIOThreadPool()
{
    LOG(("Destroying nsIOThreadPool @%p\n", this));

#ifdef DEBUG
    NS_ASSERTION(PR_CLIST_IS_EMPTY(&mEventQ), "leaking events");
    NS_ASSERTION(mNumThreads == 0, "leaking thread(s)");
#endif

    if (mIdleThreadCV)
        PR_DestroyCondVar(mIdleThreadCV);
    if (mExitThreadCV)
        PR_DestroyCondVar(mExitThreadCV);
    if (mLock)
        PR_DestroyLock(mLock);
}

void
nsIOThreadPool::Shutdown()
{
    LOG(("nsIOThreadPool::Shutdown\n"));

    
    {
        nsAutoLock lock(mLock);
        mShutdown = PR_TRUE;

        PR_NotifyAllCondVar(mIdleThreadCV);

        while (mNumThreads != 0)
            PR_WaitCondVar(mExitThreadCV, PR_INTERVAL_NO_TIMEOUT);
    }
}

NS_IMETHODIMP
nsIOThreadPool::PostEvent(PLEvent *event)
{
    LOG(("nsIOThreadPool::PostEvent [event=%p]\n", event));

    nsAutoLock lock(mLock);

    
    
    if (mShutdown)
        return NS_ERROR_UNEXPECTED;
    
    nsresult rv = NS_OK;

    PR_APPEND_LINK(&event->link, &mEventQ);

    
    if (mNumIdleThreads)
        PR_NotifyCondVar(mIdleThreadCV); 

    
    else if (mNumThreads < MAX_THREADS) {
        NS_ADDREF_THIS(); 
        mNumThreads++;
        PRThread *thread = PR_CreateThread(PR_USER_THREAD,
                                           ThreadFunc,
                                           this,
                                           PR_PRIORITY_NORMAL,
                                           PR_GLOBAL_THREAD,
                                           PR_UNJOINABLE_THREAD,
                                           0);
        if (!thread) {
            NS_RELEASE_THIS();
            mNumThreads--;
            rv = NS_ERROR_OUT_OF_MEMORY;
        }
    }
    

    return rv;
}

NS_IMETHODIMP
nsIOThreadPool::IsOnCurrentThread(PRBool *result)
{
    
    
    
    NS_NOTREACHED("nsIOThreadPool::IsOnCurrentThread");

    
    *result = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsIOThreadPool::Observe(nsISupports *, const char *topic, const PRUnichar *)
{
    NS_ASSERTION(strcmp(topic, "xpcom-shutdown-threads") == 0, "unexpected topic");
    Shutdown();
    return NS_OK;
}

void
nsIOThreadPool::ThreadFunc(void *arg)
{
    nsIOThreadPool *pool = (nsIOThreadPool *) arg;

    LOG(("entering ThreadFunc\n"));

    {
        nsAutoLock lock(pool->mLock);

        for (;;) {
            PRIntervalTime start = PR_IntervalNow(), timeout = IDLE_TIMEOUT;
            
            
            
            
            
            
            
            
            while (PR_CLIST_IS_EMPTY(&pool->mEventQ) && !pool->mShutdown) {
                pool->mNumIdleThreads++;
                PR_WaitCondVar(pool->mIdleThreadCV, timeout);
                pool->mNumIdleThreads--;

                PRIntervalTime delta = PR_IntervalNow() - start;
                if (delta >= timeout)
                    break;
                timeout -= delta;
                start += delta;
            }

            
            
            if (PR_CLIST_IS_EMPTY(&pool->mEventQ))
                break;

            
            
            do {
                PLEvent *event = PLEVENT_FROM_LINK(PR_LIST_HEAD(&pool->mEventQ));
                PR_REMOVE_AND_INIT_LINK(&event->link);

                LOG(("event:%p\n", event));

                
                lock.unlock();
                PL_HandleEvent(event);
                lock.lock();
            }
            while (!PR_CLIST_IS_EMPTY(&pool->mEventQ));
        }

        
        pool->mNumThreads--;
        PR_NotifyCondVar(pool->mExitThreadCV);
    }

    
    NS_RELEASE(pool);

    LOG(("leaving ThreadFunc\n"));
}



NS_METHOD
net_NewIOThreadPool(nsISupports *outer, REFNSIID iid, void **result)
{
    nsIOThreadPool *pool = new nsIOThreadPool();
    if (!pool)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(pool);
    nsresult rv = pool->Init();
    if (NS_SUCCEEDED(rv))
        rv = pool->QueryInterface(iid, result);
    NS_RELEASE(pool);
    return rv;
}
