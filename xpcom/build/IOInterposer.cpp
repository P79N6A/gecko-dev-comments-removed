





#include <algorithm>
#include <vector>

#include "IOInterposer.h"

#include "IOInterposerPrivate.h"
#include "MainThreadIOLogger.h"
#include "mozilla/Atomics.h"
#include "mozilla/Mutex.h"
#include "mozilla/RefPtr.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/ThreadLocal.h"
#if !defined(XP_WIN)
#include "NSPRInterposer.h"
#endif 
#include "nsXULAppAPI.h"
#include "PoisonIOInterposer.h"

using namespace mozilla;

namespace {


template<class T>
bool
VectorContains(const std::vector<T>& aVector, const T& aElement)
{
  return std::find(aVector.begin(), aVector.end(), aElement) != aVector.end();
}


template<class T>
void
VectorRemove(std::vector<T>& aVector, const T& aElement)
{
  typename std::vector<T>::iterator newEnd =
    std::remove(aVector.begin(), aVector.end(), aElement);
  aVector.erase(newEnd, aVector.end());
}


struct ObserverLists
{
private:
  ~ObserverLists() {}

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(ObserverLists)

  ObserverLists() {}

  ObserverLists(ObserverLists const& aOther)
    : mCreateObservers(aOther.mCreateObservers)
    , mReadObservers(aOther.mReadObservers)
    , mWriteObservers(aOther.mWriteObservers)
    , mFSyncObservers(aOther.mFSyncObservers)
    , mStatObservers(aOther.mStatObservers)
    , mCloseObservers(aOther.mCloseObservers)
    , mStageObservers(aOther.mStageObservers)
  {
  }
  
  
  
  
  std::vector<IOInterposeObserver*>  mCreateObservers;
  std::vector<IOInterposeObserver*>  mReadObservers;
  std::vector<IOInterposeObserver*>  mWriteObservers;
  std::vector<IOInterposeObserver*>  mFSyncObservers;
  std::vector<IOInterposeObserver*>  mStatObservers;
  std::vector<IOInterposeObserver*>  mCloseObservers;
  std::vector<IOInterposeObserver*>  mStageObservers;
};

class PerThreadData
{
public:
  explicit PerThreadData(bool aIsMainThread = false)
    : mIsMainThread(aIsMainThread)
    , mIsHandlingObservation(false)
    , mCurrentGeneration(0)
  {
    MOZ_COUNT_CTOR(PerThreadData);
  }

  ~PerThreadData()
  {
    MOZ_COUNT_DTOR(PerThreadData);
  }

  void CallObservers(IOInterposeObserver::Observation& aObservation)
  {
    
    if (mIsHandlingObservation) {
      return;
    }

    mIsHandlingObservation = true;
    
    std::vector<IOInterposeObserver*>* observers = nullptr;
    switch (aObservation.ObservedOperation()) {
      case IOInterposeObserver::OpCreateOrOpen:
        observers = &mObserverLists->mCreateObservers;
        break;
      case IOInterposeObserver::OpRead:
        observers = &mObserverLists->mReadObservers;
        break;
      case IOInterposeObserver::OpWrite:
        observers = &mObserverLists->mWriteObservers;
        break;
      case IOInterposeObserver::OpFSync:
        observers = &mObserverLists->mFSyncObservers;
        break;
      case IOInterposeObserver::OpStat:
        observers = &mObserverLists->mStatObservers;
        break;
      case IOInterposeObserver::OpClose:
        observers = &mObserverLists->mCloseObservers;
        break;
      case IOInterposeObserver::OpNextStage:
        observers = &mObserverLists->mStageObservers;
        break;
      default: {
        
        
        MOZ_ASSERT(false);
        
        return;
      }
    }
    MOZ_ASSERT(observers);

    
    for (auto i = observers->begin(), e = observers->end(); i != e; ++i) {
      (*i)->Observe(aObservation);
    }
    mIsHandlingObservation = false;
  }

  inline uint32_t GetCurrentGeneration() const { return mCurrentGeneration; }

  inline bool IsMainThread() const { return mIsMainThread; }

  inline void SetObserverLists(uint32_t aNewGeneration,
                               RefPtr<ObserverLists>& aNewLists)
  {
    mCurrentGeneration = aNewGeneration;
    mObserverLists = aNewLists;
  }

  inline void ClearObserverLists()
  {
    if (mObserverLists) {
      mCurrentGeneration = 0;
      mObserverLists = nullptr;
    }
  }

private:
  bool                  mIsMainThread;
  bool                  mIsHandlingObservation;
  uint32_t              mCurrentGeneration;
  RefPtr<ObserverLists> mObserverLists;
};

class MasterList
{
public:
  MasterList()
    : mObservedOperations(IOInterposeObserver::OpNone)
    , mIsEnabled(true)
  {
    MOZ_COUNT_CTOR(MasterList);
  }

  ~MasterList()
  {
    MOZ_COUNT_DTOR(MasterList);
  }

  inline void Disable() { mIsEnabled = false; }

  void Register(IOInterposeObserver::Operation aOp,
                IOInterposeObserver* aObserver)
  {
    IOInterposer::AutoLock lock(mLock);

    ObserverLists* newLists = nullptr;
    if (mObserverLists) {
      newLists = new ObserverLists(*mObserverLists);
    } else {
      newLists = new ObserverLists();
    }
    
    
    if (aOp & IOInterposeObserver::OpCreateOrOpen &&
        !VectorContains(newLists->mCreateObservers, aObserver)) {
      newLists->mCreateObservers.push_back(aObserver);
    }
    if (aOp & IOInterposeObserver::OpRead &&
        !VectorContains(newLists->mReadObservers, aObserver)) {
      newLists->mReadObservers.push_back(aObserver);
    }
    if (aOp & IOInterposeObserver::OpWrite &&
        !VectorContains(newLists->mWriteObservers, aObserver)) {
      newLists->mWriteObservers.push_back(aObserver);
    }
    if (aOp & IOInterposeObserver::OpFSync &&
        !VectorContains(newLists->mFSyncObservers, aObserver)) {
      newLists->mFSyncObservers.push_back(aObserver);
    }
    if (aOp & IOInterposeObserver::OpStat &&
        !VectorContains(newLists->mStatObservers, aObserver)) {
      newLists->mStatObservers.push_back(aObserver);
    }
    if (aOp & IOInterposeObserver::OpClose &&
        !VectorContains(newLists->mCloseObservers, aObserver)) {
      newLists->mCloseObservers.push_back(aObserver);
    }
    if (aOp & IOInterposeObserver::OpNextStage &&
        !VectorContains(newLists->mStageObservers, aObserver)) {
      newLists->mStageObservers.push_back(aObserver);
    }
    mObserverLists = newLists;
    mObservedOperations =
      (IOInterposeObserver::Operation)(mObservedOperations | aOp);

    mCurrentGeneration++;
  }

  void Unregister(IOInterposeObserver::Operation aOp,
                  IOInterposeObserver* aObserver)
  {
    IOInterposer::AutoLock lock(mLock);

    ObserverLists* newLists = nullptr;
    if (mObserverLists) {
      newLists = new ObserverLists(*mObserverLists);
    } else {
      newLists = new ObserverLists();
    }

    if (aOp & IOInterposeObserver::OpCreateOrOpen) {
      VectorRemove(newLists->mCreateObservers, aObserver);
      if (newLists->mCreateObservers.empty()) {
        mObservedOperations =
          (IOInterposeObserver::Operation)(mObservedOperations &
                                           ~IOInterposeObserver::OpCreateOrOpen);
      }
    }
    if (aOp & IOInterposeObserver::OpRead) {
      VectorRemove(newLists->mReadObservers, aObserver);
      if (newLists->mReadObservers.empty()) {
        mObservedOperations =
          (IOInterposeObserver::Operation)(mObservedOperations &
                                           ~IOInterposeObserver::OpRead);
      }
    }
    if (aOp & IOInterposeObserver::OpWrite) {
      VectorRemove(newLists->mWriteObservers, aObserver);
      if (newLists->mWriteObservers.empty()) {
        mObservedOperations =
          (IOInterposeObserver::Operation)(mObservedOperations &
                                           ~IOInterposeObserver::OpWrite);
      }
    }
    if (aOp & IOInterposeObserver::OpFSync) {
      VectorRemove(newLists->mFSyncObservers, aObserver);
      if (newLists->mFSyncObservers.empty()) {
        mObservedOperations =
          (IOInterposeObserver::Operation)(mObservedOperations &
                                           ~IOInterposeObserver::OpFSync);
      }
    }
    if (aOp & IOInterposeObserver::OpStat) {
      VectorRemove(newLists->mStatObservers, aObserver);
      if (newLists->mStatObservers.empty()) {
        mObservedOperations =
          (IOInterposeObserver::Operation)(mObservedOperations &
                                           ~IOInterposeObserver::OpStat);
      }
    }
    if (aOp & IOInterposeObserver::OpClose) {
      VectorRemove(newLists->mCloseObservers, aObserver);
      if (newLists->mCloseObservers.empty()) {
        mObservedOperations =
          (IOInterposeObserver::Operation)(mObservedOperations &
                                           ~IOInterposeObserver::OpClose);
      }
    }
    if (aOp & IOInterposeObserver::OpNextStage) {
      VectorRemove(newLists->mStageObservers, aObserver);
      if (newLists->mStageObservers.empty()) {
        mObservedOperations =
          (IOInterposeObserver::Operation)(mObservedOperations &
                                           ~IOInterposeObserver::OpNextStage);
      }
    }
    mObserverLists = newLists;
    mCurrentGeneration++;
  }

  void Update(PerThreadData& aPtd)
  {
    if (mCurrentGeneration == aPtd.GetCurrentGeneration()) {
      return;
    }
    
    
    IOInterposer::AutoLock lock(mLock);
    aPtd.SetObserverLists(mCurrentGeneration, mObserverLists);
  }

  inline bool IsObservedOperation(IOInterposeObserver::Operation aOp)
  {
    
    
    
    
    
    return mIsEnabled && !!(mObservedOperations & aOp);
  }

private:
  RefPtr<ObserverLists>             mObserverLists;
  
  
  
  
  
  IOInterposer::Mutex               mLock;
  
  IOInterposeObserver::Operation    mObservedOperations;
  
  Atomic<bool>                      mIsEnabled;
  
  Atomic<uint32_t>                  mCurrentGeneration;
};


class NextStageObservation : public IOInterposeObserver::Observation
{
public:
  NextStageObservation()
    : IOInterposeObserver::Observation(IOInterposeObserver::OpNextStage,
                                       "IOInterposer", false)
  {
    mStart = TimeStamp::Now();
    mEnd = mStart;
  }
};


static StaticAutoPtr<MasterList> sMasterList;
static ThreadLocal<PerThreadData*> sThreadLocalData;
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

const char*
IOInterposeObserver::Observation::ObservedOperationString() const
{
  switch (mOperation) {
    case OpCreateOrOpen:
      return "create/open";
    case OpRead:
      return "read";
    case OpWrite:
      return "write";
    case OpFSync:
      return "fsync";
    case OpStat:
      return "stat";
    case OpClose:
      return "close";
    case OpNextStage:
      return "NextStage";
    default:
      return "unknown";
  }
}

void
IOInterposeObserver::Observation::Report()
{
  if (mShouldReport) {
    mEnd = TimeStamp::Now();
    IOInterposer::Report(*this);
  }
}

bool
IOInterposer::Init()
{
  
  if (sMasterList) {
    return true;
  }
  if (!sThreadLocalData.init()) {
    return false;
  }
  bool isMainThread = true;
  RegisterCurrentThread(isMainThread);
  sMasterList = new MasterList();

  MainThreadIOLogger::Init();

  
  InitPoisonIOInterposer();
  
  
#if !defined(XP_WIN)
  InitNSPRIOInterposing();
#endif
  return true;
}

bool
IOInterposeObserver::IsMainThread()
{
  if (!sThreadLocalData.initialized()) {
    return false;
  }
  PerThreadData* ptd = sThreadLocalData.get();
  if (!ptd) {
    return false;
  }
  return ptd->IsMainThread();
}

void
IOInterposer::Clear()
{
  


#if defined(DEBUG) || defined(FORCE_BUILD_REFCNT_LOGGING) || defined(MOZ_ASAN)
  UnregisterCurrentThread();
  sMasterList = nullptr;
#endif
}

void
IOInterposer::Disable()
{
  if (!sMasterList) {
    return;
  }
  sMasterList->Disable();
}

void
IOInterposer::Report(IOInterposeObserver::Observation& aObservation)
{
  PerThreadData* ptd = sThreadLocalData.get();
  if (!ptd) {
    
    
    
    return;
  }

  if (!sMasterList) {
    
    ptd->ClearObserverLists();
    return;
  }

  sMasterList->Update(*ptd);

  
  if (!IOInterposer::IsObservedOperation(aObservation.ObservedOperation())) {
    return;
  }

  ptd->CallObservers(aObservation);
}

bool
IOInterposer::IsObservedOperation(IOInterposeObserver::Operation aOp)
{
  return sMasterList && sMasterList->IsObservedOperation(aOp);
}

void
IOInterposer::Register(IOInterposeObserver::Operation aOp,
                       IOInterposeObserver* aObserver)
{
  MOZ_ASSERT(aObserver);
  if (!sMasterList || !aObserver) {
    return;
  }

  sMasterList->Register(aOp, aObserver);
}

void
IOInterposer::Unregister(IOInterposeObserver::Operation aOp,
                         IOInterposeObserver* aObserver)
{
  if (!sMasterList) {
    return;
  }

  sMasterList->Unregister(aOp, aObserver);
}

void
IOInterposer::RegisterCurrentThread(bool aIsMainThread)
{
  if (!sThreadLocalData.initialized()) {
    return;
  }
  MOZ_ASSERT(!sThreadLocalData.get());
  PerThreadData* curThreadData = new PerThreadData(aIsMainThread);
  sThreadLocalData.set(curThreadData);
}

void
IOInterposer::UnregisterCurrentThread()
{
  if (!sThreadLocalData.initialized()) {
    return;
  }
  PerThreadData* curThreadData = sThreadLocalData.get();
  MOZ_ASSERT(curThreadData);
  sThreadLocalData.set(nullptr);
  delete curThreadData;
}

void
IOInterposer::EnteringNextStage()
{
  if (!sMasterList) {
    return;
  }
  NextStageObservation observation;
  Report(observation);
}

