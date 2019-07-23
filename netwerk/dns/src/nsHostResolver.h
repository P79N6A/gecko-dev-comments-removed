




































#ifndef nsHostResolver_h__
#define nsHostResolver_h__

#include "nscore.h"
#include "pratom.h"
#include "prcvar.h"
#include "prclist.h"
#include "prnetdb.h"
#include "pldhash.h"
#include "nsISupportsImpl.h"

class nsHostResolver;
class nsHostRecord;
class nsResolveHostCallback;


#define NS_DECL_REFCOUNTED_THREADSAFE(classname)                             \
  private:                                                                   \
    nsAutoRefCnt _refc;                                                      \
  public:                                                                    \
    PRInt32 AddRef() {                                                       \
        PRInt32 n = PR_AtomicIncrement((PRInt32*)&_refc);                    \
        NS_LOG_ADDREF(this, n, #classname, sizeof(classname));               \
        return n;                                                            \
    }                                                                        \
    PRInt32 Release() {                                                      \
        PRInt32 n = PR_AtomicDecrement((PRInt32*)&_refc);                    \
        NS_LOG_RELEASE(this, n, #classname);                                 \
        if (n == 0)                                                          \
            delete this;                                                     \
        return n;                                                            \
    }

struct nsHostKey
{
    const char *host;
    PRUint16    flags;
    PRUint16    af;
};




class nsHostRecord : public PRCList, public nsHostKey
{
public:
    NS_DECL_REFCOUNTED_THREADSAFE(nsHostRecord)

    
    static nsresult Create(const nsHostKey *key, nsHostRecord **record);

    








    PRAddrInfo  *addr_info;
    PRNetAddr   *addr;
    PRUint32     expiration; 

    PRBool HasResult() const { return (addr_info || addr) != nsnull; }

private:
    friend class nsHostResolver;

    PRCList callbacks; 

    PRBool  resolving; 

 

   ~nsHostRecord();
};





class NS_NO_VTABLE nsResolveHostCallback : public PRCList
{
public:
    
















    virtual void OnLookupComplete(nsHostResolver *resolver,
                                  nsHostRecord   *record,
                                  nsresult        status) = 0;
};




class nsHostResolver
{
public:
    


    NS_DECL_REFCOUNTED_THREADSAFE(nsHostResolver)

    


    static nsresult Create(PRUint32         maxCacheEntries,  
                           PRUint32         maxCacheLifetime, 
                           nsHostResolver **resolver);
    
    



    void Shutdown();

    






    nsresult ResolveHost(const char            *hostname,
                         PRUint16               flags,
                         PRUint16               af,
                         nsResolveHostCallback *callback);

    





    void DetachCallback(const char            *hostname,
                        PRUint16               flags,
                        PRUint16               af,
                        nsResolveHostCallback *callback,
                        nsresult               status);

    






    enum {
        RES_BYPASS_CACHE = 1 << 0,
        RES_CANON_NAME   = 1 << 1
    };

private:
    nsHostResolver(PRUint32 maxCacheEntries=50, PRUint32 maxCacheLifetime=1);
   ~nsHostResolver();

    nsresult Init();
    nsresult IssueLookup(nsHostRecord *);
    PRBool   GetHostToLookup(nsHostRecord **);
    void     OnLookupComplete(nsHostRecord *, nsresult, PRAddrInfo *);

    PR_STATIC_CALLBACK(void) ThreadFunc(void *);

    PRUint32      mMaxCacheEntries;
    PRUint32      mMaxCacheLifetime;
    PRLock       *mLock;
    PRCondVar    *mIdleThreadCV; 
    PRBool        mHaveIdleThread;
    PRUint32      mThreadCount;
    PLDHashTable  mDB;
    PRCList       mPendingQ;
    PRCList       mEvictionQ;
    PRUint32      mEvictionQSize;
    PRTime        mCreationTime;
    PRBool        mShutdown;
};

#endif 
