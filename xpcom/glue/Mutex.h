





































#ifndef mozilla_Mutex_h
#define mozilla_Mutex_h

#include "prlock.h"

#include "mozilla/BlockingResourceBase.h"
#include "mozilla/GuardObjects.h"












namespace mozilla {








class NS_COM_GLUE Mutex : BlockingResourceBase
{
public:
    






    Mutex(const char* name) :
        BlockingResourceBase(name, eMutex)
    {
        MOZ_COUNT_CTOR(Mutex);
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
        MOZ_COUNT_DTOR(Mutex);
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

    



    void AssertCurrentThreadOwns () const
    {
    }

    



    void AssertNotCurrentThreadOwns () const
    {
    }

#else
    void Lock();
    void Unlock();

    void AssertCurrentThreadOwns () const
    {
        PR_ASSERT_CURRENT_THREAD_OWNS_LOCK(mLock);
    }

    void AssertNotCurrentThreadOwns () const
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








 
template<typename T>
class NS_COM_GLUE NS_STACK_CLASS BaseAutoLock
{
public:
    







    BaseAutoLock(T& aLock MOZ_GUARD_OBJECT_NOTIFIER_PARAM) :
        mLock(&aLock)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        NS_ASSERTION(mLock, "null mutex");
        mLock->Lock();
    }
    
    ~BaseAutoLock(void) {
        mLock->Unlock();
    }
 
private:
    BaseAutoLock();
    BaseAutoLock(BaseAutoLock&);
    BaseAutoLock& operator=(BaseAutoLock&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);

    T* mLock;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

typedef BaseAutoLock<Mutex> MutexAutoLock;







 
template<typename T>
class NS_COM_GLUE NS_STACK_CLASS BaseAutoUnlock 
{
public:
    BaseAutoUnlock(T& aLock MOZ_GUARD_OBJECT_NOTIFIER_PARAM) :
        mLock(&aLock)
    {
        MOZ_GUARD_OBJECT_NOTIFIER_INIT;
        NS_ASSERTION(mLock, "null lock");
        mLock->Unlock();
    }

    ~BaseAutoUnlock() 
    {
        mLock->Lock();
    }

private:
    BaseAutoUnlock();
    BaseAutoUnlock(BaseAutoUnlock&);
    BaseAutoUnlock& operator =(BaseAutoUnlock&);
    static void* operator new(size_t) CPP_THROW_NEW;
    static void operator delete(void*);
     
    T* mLock;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

typedef BaseAutoUnlock<Mutex> MutexAutoUnlock;

} 


#endif
