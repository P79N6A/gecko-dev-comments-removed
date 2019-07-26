





#include "SharedSSLState.h"
#include "nsClientAuthRemember.h"
#include "nsComponentManagerUtils.h"
#include "nsICertOverrideService.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "nsThreadUtils.h"
#include "nsCRT.h"
#include "nsServiceManagerUtils.h"
#include "nsRecentBadCerts.h"
#include "PSMRunnable.h"
#include "PublicSSL.h"
#include "ssl.h"

using mozilla::psm::SyncRunnableBase;

namespace {

class CertOverrideClearer : public SyncRunnableBase
{
public:
  void RunOnTargetThread() {
    nsCOMPtr<nsICertOverrideService> icos = do_GetService(NS_CERTOVERRIDE_CONTRACTID);
    if (icos) {
      icos->ClearValidityOverride(
        NS_LITERAL_CSTRING("all:temporary-certificates"),
        0);
    }
  }
};

} 

namespace mozilla {

void ClearPrivateSSLState()
{
  SSL_ClearSessionCache();

  nsCOMPtr<nsIX509CertDB> certdb = do_GetService(NS_X509CERTDB_CONTRACTID);
  if (certdb) {
    nsCOMPtr<nsIRecentBadCerts> badCerts;
    certdb->GetRecentBadCerts(true, getter_AddRefs(badCerts));
    if (badCerts) {
      badCerts->ResetStoredCerts();
    }
  }

  RefPtr<CertOverrideClearer> runnable = new CertOverrideClearer;
  runnable->DispatchToMainThreadAndWait();
}

namespace psm {

namespace {
class PrivateBrowsingObserver : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  PrivateBrowsingObserver(SharedSSLState* aOwner) : mOwner(aOwner) {}
  virtual ~PrivateBrowsingObserver() {}
private:
  SharedSSLState* mOwner;
};

SharedSSLState* gPublicState;
SharedSSLState* gPrivateState;
} 

NS_IMPL_ISUPPORTS1(PrivateBrowsingObserver, nsIObserver)

NS_IMETHODIMP
PrivateBrowsingObserver::Observe(nsISupports     *aSubject,
                                 const char      *aTopic,
                                 const PRUnichar *aData)
{
  if (!nsCRT::strcmp(aTopic, "last-pb-context-exited")) {
    mOwner->ResetStoredData();
  }
  return NS_OK;
}

SharedSSLState::SharedSSLState()
: mClientAuthRemember(new nsClientAuthRememberService)
{
  mIOLayerHelpers.Init();
  mClientAuthRemember->Init();
}

void
SharedSSLState::NotePrivateBrowsingStatus()
{
  mObserver = new PrivateBrowsingObserver(this);
  nsCOMPtr<nsIObserverService> obsSvc = mozilla::services::GetObserverService();
  obsSvc->AddObserver(mObserver, "last-pb-context-exited", false);
}

void
SharedSSLState::ResetStoredData()
{
  mClientAuthRemember->ClearRememberedDecisions();
  mIOLayerHelpers.clearStoredData();
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

  gPrivateState->Cleanup();
  delete gPrivateState;
  gPrivateState = nullptr;

  gPublicState->Cleanup();
  delete gPublicState;
  gPublicState = nullptr;
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
