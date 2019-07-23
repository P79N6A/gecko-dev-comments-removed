





































#ifndef nsSocketTransportService2_h__
#define nsSocketTransportService2_h__

#include "nsPISocketTransportService.h"
#include "nsIThreadInternal.h"
#include "nsThreadUtils.h"
#include "nsEventQueue.h"
#include "nsCOMPtr.h"
#include "pldhash.h"
#include "prinrval.h"
#include "prlog.h"
#include "prio.h"



#if defined(PR_LOGGING)



extern PRLogModuleInfo *gSocketTransportLog;
#endif
#define LOG(args)     PR_LOG(gSocketTransportLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gSocketTransportLog, PR_LOG_DEBUG)



#define NS_SOCKET_MAX_COUNT    50
#define NS_SOCKET_POLL_TIMEOUT PR_INTERVAL_NO_TIMEOUT




class nsASocketHandler : public nsISupports
{
public:
    nsASocketHandler()
        : mCondition(NS_OK)
        , mPollFlags(0)
        , mPollTimeout(PR_UINT16_MAX)
        {}

    
    
    
    
    
    nsresult mCondition;

    
    
    
    
    
    PRUint16 mPollFlags;

    
    
    
    
    
    
    
    
    
    PRUint16 mPollTimeout;

    
    
    
    
    
    
    
    
    
    virtual void OnSocketReady(PRFileDesc *fd, PRInt16 outFlags) = 0;

    
    
    
    
    
    
    virtual void OnSocketDetached(PRFileDesc *fd) = 0;
};



class nsSocketTransportService : public nsPISocketTransportService
                               , public nsIEventTarget
                               , public nsIThreadObserver
                               , public nsIRunnable
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSPISOCKETTRANSPORTSERVICE
    NS_DECL_NSISOCKETTRANSPORTSERVICE
    NS_DECL_NSIEVENTTARGET
    NS_DECL_NSITHREADOBSERVER
    NS_DECL_NSIRUNNABLE

    nsSocketTransportService();

    
    
    
    
    
    
    
    PRBool CanAttachSocket() {
        return mActiveCount + mIdleCount < NS_SOCKET_MAX_COUNT;
    }

    
    
    
    
    
    
    nsresult NotifyWhenCanAttachSocket(nsIRunnable *);

    
    
    
    
    
    
    
    
    nsresult AttachSocket(PRFileDesc *fd, nsASocketHandler *);

protected:

    virtual ~nsSocketTransportService();

private:

    
    
    

    nsIThread  *mThread;
    PRFileDesc *mThreadEvent;
    PRBool      mAutodialEnabled;
                            

    
    
    

    PRLock       *mLock;
    PRPackedBool  mInitialized;
    PRPackedBool  mShuttingDown;
                            
                            

    
    
    
    
    
    
    
    
    
    

    struct SocketContext
    {
        PRFileDesc       *mFD;
        nsASocketHandler *mHandler;
        PRUint16          mElapsedTime;  
    };

    SocketContext mActiveList [ NS_SOCKET_MAX_COUNT ];
    SocketContext mIdleList   [ NS_SOCKET_MAX_COUNT ];

    PRUint32 mActiveCount;
    PRUint32 mIdleCount;

    nsresult DetachSocket(SocketContext *);
    nsresult AddToIdleList(SocketContext *);
    nsresult AddToPollList(SocketContext *);
    void RemoveFromIdleList(SocketContext *);
    void RemoveFromPollList(SocketContext *);
    void MoveToIdleList(SocketContext *sock);
    void MoveToPollList(SocketContext *sock);
    
    
    
    
    
    
    

    PRPollDesc mPollList[ NS_SOCKET_MAX_COUNT + 1 ];

    PRIntervalTime PollTimeout();            
    nsresult       DoPollIteration(PRBool wait);
                                             
    PRInt32        Poll(PRBool wait, PRUint32 *interval);
                                             
                                             
                                             

    
    
    

    nsEventQueue mPendingSocketQ; 
};

extern nsSocketTransportService *gSocketTransportService;
extern PRThread                 *gSocketThread;

#endif 
