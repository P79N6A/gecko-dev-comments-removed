



#include <algorithm>
#include <vector>

#include "IOInterposer.h"

#include "mozilla/Atomics.h"
#include "mozilla/Mutex.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/ThreadLocal.h"
#if !defined(XP_WIN)
#include "NSPRInterposer.h"
#endif 
#include "nsXULAppAPI.h"
#include "PoisonIOInterposer.h"

using namespace mozilla;

namespace {


struct ObserverLists {
  ObserverLists()
    : mObserverListsLock(PR_NewLock())
    , mIsEnabled(true)
  {
    
    
    
  }

  
  
  
  
  
  
  PRLock* mObserverListsLock;

  
  mozilla::Atomic<bool>              mIsEnabled;

  ~ObserverLists()
  {
    PR_DestroyLock(mObserverListsLock);
    mObserverListsLock = nullptr;
  }

  
  
  
  
  std::vector<IOInterposeObserver*>  mCreateObservers;
  std::vector<IOInterposeObserver*>  mReadObservers;
  std::vector<IOInterposeObserver*>  mWriteObservers;
  std::vector<IOInterposeObserver*>  mFSyncObservers;
  std::vector<IOInterposeObserver*>  mStatObservers;
  std::vector<IOInterposeObserver*>  mCloseObservers;
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
static ThreadLocal<bool> sIsMainThread;


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

IOInterposeObserver::Observation::Observation(Operation aOperation,
                                              const char* aReference,
                                              bool aShouldReport)
  : mOperation(aOperation)
  , mReference(aReference)
  , mShouldReport(IOInterposer::IsObservedOperation(aOperation) &&
                  aShouldReport)
{
  if (mShouldReport) {
    mStart = TimeStamp::Now();
  }
}

IOInterposeObserver::Observation::Observation(Operation aOperation,
                                              const TimeStamp& aStart,
                                              const TimeStamp& aEnd,
                                              const char* aReference)
  : mOperation(aOperation)
  , mStart(aStart)
  , mEnd(aEnd)
  , mReference(aReference)
  , mShouldReport(false)
{
}

void
IOInterposeObserver::Observation::Report()
{
  if (mShouldReport) {
    mEnd = TimeStamp::Now();
    IOInterposer::Report(*this);
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
  if (sIsMainThread.init()) {
#if defined(XP_WIN)
    bool isMainThread = XRE_GetWindowsEnvironment() !=
                          WindowsEnvironmentType_Metro;
#else
    bool isMainThread = true;
#endif
    sIsMainThread.set(isMainThread);
  }
  
#if defined(XP_WIN) || defined(XP_MACOSX)
  InitPoisonIOInterposer();
#endif
  
  
#if !defined(XP_WIN)
  InitNSPRIOInterposing();
#endif
}

 bool
IOInterposeObserver::IsMainThread()
{
  return sIsMainThread.initialized() && sIsMainThread.get();
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

 void
IOInterposer::Disable()
{
  if (!sObserverLists) {
    return;
  }
  sObserverLists->mIsEnabled = false;
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
    case IOInterposeObserver::OpCreateOrOpen:
      {
        observers = &sObserverLists->mCreateObservers;
      }
      break;
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
    case IOInterposeObserver::OpStat:
      {
        observers = &sObserverLists->mStatObservers;
      }
      break;
    case IOInterposeObserver::OpClose:
      {
        observers = &sObserverLists->mCloseObservers;
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

 bool
IOInterposer::IsObservedOperation(IOInterposeObserver::Operation aOp)
{
  return sObserverLists && sObserverLists->mIsEnabled &&
         !!(sObservedOperations & aOp);
}

 void IOInterposer::Register(IOInterposeObserver::Operation aOp,
                                         IOInterposeObserver* aObserver)
{
  
  MOZ_ASSERT(aObserver);
  if (!sObserverLists || !aObserver) {
    return;
  }

  AutoPRLock listLock(sObserverLists->mObserverListsLock);

  
  
  if (aOp & IOInterposeObserver::OpCreateOrOpen &&
      !VectorContains(sObserverLists->mCreateObservers, aObserver)) {
    sObserverLists->mCreateObservers.push_back(aObserver);
  }
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
  if (aOp & IOInterposeObserver::OpStat &&
      !VectorContains(sObserverLists->mStatObservers, aObserver)) {
    sObserverLists->mStatObservers.push_back(aObserver);
  }
  if (aOp & IOInterposeObserver::OpClose &&
      !VectorContains(sObserverLists->mCloseObservers, aObserver)) {
    sObserverLists->mCloseObservers.push_back(aObserver);
  }

  
  
  sObservedOperations = (IOInterposeObserver::Operation)
                        (sObservedOperations | aOp);
}

 void IOInterposer::Unregister(IOInterposeObserver::Operation aOp,
                                           IOInterposeObserver* aObserver)
{
  if (!sObserverLists) {
    return;
  }

  AutoPRLock listLock(sObserverLists->mObserverListsLock);

  if (aOp & IOInterposeObserver::OpCreateOrOpen) {
    VectorRemove(sObserverLists->mCreateObservers, aObserver);
    if (sObserverLists->mCreateObservers.empty()) {
      sObservedOperations = (IOInterposeObserver::Operation)
                       (sObservedOperations &
                        ~IOInterposeObserver::OpCreateOrOpen);
    }
  }
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
  if (aOp & IOInterposeObserver::OpStat) {
    VectorRemove(sObserverLists->mStatObservers, aObserver);
    if (sObserverLists->mStatObservers.empty()) {
      sObservedOperations = (IOInterposeObserver::Operation)
                       (sObservedOperations & ~IOInterposeObserver::OpStat);
    }
  }
  if (aOp & IOInterposeObserver::OpClose) {
    VectorRemove(sObserverLists->mCloseObservers, aObserver);
    if (sObserverLists->mCloseObservers.empty()) {
      sObservedOperations = (IOInterposeObserver::Operation)
                       (sObservedOperations & ~IOInterposeObserver::OpClose);
    }
  }
}

 void
IOInterposer::RegisterCurrentThread(bool aIsMainThread)
{
  
  
#if defined(XP_WIN)
  if (XRE_GetWindowsEnvironment() != WindowsEnvironmentType_Metro ||
      !sIsMainThread.initialized()) {
    return;
  }
  sIsMainThread.set(aIsMainThread);
#endif
}

