




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
#include "prinit.h"
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

    
    
    static uint32_t gMaxCount;
    static PRCallOnceType gMaxCountInitOnce;
    static PRStatus DiscoverMaxCount();

    
    
    
    
    
    
    
    bool CanAttachSocket() {
        return mActiveCount + mIdleCount < gMaxCount;
    }

protected:

    virtual ~nsSocketTransportService();

private:

    
    
    

    nsCOMPtr<nsIThread> mThread;    
    PRFileDesc *mThreadEvent;
                            
                            
                            
                            
                            
                            
                            
    bool        mAutodialEnabled;
                            

    
    already_AddRefed<nsIThread> GetThreadSafely();

    
    
    

    Mutex         mLock;
    bool          mInitialized;
    bool          mShuttingDown;
                            
                            

    
    
    
    
    
    
    
    
    
    

    struct SocketContext
    {
        PRFileDesc       *mFD;
        nsASocketHandler *mHandler;
        uint16_t          mElapsedTime;  
    };

    SocketContext *mActiveList;                   
    SocketContext *mIdleList;                     

    uint32_t mActiveListSize;
    uint32_t mIdleListSize;
    uint32_t mActiveCount;
    uint32_t mIdleCount;

    nsresult DetachSocket(SocketContext *, SocketContext *);
    nsresult AddToIdleList(SocketContext *);
    nsresult AddToPollList(SocketContext *);
    void RemoveFromIdleList(SocketContext *);
    void RemoveFromPollList(SocketContext *);
    void MoveToIdleList(SocketContext *sock);
    void MoveToPollList(SocketContext *sock);

    bool GrowActiveList();
    bool GrowIdleList();
    void   InitMaxCount();
    
    
    
    
    
    
    

    PRPollDesc *mPollList;                        

    PRIntervalTime PollTimeout();            
    nsresult       DoPollIteration(bool wait);
                                             
    int32_t        Poll(bool wait, uint32_t *interval);
                                             
                                             
                                             

    
    
    

    nsEventQueue mPendingSocketQ; 

    
    nsresult    UpdatePrefs();
    int32_t     mSendBufferSize;

    
#if defined(XP_WIN)
    void ProbeMaxCount();
#endif
    bool mProbedMaxCount;
};

extern nsSocketTransportService *gSocketTransportService;
extern PRThread                 *gSocketThread;

#endif 
