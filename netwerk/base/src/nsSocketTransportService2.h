





































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
#include "nsASocketHandler.h"
#include "nsIObserver.h"
#include "mozilla/Mutex.h"



#if defined(PR_LOGGING)



extern PRLogModuleInfo *gSocketTransportLog;
#endif
#define SOCKET_LOG(args)     PR_LOG(gSocketTransportLog, PR_LOG_DEBUG, args)
#define SOCKET_LOG_ENABLED() PR_LOG_TEST(gSocketTransportLog, PR_LOG_DEBUG)



#define NS_SOCKET_POLL_TIMEOUT PR_INTERVAL_NO_TIMEOUT



class nsSocketTransportService : public nsPISocketTransportService
                               , public nsIEventTarget
                               , public nsIThreadObserver
                               , public nsIRunnable
                               , public nsIObserver
{
    typedef mozilla::Mutex Mutex;

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSPISOCKETTRANSPORTSERVICE
    NS_DECL_NSISOCKETTRANSPORTSERVICE
    NS_DECL_NSIEVENTTARGET
    NS_DECL_NSITHREADOBSERVER
    NS_DECL_NSIRUNNABLE
    NS_DECL_NSIOBSERVER 

    nsSocketTransportService();

    
    
    static PRUint32 gMaxCount;
    static PRCallOnceType gMaxCountInitOnce;
    static PRStatus DiscoverMaxCount();

    
    
    
    
    
    
    
    PRBool CanAttachSocket() {
        return mActiveCount + mIdleCount < gMaxCount;
    }

protected:

    virtual ~nsSocketTransportService();

private:

    
    
    

    nsCOMPtr<nsIThread> mThread;    
    PRFileDesc *mThreadEvent;
                            
                            
                            
                            
                            
                            
                            
    PRBool      mAutodialEnabled;
                            

    
    already_AddRefed<nsIThread> GetThreadSafely();

    
    
    

    Mutex         mLock;
    PRPackedBool  mInitialized;
    PRPackedBool  mShuttingDown;
                            
                            

    
    
    
    
    
    
    
    
    
    

    struct SocketContext
    {
        PRFileDesc       *mFD;
        nsASocketHandler *mHandler;
        PRUint16          mElapsedTime;  
    };

    SocketContext *mActiveList;                   
    SocketContext *mIdleList;                     

    PRUint32 mActiveListSize;
    PRUint32 mIdleListSize;
    PRUint32 mActiveCount;
    PRUint32 mIdleCount;

    nsresult DetachSocket(SocketContext *, SocketContext *);
    nsresult AddToIdleList(SocketContext *);
    nsresult AddToPollList(SocketContext *);
    void RemoveFromIdleList(SocketContext *);
    void RemoveFromPollList(SocketContext *);
    void MoveToIdleList(SocketContext *sock);
    void MoveToPollList(SocketContext *sock);

    PRBool GrowActiveList();
    PRBool GrowIdleList();
    void   InitMaxCount();
    
    
    
    
    
    
    

    PRPollDesc *mPollList;                        

    PRIntervalTime PollTimeout();            
    nsresult       DoPollIteration(PRBool wait);
                                             
    PRInt32        Poll(PRBool wait, PRUint32 *interval);
                                             
                                             
                                             

    
    
    

    nsEventQueue mPendingSocketQ; 

    
    nsresult    UpdatePrefs();
    PRInt32     mSendBufferSize;
};

extern nsSocketTransportService *gSocketTransportService;
extern PRThread                 *gSocketThread;

#endif 
