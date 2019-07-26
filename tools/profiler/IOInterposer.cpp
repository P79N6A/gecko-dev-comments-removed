



#include <algorithm>
#include <vector>

#include "IOInterposer.h"

#include "mozilla/Mutex.h"
#include "mozilla/StaticPtr.h"

using namespace mozilla;

namespace {


struct ObserverLists {
  ObserverLists()
  {
    mObserverListsLock = PR_NewLock();
    
    
    
  }

  
  
  
  
  
  
  PRLock* mObserverListsLock;

  ~ObserverLists()
  {
    PR_DestroyLock(mObserverListsLock);
    mObserverListsLock = nullptr;
  }

  
  
  
  
  std::vector<IOInterposeObserver*>  mReadObservers;
  std::vector<IOInterposeObserver*>  mWriteObservers;
  std::vector<IOInterposeObserver*>  mFSyncObservers;
};




class AutoPRLock
{
  PRLock* mLock;
public:
  AutoPRLock(PRLock* aLock)
   : mLock(aLock)
  {
    PR_Lock(aLock);
  }
  ~AutoPRLock()
  {
    PR_Unlock(mLock);
  }
};


static StaticAutoPtr<ObserverLists> sObserverLists;


template<class T>
bool VectorContains(const std::vector<T>& vector, const T& element)
{
  return std::find(vector.begin(), vector.end(), element) != vector.end();
}


template<class T>
void VectorRemove(std::vector<T>& vector, const T& element)
{
  typename std::vector<T>::iterator newEnd = std::remove(vector.begin(),
                                                         vector.end(), element);
  vector.erase(newEnd, vector.end());
}

} 


IOInterposeObserver::Operation IOInterposer::sObservedOperations =
                                                  IOInterposeObserver::OpNone;

 void IOInterposer::Init()
{
  
  if (sObserverLists) {
    return;
  }
  sObserverLists = new ObserverLists();
  sObservedOperations = IOInterposeObserver::OpNone;
}

 void IOInterposer::Clear()
{
  
  MOZ_ASSERT(sObserverLists);
  if (sObserverLists) {
    
    
    
    MOZ_ASSERT(sObserverLists->mReadObservers.empty());
    MOZ_ASSERT(sObserverLists->mWriteObservers.empty());
    MOZ_ASSERT(sObserverLists->mFSyncObservers.empty());

    sObserverLists = nullptr;
    sObservedOperations = IOInterposeObserver::OpNone;
  }
}

 void IOInterposer::Report(
  IOInterposeObserver::Observation& aObservation)
{
  
  MOZ_ASSERT(sObserverLists);
  if (!sObserverLists) {
    return;
  }

  
  
  
  AutoPRLock listLock(sObserverLists->mObserverListsLock);

  
  if (!IOInterposer::IsObservedOperation(aObservation.ObservedOperation())) {
    return;
  }

  
  std::vector<IOInterposeObserver*>* observers = nullptr;
  switch (aObservation.ObservedOperation()) {
    case IOInterposeObserver::OpRead:
      {
        observers = &sObserverLists->mReadObservers;
      }
      break;
    case IOInterposeObserver::OpWrite:
      {
        observers = &sObserverLists->mWriteObservers;
      }
      break;
    case IOInterposeObserver::OpFSync:
      {
        observers = &sObserverLists->mFSyncObservers;
      }
      break;
    default:
      {
        
        MOZ_ASSERT(false);
        
        return;
      }
  }
  MOZ_ASSERT(observers);

  
  uint32_t nObservers = observers->size();
  for (uint32_t i = 0; i < nObservers; ++i) {
    (*observers)[i]->Observe(aObservation);
  }
}

 void IOInterposer::Register(IOInterposeObserver::Operation aOp,
                                         IOInterposeObserver* aObserver)
{
  
  MOZ_ASSERT(sObserverLists);
  
  MOZ_ASSERT(aObserver);
  if (!sObserverLists || !aObserver) {
    return;
  }

  AutoPRLock listLock(sObserverLists->mObserverListsLock);

  
  
  if (aOp & IOInterposeObserver::OpRead &&
      !VectorContains(sObserverLists->mReadObservers, aObserver)) {
    sObserverLists->mReadObservers.push_back(aObserver);
  }
  if (aOp & IOInterposeObserver::OpWrite &&
      !VectorContains(sObserverLists->mWriteObservers, aObserver)) {
    sObserverLists->mWriteObservers.push_back(aObserver);
  }
  if (aOp & IOInterposeObserver::OpFSync &&
      !VectorContains(sObserverLists->mFSyncObservers, aObserver)) {
    sObserverLists->mFSyncObservers.push_back(aObserver);
  }

  
  
  sObservedOperations = (IOInterposeObserver::Operation)
                        (sObservedOperations | aOp);
}

 void IOInterposer::Unregister(IOInterposeObserver::Operation aOp,
                                           IOInterposeObserver* aObserver)
{
  
  MOZ_ASSERT(sObserverLists);
  if (!sObserverLists) {
    return;
  }

  AutoPRLock listLock(sObserverLists->mObserverListsLock);

  if (aOp & IOInterposeObserver::OpRead) {
    VectorRemove(sObserverLists->mReadObservers, aObserver);
    if (sObserverLists->mReadObservers.empty()) {
      sObservedOperations = (IOInterposeObserver::Operation)
                       (sObservedOperations & ~IOInterposeObserver::OpRead);
    }
  }
  if (aOp & IOInterposeObserver::OpWrite) {
    VectorRemove(sObserverLists->mWriteObservers, aObserver);
    if (sObserverLists->mWriteObservers.empty()) {
      sObservedOperations = (IOInterposeObserver::Operation)
                       (sObservedOperations & ~IOInterposeObserver::OpWrite);
    }
  }
  if (aOp & IOInterposeObserver::OpFSync) {
    VectorRemove(sObserverLists->mFSyncObservers, aObserver);
    if (sObserverLists->mFSyncObservers.empty()) {
      sObservedOperations = (IOInterposeObserver::Operation)
                       (sObservedOperations & ~IOInterposeObserver::OpFSync);
    }
  }
}
