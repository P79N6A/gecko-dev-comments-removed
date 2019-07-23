






































#ifndef mozilla_BlockingResourceBase_h
#define mozilla_BlockingResourceBase_h

#include "nscore.h"
#include "prlog.h"
#include "nsError.h"
#include "nsDebug.h"





namespace mozilla {






class NS_COM_GLUE BlockingResourceBase
{
protected:
    BlockingResourceBase() {
    }
    
    enum BlockingResourceType { eMutex, eMonitor, eCondVar };

#ifdef DEBUG
    ~BlockingResourceBase();

    







    void Init(void* aResource, 
              const char* aName,
              BlockingResourceType aType);

    















































    




    void* mResource;

    




    BlockingResourceType mType;

    





    BlockingResourceBase* mChainPrev;

    




    void Acquire();
    
    






    void Release();

#else
    ~BlockingResourceBase() {
    }

    void Init(void* aResource, 
              const char* aName,
              BlockingResourceType aType) { }

    void            Acquire() { }
    void            Release() { }

#endif
};


} 


#endif 
