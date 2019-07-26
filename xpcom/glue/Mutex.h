





#ifndef mozilla_Mutex_h
#define mozilla_Mutex_h

#include "prlock.h"

#include "mozilla/BlockingResourceBase.h"
#include "mozilla/GuardObjects.h"
















namespace mozilla {






class NS_COM_GLUE OffTheBooksMutex : BlockingResourceBase
{
public:
    





    OffTheBooksMutex(const char* name) :
        BlockingResourceBase(name, eMutex)
    {
        mLock = PR_NewLock();
        if (!mLock)
            NS_RUNTIMEABORT("Can't allocate mozilla::Mutex");
    }

    ~OffTheBooksMutex()
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
    OffTheBooksMutex();
    OffTheBooksMutex(const OffTheBooksMutex&);
    OffTheBooksMutex& operator=(const OffTheBooksMutex&);

    PRLock* mLock;

    friend class CondVar;
};






class NS_COM_GLUE Mutex : public OffTheBooksMutex
{
public:
   Mutex(const char* name)
       : OffTheBooksMutex(name)
   {
       MOZ_COUNT_CTOR(Mutex);
   }

   ~Mutex()
   {
       MOZ_COUNT_DTOR(Mutex);
   }

private:
    Mutex();
    Mutex(const Mutex&);
    Mutex& operator=(const Mutex&);
};







 
template<typename T>
class NS_COM_GLUE MOZ_STACK_CLASS BaseAutoLock
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
typedef BaseAutoLock<OffTheBooksMutex> OffTheBooksMutexAutoLock;







 
template<typename T>
class NS_COM_GLUE MOZ_STACK_CLASS BaseAutoUnlock 
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
typedef BaseAutoUnlock<OffTheBooksMutex> OffTheBooksMutexAutoUnlock;

} 


#endif
