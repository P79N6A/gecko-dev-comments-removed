








































































































#ifndef nsAutoLock_h__
#define nsAutoLock_h__

#include "nscore.h"
#include "prlock.h"
#include "prlog.h"






class NS_COM_GLUE nsAutoLockBase {
    friend class nsAutoUnlockBase;

protected:
    nsAutoLockBase() {}
    enum nsAutoLockType {eAutoLock, eAutoMonitor, eAutoCMonitor};

#ifdef DEBUG
    nsAutoLockBase(void* addr, nsAutoLockType type);
    ~nsAutoLockBase();

    void            Show();
    void            Hide();

    void*           mAddr;
    nsAutoLockBase* mDown;
    nsAutoLockType  mType;
#else
    nsAutoLockBase(void* addr, nsAutoLockType type) {}
    ~nsAutoLockBase() {}

    void            Show() {}
    void            Hide() {}
#endif
};






class NS_COM_GLUE nsAutoUnlockBase {
protected:
    nsAutoUnlockBase() {}

#ifdef DEBUG
    nsAutoUnlockBase(void* addr);
    ~nsAutoUnlockBase();

    nsAutoLockBase* mLock;
#else
    nsAutoUnlockBase(void* addr) {}
    ~nsAutoUnlockBase() {}
#endif
};





class NS_COM_GLUE nsAutoLock : public nsAutoLockBase {
private:
    PRLock* mLock;
    PRBool mLocked;

    
    
    nsAutoLock(void) {}
    nsAutoLock(nsAutoLock& ) {}
    nsAutoLock& operator =(nsAutoLock& ) {
        return *this;
    }

    
    
    static void* operator new(size_t ) CPP_THROW_NEW {
        return nsnull;
    }
    static void operator delete(void* ) {}

public:

    









    static PRLock* NewLock(const char* name);
    static void    DestroyLock(PRLock* lock);

    







    nsAutoLock(PRLock* aLock)
        : nsAutoLockBase(aLock, eAutoLock),
          mLock(aLock),
          mLocked(PR_TRUE) {
        PR_ASSERT(mLock);

        
        
        PR_Lock(mLock);
    }
    
    ~nsAutoLock(void) {
        if (mLocked)
            PR_Unlock(mLock);
    }

    



  
    void lock() {
        Show();
        PR_ASSERT(!mLocked);
        PR_Lock(mLock);
        mLocked = PR_TRUE;
    }


    



 
     void unlock() {
        PR_ASSERT(mLocked);
        PR_Unlock(mLock);
        mLocked = PR_FALSE;
        Hide();
    }
};

class nsAutoUnlock : nsAutoUnlockBase
{
private:
    PRLock *mLock;
     
public:
    nsAutoUnlock(PRLock *lock) : 
        nsAutoUnlockBase(lock),
        mLock(lock)
    {
        PR_Unlock(mLock);
    }

    ~nsAutoUnlock() {
        PR_Lock(mLock);
    }
};

#include "prcmon.h"
#include "nsError.h"
#include "nsDebug.h"

class NS_COM_GLUE nsAutoMonitor : public nsAutoLockBase {
public:

    







    static PRMonitor* NewMonitor(const char* name);
    static void       DestroyMonitor(PRMonitor* mon);

    
    







    nsAutoMonitor(PRMonitor* mon)
        : nsAutoLockBase((void*)mon, eAutoMonitor),
          mMonitor(mon), mLockCount(0)
    {
        NS_ASSERTION(mMonitor, "null monitor");
        if (mMonitor) {
            PR_EnterMonitor(mMonitor);
            mLockCount = 1;
        }
    }

    ~nsAutoMonitor() {
        NS_ASSERTION(mMonitor, "null monitor");
        if (mMonitor && mLockCount) {
#ifdef DEBUG
            PRStatus status = 
#endif
            PR_ExitMonitor(mMonitor);
            NS_ASSERTION(status == PR_SUCCESS, "PR_ExitMonitor failed");
        }
    }

    



  
    void Enter();

    



      
    void Exit();

    


      
    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT) {
        return PR_Wait(mMonitor, interval) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

    


      
    nsresult Notify() {
        return PR_Notify(mMonitor) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

    


      
    nsresult NotifyAll() {
        return PR_NotifyAll(mMonitor) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

private:
    PRMonitor*  mMonitor;
    PRInt32     mLockCount;

    
    
    nsAutoMonitor(void) {}
    nsAutoMonitor(nsAutoMonitor& ) {}
    nsAutoMonitor& operator =(nsAutoMonitor& ) {
        return *this;
    }

    
    
    static void* operator new(size_t ) CPP_THROW_NEW {
        return nsnull;
    }
    static void operator delete(void* ) {}
};






#include "prcmon.h"
#include "nsError.h"

class NS_COM_GLUE nsAutoCMonitor : public nsAutoLockBase {
public:
    nsAutoCMonitor(void* lockObject)
        : nsAutoLockBase(lockObject, eAutoCMonitor),
          mLockObject(lockObject), mLockCount(0)
    {
        NS_ASSERTION(lockObject, "null lock object");
        PR_CEnterMonitor(mLockObject);
        mLockCount = 1;
    }

    ~nsAutoCMonitor() {
        if (mLockCount) {
#ifdef DEBUG
            PRStatus status =
#endif
            PR_CExitMonitor(mLockObject);
            NS_ASSERTION(status == PR_SUCCESS, "PR_CExitMonitor failed");
        }
    }

    void Enter();
    void Exit();

    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT) {
        return PR_CWait(mLockObject, interval) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

    nsresult Notify() {
        return PR_CNotify(mLockObject) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

    nsresult NotifyAll() {
        return PR_CNotifyAll(mLockObject) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

private:
    void*   mLockObject;
    PRInt32 mLockCount;

    
    
    nsAutoCMonitor(void) {}
    nsAutoCMonitor(nsAutoCMonitor& ) {}
    nsAutoCMonitor& operator =(nsAutoCMonitor& ) {
        return *this;
    }

    
    
    static void* operator new(size_t ) CPP_THROW_NEW {
        return nsnull;
    }
    static void operator delete(void* ) {}
};

#endif 

