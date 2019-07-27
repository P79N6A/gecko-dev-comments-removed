















#include "ClearKeyPersistence.h"
#include "ClearKeyUtils.h"
#include "ClearKeyStorage.h"
#include "ClearKeySessionManager.h"
#include "RefCounted.h"

#include <stdint.h>
#include <string.h>
#include <set>
#include <vector>
#include <sstream>
#include <assert.h>

using namespace std;


enum PersistentKeyState {
  UNINITIALIZED,
  LOADING,
  LOADED
};
static PersistentKeyState sPersistentKeyState = UNINITIALIZED;



static set<uint32_t> sPersistentSessionIds;

static vector<GMPTask*> sTasksBlockedOnSessionIdLoad;

static void
ReadAllRecordsFromIterator(GMPRecordIterator* aRecordIterator,
                           void* aUserArg,
                           GMPErr aStatus)
{
  assert(sPersistentKeyState == LOADING);
  if (GMP_SUCCEEDED(aStatus)) {
    
    
    const char* name = nullptr;
    uint32_t len = 0;
    while (GMP_SUCCEEDED(aRecordIterator->GetName(&name, &len))) {
      if (ClearKeyUtils::IsValidSessionId(name, len)) {
        assert(name[len] == 0);
        sPersistentSessionIds.insert(atoi(name));
      }
      aRecordIterator->NextRecord();
    }
  }
  sPersistentKeyState = LOADED;
  aRecordIterator->Close();

  for (size_t i = 0; i < sTasksBlockedOnSessionIdLoad.size(); i++) {
    sTasksBlockedOnSessionIdLoad[i]->Run();
    sTasksBlockedOnSessionIdLoad[i]->Destroy();
  }
  sTasksBlockedOnSessionIdLoad.clear();
}

 void
ClearKeyPersistence::EnsureInitialized()
{
  if (sPersistentKeyState == UNINITIALIZED) {
    sPersistentKeyState = LOADING;
    if (GMP_FAILED(EnumRecordNames(&ReadAllRecordsFromIterator))) {
      sPersistentKeyState = LOADED;
    }
  }
}

 string
ClearKeyPersistence::GetNewSessionId(GMPSessionType aSessionType)
{
  static uint32_t sNextSessionId = 1;

  
  while (Contains(sPersistentSessionIds, sNextSessionId)) {
    sNextSessionId++;
  }

  string sessionId;
  stringstream ss;
  ss << sNextSessionId;
  ss >> sessionId;

  if (aSessionType == kGMPPersistentSession) {
    sPersistentSessionIds.insert(sNextSessionId);
  }

  sNextSessionId++;

  return sessionId;
}


class CreateSessionTask : public GMPTask {
public:
  CreateSessionTask(ClearKeySessionManager* aTarget,
                    uint32_t aCreateSessionToken,
                    uint32_t aPromiseId,
                    const uint8_t* aInitData,
                    uint32_t aInitDataSize,
                    GMPSessionType aSessionType)
    : mTarget(aTarget)
    , mCreateSessionToken(aCreateSessionToken)
    , mPromiseId(aPromiseId)
    , mSessionType(aSessionType)
  {
    mInitData.insert(mInitData.end(),
                     aInitData,
                     aInitData + aInitDataSize);
  }
  virtual void Run() override {
    mTarget->CreateSession(mCreateSessionToken,
                           mPromiseId,
                           "cenc",
                           strlen("cenc"),
                           &mInitData.front(),
                           mInitData.size(),
                           mSessionType);
  }
  virtual void Destroy() override {
    delete this;
  }
private:
  RefPtr<ClearKeySessionManager> mTarget;
  uint32_t mCreateSessionToken;
  uint32_t mPromiseId;
  vector<uint8_t> mInitData;
  GMPSessionType mSessionType;
};


 bool
ClearKeyPersistence::DeferCreateSessionIfNotReady(ClearKeySessionManager* aInstance,
                                                  uint32_t aCreateSessionToken,
                                                  uint32_t aPromiseId,
                                                  const uint8_t* aInitData,
                                                  uint32_t aInitDataSize,
                                                  GMPSessionType aSessionType)
{
  if (sPersistentKeyState >= LOADED)  {
    return false;
  }
  GMPTask* t = new CreateSessionTask(aInstance,
                                     aCreateSessionToken,
                                     aPromiseId,
                                     aInitData,
                                     aInitDataSize,
                                     aSessionType);
  sTasksBlockedOnSessionIdLoad.push_back(t);
  return true;
}

class LoadSessionTask : public GMPTask {
public:
  LoadSessionTask(ClearKeySessionManager* aTarget,
                  uint32_t aPromiseId,
                  const char* aSessionId,
                  uint32_t aSessionIdLength)
    : mTarget(aTarget)
    , mPromiseId(aPromiseId)
    , mSessionId(aSessionId, aSessionId + aSessionIdLength)
  {
  }
  virtual void Run() override {
    mTarget->LoadSession(mPromiseId,
                         mSessionId.c_str(),
                         mSessionId.size());
  }
  virtual void Destroy() override {
    delete this;
  }
private:
  RefPtr<ClearKeySessionManager> mTarget;
  uint32_t mPromiseId;
  string mSessionId;
};

 bool
ClearKeyPersistence::DeferLoadSessionIfNotReady(ClearKeySessionManager* aInstance,
                                                uint32_t aPromiseId,
                                                const char* aSessionId,
                                                uint32_t aSessionIdLength)
{
  if (sPersistentKeyState >= LOADED)  {
    return false;
  }
  GMPTask* t = new LoadSessionTask(aInstance,
                                   aPromiseId,
                                   aSessionId,
                                   aSessionIdLength);
  sTasksBlockedOnSessionIdLoad.push_back(t);
  return true;
}

 bool
ClearKeyPersistence::IsPersistentSessionId(const string& aSessionId)
{
  return Contains(sPersistentSessionIds, atoi(aSessionId.c_str()));
}

class LoadSessionFromKeysTask : public ReadContinuation {
public:
  LoadSessionFromKeysTask(ClearKeySessionManager* aTarget,
                          const string& aSessionId,
                          uint32_t aPromiseId)
    : mTarget(aTarget)
    , mSessionId(aSessionId)
    , mPromiseId(aPromiseId)
  {
  }

  virtual void ReadComplete(GMPErr aStatus,
                            const uint8_t* aData,
                            uint32_t aLength) override
  {
    mTarget->PersistentSessionDataLoaded(aStatus, mPromiseId, mSessionId, aData, aLength);
  }
private:
  RefPtr<ClearKeySessionManager> mTarget;
  string mSessionId;
  uint32_t mPromiseId;
};

 void
ClearKeyPersistence::LoadSessionData(ClearKeySessionManager* aInstance,
                                     const string& aSid,
                                     uint32_t aPromiseId)
{
  LoadSessionFromKeysTask* loadTask =
    new LoadSessionFromKeysTask(aInstance, aSid, aPromiseId);
  ReadData(aSid, loadTask);
}

 void
ClearKeyPersistence::PersistentSessionRemoved(const string& aSessionId)
{
  sPersistentSessionIds.erase(atoi(aSessionId.c_str()));
}
