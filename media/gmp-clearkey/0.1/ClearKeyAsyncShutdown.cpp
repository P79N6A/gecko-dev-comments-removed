















#include "ClearKeyAsyncShutdown.h"
#include "gmp-task-utils.h"

ClearKeyAsyncShutdown::ClearKeyAsyncShutdown(GMPAsyncShutdownHost *aHostAPI)
  : mHost(aHostAPI)
{
  CK_LOGD("ClearKeyAsyncShutdown::ClearKeyAsyncShutdown");
  AddRef();
}

ClearKeyAsyncShutdown::~ClearKeyAsyncShutdown()
{
  CK_LOGD("ClearKeyAsyncShutdown::~ClearKeyAsyncShutdown");
}

void ShutdownTask(ClearKeyAsyncShutdown* aSelf, GMPAsyncShutdownHost* aHost)
{
  
  
  CK_LOGD("ClearKeyAsyncShutdown::BeginShutdown calling ShutdownComplete");
  aHost->ShutdownComplete();
  aSelf->Release();
}

void ClearKeyAsyncShutdown::BeginShutdown()
{
  CK_LOGD("ClearKeyAsyncShutdown::BeginShutdown dispatching asynchronous shutdown task");
  GetPlatform()->runonmainthread(WrapTaskNM(ShutdownTask, this, mHost));
}
