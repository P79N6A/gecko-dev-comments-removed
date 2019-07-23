





































#ifndef mozilla_CondVar_h
#define mozilla_CondVar_h

#include "prcvar.h"

#include "mozilla/BlockingResourceBase.h"
#include "mozilla/Mutex.h"

namespace mozilla {







class NS_COM_GLUE CondVar : BlockingResourceBase
{
public:
    










    CondVar(Mutex& aLock, const char* aName) :
        BlockingResourceBase(aName, eCondVar),
        mLock(&aLock)
    {
        
        mCvar = PR_NewCondVar(mLock->mLock);
        if (!mCvar)
            NS_RUNTIMEABORT("Can't allocate mozilla::CondVar");
    }

    



    ~CondVar()
    {
        NS_ASSERTION(mCvar && mLock,
                     "improperly constructed CondVar or double free");
        PR_DestroyCondVar(mCvar);
        mCvar = 0;
        mLock = 0;
    }

#ifndef DEBUG
    


      
    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT)
    {
        
        return PR_WaitCondVar(mCvar, interval) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }
#else
    nsresult Wait(PRIntervalTime interval = PR_INTERVAL_NO_TIMEOUT);
#endif 

    


      
    nsresult Notify()
    {
        
        return PR_NotifyCondVar(mCvar) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

    


      
    nsresult NotifyAll()
    {
        
        return PR_NotifyAllCondVar(mCvar) == PR_SUCCESS
            ? NS_OK : NS_ERROR_FAILURE;
    }

#ifdef DEBUG
    



    void AssertCurrentThreadOwnsMutex()
    {
        mLock->AssertCurrentThreadOwns();
    }

    



    void AssertNotCurrentThreadOwnsMutex()
    {
        mLock->AssertNotCurrentThreadOwns();
    }

#else
    void AssertCurrentThreadOwnsMutex()
    {
    }
    void AssertNotCurrentThreadOwnsMutex()
    {
    }

#endif  

private:
    CondVar();
    CondVar(CondVar&);
    CondVar& operator=(CondVar&);

    Mutex* mLock;
    PRCondVar* mCvar;
};


} 


#endif  
