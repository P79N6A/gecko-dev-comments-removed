








































































































#ifndef nsAutoLock_h__
#define nsAutoLock_h__

#include "nscore.h"
#include "prlock.h"
#include "prlog.h"
#include "mozilla/AutoRestore.h"






class NS_COM_GLUE NS_STACK_CLASS nsAutoLockBase {
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






class NS_COM_GLUE NS_STACK_CLASS nsAutoUnlockBase {
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





class NS_COM_GLUE NS_STACK_CLASS nsAutoLock : public nsAutoLockBase {
private:
    PRLock* mLock;
    PRBool mLocked;
    MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    
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

    







    nsAutoLock(PRLock* aLock MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM)
        : nsAutoLockBase(aLock, eAutoLock),
          mLock(aLock),
          mLocked(PR_TRUE) {
        MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
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

class NS_STACK_CLASS nsAutoUnlock : private nsAutoUnlockBase
{
private:
    PRLock *mLock;
    MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER
     
public:
    nsAutoUnlock(PRLock *lock MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM) : 
        nsAutoUnlockBase(lock),
        mLock(lock)
    {
        MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
        PR_Unlock(mLock);
    }

    ~nsAutoUnlock() {
        PR_Lock(mLock);
    }
};

#include "prcmon.h"
#include "nsError.h"
#include "nsDebug.h"

class NS_COM_GLUE NS_STACK_CLASS nsAutoMonitor : public nsAutoLockBase {
public:

    







    static PRMonitor* NewMonitor(const char* name);
    static void       DestroyMonitor(PRMonitor* mon);

    
    







    nsAutoMonitor(PRMonitor* mon MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM)
        : nsAutoLockBase((void*)mon, eAutoMonitor),
          mMonitor(mon), mLockCount(0)
    {
        MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
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
    MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    
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

class NS_COM_GLUE NS_STACK_CLASS nsAutoCMonitor : public nsAutoLockBase {
public:
    nsAutoCMonitor(void* lockObject MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM)
        : nsAutoLockBase(lockObject, eAutoCMonitor),
          mLockObject(lockObject), mLockCount(0)
    {
        MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
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
    MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER

    
    
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

