



#include "IOInterposer.h"
#include "NSPRInterposer.h"
#include "SQLiteInterposer.h"

using namespace mozilla;

static StaticAutoPtr<IOInterposer> sSingleton;

 IOInterposer*
IOInterposer::GetInstance()
{
  if (!sSingleton) {
    
    
    
    sSingleton = new IOInterposer();
    sSingleton->Init();
  }

  return sSingleton.get();
}

 void
IOInterposer::ClearInstance()
{
  
  
  
  sSingleton = nullptr;
}

IOInterposer::IOInterposer()
  :mMutex("IOInterposer::mMutex")
{
  
  
  
}

IOInterposer::~IOInterposer()
{
  
  
  
  Enable(false);
  NSPRInterposer::ClearInstance();
  SQLiteInterposer::ClearInstance();
}

bool
IOInterposer::Init()
{
  
  
  
  mozilla::MutexAutoLock lock(mMutex);
  IOInterposerModule* nsprModule = NSPRInterposer::GetInstance(this,
      IOInterposeObserver::OpAll);
  if (!nsprModule) {
    return false;
  }

  IOInterposerModule* sqlModule = SQLiteInterposer::GetInstance(this,
      IOInterposeObserver::OpAll);
  if (!sqlModule) {
    return false;
  }

  mModules.AppendElement(nsprModule);
  mModules.AppendElement(sqlModule);
  return true;
}

void
IOInterposer::Enable(bool aEnable)
{
  mozilla::MutexAutoLock lock(mMutex);
  for (PRUint32 i = 0; i < mModules.Length(); ++i ) {
    mModules[i]->Enable(aEnable);
  }
}

void
IOInterposer::Register(IOInterposeObserver::Operation aOp,
                       IOInterposeObserver* aObserver)
{
  
  
  
  if (aOp & IOInterposeObserver::OpRead) {
    mReadObservers.AppendElement(aObserver);
  }
  if (aOp & IOInterposeObserver::OpWrite) {
    mWriteObservers.AppendElement(aObserver);
  }
  if (aOp & IOInterposeObserver::OpFSync) {
    mFSyncObservers.AppendElement(aObserver);
  }
}

void
IOInterposer::Deregister(IOInterposeObserver::Operation aOp,
                         IOInterposeObserver* aObserver)
{
  
  
  
  if (aOp & IOInterposeObserver::OpRead) {
    mReadObservers.RemoveElement(aObserver);
  }
  if (aOp & IOInterposeObserver::OpWrite) {
    mWriteObservers.RemoveElement(aObserver);
  }
  if (aOp & IOInterposeObserver::OpFSync) {
    mFSyncObservers.RemoveElement(aObserver);
  }
}

void
IOInterposer::Observe(IOInterposeObserver::Operation aOp, double& aDuration,
                      const char* aModuleInfo)
{
  MOZ_ASSERT(NS_IsMainThread());
  switch (aOp) {
    case IOInterposeObserver::OpRead:
      {
        for (PRUint32 i = 0; i < mReadObservers.Length(); ++i) {
          mReadObservers[i]->Observe(aOp, aDuration, aModuleInfo);
        }
      }
      break;
    case IOInterposeObserver::OpWrite:
      {
        for (PRUint32 i = 0; i < mWriteObservers.Length(); ++i) {
          mWriteObservers[i]->Observe(aOp, aDuration, aModuleInfo);
        }
      }
      break;
    case IOInterposeObserver::OpFSync:
      {
        for (PRUint32 i = 0; i < mFSyncObservers.Length(); ++i) {
          mFSyncObservers[i]->Observe(aOp, aDuration, aModuleInfo);
        }
      }
      break;
    default:
      break;
  }
}

