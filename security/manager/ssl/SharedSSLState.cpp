





#include "SharedSSLState.h"
#include "nsClientAuthRemember.h"
#include "nsComponentManagerUtils.h"
#include "nsICertOverrideService.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "nsThreadUtils.h"
#include "nsCRT.h"
#include "nsServiceManagerUtils.h"
#include "PSMRunnable.h"
#include "PublicSSL.h"
#include "ssl.h"
#include "nsNetCID.h"
#include "mozilla/Atomics.h"
#include "mozilla/unused.h"

using mozilla::psm::SyncRunnableBase;
using mozilla::Atomic;
using mozilla::unused;

namespace {

static Atomic<bool> sCertOverrideSvcExists(false);

class MainThreadClearer : public SyncRunnableBase
{
public:
  MainThreadClearer() : mShouldClearSessionCache(false) {}

  void RunOnTargetThread() {
    
    
    

    bool certOverrideSvcExists = sCertOverrideSvcExists.exchange(false);
    if (certOverrideSvcExists) {
      sCertOverrideSvcExists = true;
      nsCOMPtr<nsICertOverrideService> icos = do_GetService(NS_CERTOVERRIDE_CONTRACTID);
      if (icos) {
        icos->ClearValidityOverride(
          NS_LITERAL_CSTRING("all:temporary-certificates"),
          0);
      }
    }

    
    
    mShouldClearSessionCache = mozilla::psm::PrivateSSLState() &&
                               mozilla::psm::PrivateSSLState()->SocketCreated();
  }
  bool mShouldClearSessionCache;
};

} 

namespace mozilla {

void ClearPrivateSSLState()
{
  
  
  
#ifdef DEBUG
  nsresult rv;
  nsCOMPtr<nsIEventTarget> sts
    = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  bool onSTSThread;
  rv = sts->IsOnCurrentThread(&onSTSThread);
  MOZ_ASSERT(NS_SUCCEEDED(rv) && onSTSThread);
#endif

  RefPtr<MainThreadClearer> runnable = new MainThreadClearer;
  runnable->DispatchToMainThreadAndWait();

  
  
  if (runnable->mShouldClearSessionCache) {
    SSL_ClearSessionCache();
  }
}

namespace psm {

namespace {
class PrivateBrowsingObserver : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  explicit PrivateBrowsingObserver(SharedSSLState* aOwner) : mOwner(aOwner) {}
protected:
  virtual ~PrivateBrowsingObserver() {}
private:
  SharedSSLState* mOwner;
};

SharedSSLState* gPublicState;
SharedSSLState* gPrivateState;
} 

NS_IMPL_ISUPPORTS(PrivateBrowsingObserver, nsIObserver)

NS_IMETHODIMP
PrivateBrowsingObserver::Observe(nsISupports     *aSubject,
                                 const char      *aTopic,
                                 const char16_t *aData)
{
  if (!nsCRT::strcmp(aTopic, "last-pb-context-exited")) {
    mOwner->ResetStoredData();
  }
  return NS_OK;
}

SharedSSLState::SharedSSLState()
: mClientAuthRemember(new nsClientAuthRememberService)
, mMutex("SharedSSLState::mMutex")
, mSocketCreated(false)
, mOCSPStaplingEnabled(false)
{
  mIOLayerHelpers.Init();
  mClientAuthRemember->Init();
}

SharedSSLState::~SharedSSLState()
{
}

void
SharedSSLState::NotePrivateBrowsingStatus()
{
  MOZ_ASSERT(NS_IsMainThread(), "Not on main thread");
  mObserver = new PrivateBrowsingObserver(this);
  nsCOMPtr<nsIObserverService> obsSvc = mozilla::services::GetObserverService();
  obsSvc->AddObserver(mObserver, "last-pb-context-exited", false);
}

void
SharedSSLState::ResetStoredData()
{
  MOZ_ASSERT(NS_IsMainThread(), "Not on main thread");
  mClientAuthRemember->ClearRememberedDecisions();
  mIOLayerHelpers.clearStoredData();
}

void
SharedSSLState::NoteSocketCreated()
{
  MutexAutoLock lock(mMutex);
  mSocketCreated = true;
}

bool
SharedSSLState::SocketCreated()
{
  MutexAutoLock lock(mMutex);
  return mSocketCreated;
}

 void
SharedSSLState::GlobalInit()
{
  MOZ_ASSERT(NS_IsMainThread(), "Not on main thread");
  gPublicState = new SharedSSLState();
  gPrivateState = new SharedSSLState();
  gPrivateState->NotePrivateBrowsingStatus();
}

 void
SharedSSLState::GlobalCleanup()
{
  MOZ_ASSERT(NS_IsMainThread(), "Not on main thread");

  if (gPrivateState) {
    gPrivateState->Cleanup();
    delete gPrivateState;
    gPrivateState = nullptr;
  }

  if (gPublicState) {
    gPublicState->Cleanup();
    delete gPublicState;
    gPublicState = nullptr;
  }
}

 void
SharedSSLState::NoteCertOverrideServiceInstantiated()
{
  sCertOverrideSvcExists = true;
}

void
SharedSSLState::Cleanup()
{
  mIOLayerHelpers.Cleanup();
}

SharedSSLState*
PublicSSLState()
{
  return gPublicState;
}

SharedSSLState*
PrivateSSLState()
{
  return gPrivateState;
}

} 
} 
