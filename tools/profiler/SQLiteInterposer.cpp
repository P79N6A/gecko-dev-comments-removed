



#include "SQLiteInterposer.h"

using namespace mozilla;

static StaticAutoPtr<SQLiteInterposer> sSingleton;
static const char* sModuleInfo = "SQLite";

 IOInterposerModule*
SQLiteInterposer::GetInstance(IOInterposeObserver* aObserver,
                              IOInterposeObserver::Operation aOpsToInterpose)
{
  
  
  
  if (!sSingleton) {
    nsAutoPtr<SQLiteInterposer> newObj(new SQLiteInterposer());
    if (!newObj->Init(aObserver, aOpsToInterpose)) {
      return nullptr;
    }
    sSingleton = newObj.forget();
  }
  return sSingleton;
}

 void
SQLiteInterposer::ClearInstance()
{
  
  
  
  sSingleton = nullptr;
}

SQLiteInterposer::SQLiteInterposer()
  :mObserver(nullptr),
   mOps(IOInterposeObserver::OpNone),
   mEnabled(false)
{
}

SQLiteInterposer::~SQLiteInterposer()
{
  
  
  
  mOps = IOInterposeObserver::OpNone;
  sSingleton = nullptr;
  Enable(false);
}

bool
SQLiteInterposer::Init(IOInterposeObserver* aObserver,
                       IOInterposeObserver::Operation aOpsToInterpose)
{
  
  
  
  if (!aObserver || !(aOpsToInterpose & IOInterposeObserver::OpAll)) {
    return false;
  }
  mObserver = aObserver;
  mOps = aOpsToInterpose;
  return true;
}

void
SQLiteInterposer::Enable(bool aEnable)
{
  mEnabled = aEnable ? 1 : 0;
}

 void
SQLiteInterposer::OnRead(double& aDuration)
{
  if (!NS_IsMainThread() || !sSingleton) {
    return;
  }
  if (sSingleton->mEnabled &&
      (sSingleton->mOps & IOInterposeObserver::OpRead)) {
    sSingleton->mObserver->Observe(IOInterposeObserver::OpRead, aDuration,
                                   sModuleInfo);
  }
}

 void
SQLiteInterposer::OnWrite(double& aDuration)
{
  if (!NS_IsMainThread() || !sSingleton) {
    return;
  }
  if (sSingleton->mEnabled && 
      (sSingleton->mOps & IOInterposeObserver::OpWrite)) {
    sSingleton->mObserver->Observe(IOInterposeObserver::OpWrite, aDuration,
                                   sModuleInfo);
  }
}

 void
SQLiteInterposer::OnFSync(double& aDuration)
{
  if (!NS_IsMainThread() || !sSingleton) {
    return;
  }
  if (sSingleton->mEnabled &&
      (sSingleton->mOps & IOInterposeObserver::OpFSync)) {
    sSingleton->mObserver->Observe(IOInterposeObserver::OpFSync, aDuration,
                                   sModuleInfo);
  }
}

