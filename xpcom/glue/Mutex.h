





































#ifndef mozilla_Mutex_h
#define mozilla_Mutex_h

#include "prlock.h"

#include "mozilla/BlockingResourceBase.h"












namespace mozilla {








class NS_COM_GLUE Mutex : BlockingResourceBase
{
public:
    






    Mutex(const char* name) :
        BlockingResourceBase(name, eMutex)
    {
        mLock = PR_NewLock();
        if (!mLock)
            NS_RUNTIMEABORT("Can't allocate mozilla::Mutex");
    }

    


    ~Mutex()
    {
        NS_ASSERTION(mLock,
                     "improperly constructed Lock or double free");
        
        PR_DestroyLock(mLock);
        mLock = 0;
    }

#ifndef DEBUG
    



    void Lock()
    {
        PR_Lock(mLock);
    }

    



    void Unlock()
    {
        PR_Unlock(mLock);
    }

    



    void AssertCurrentThreadOwns ()
    {
    }

    



    void AssertNotCurrentThreadOwns ()
    {
    }

#else
    void Lock();
    void Unlock();

    void AssertCurrentThreadOwns ()
    {
        PR_ASSERT_CURRENT_THREAD_OWNS_LOCK(mLock);
    }

    void AssertNotCurrentThreadOwns ()
    {
        
    }

#endif  

private:
    Mutex();
    Mutex(const Mutex&);
    Mutex& operator=(const Mutex&);

    PRLock* mLock;

    friend class CondVar;
};








 
class NS_COM_GLUE NS_STACK_CLASS MutexAutoLock
{
public:
    







    MutexAutoLock(mozilla::Mutex& aLock) :
        mLock(&aLock)
    {
        NS_ASSERTION(mLock, "null mutex");
        mLock->Lock();
    }
    
    ~MutexAutoLock(void) {
        mLock->Unlock();
    }
 
private:
    MutexAutoLock();
    MutexAutoLock(MutexAutoLock&);
    MutexAutoLock& operator=(MutexAutoLock&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);

    mozilla::Mutex* mLock;
};








 
class NS_COM_GLUE NS_STACK_CLASS MutexAutoUnlock 
{
public:
    MutexAutoUnlock(mozilla::Mutex& aLock) :
        mLock(&aLock)
    {
        NS_ASSERTION(mLock, "null lock");
        mLock->Unlock();
    }

    ~MutexAutoUnlock() 
    {
        mLock->Lock();
    }

private:
    MutexAutoUnlock();
    MutexAutoUnlock(MutexAutoUnlock&);
    MutexAutoUnlock& operator =(MutexAutoUnlock&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);
     
    mozilla::Mutex* mLock;
};


} 


#endif 
