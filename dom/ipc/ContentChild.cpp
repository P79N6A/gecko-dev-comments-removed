






































#include "ContentChild.h"
#include "TabChild.h"

#include "mozilla/ipc/TestShellChild.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/ipc/XPCShellEnvironment.h"
#include "mozilla/jsipc/PContextWrapperChild.h"

#include "nsIObserverService.h"
#include "nsTObserverArray.h"
#include "nsIObserver.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsServiceManagerUtils.h"
#include "nsXULAppAPI.h"
#include "nsWeakReference.h"

#include "History.h"
#include "nsDocShellCID.h"
#include "nsNetUtil.h"

#include "base/message_loop.h"
#include "base/task.h"

#include "nsChromeRegistryContent.h"
#include "mozilla/chrome/RegistryMessageUtils.h"

using namespace mozilla::ipc;
using namespace mozilla::net;
using namespace mozilla::places;

namespace mozilla {
namespace dom {

class PrefObserver
{
public:
    





    PrefObserver(nsIObserver *aObserver, bool aHoldWeak,
                 const nsCString& aPrefRoot, const nsCString& aDomain)
        : mPrefRoot(aPrefRoot)
        , mDomain(aDomain)
    {
        if (aHoldWeak) {
            nsCOMPtr<nsISupportsWeakReference> supportsWeakRef = 
                do_QueryInterface(aObserver);
            if (supportsWeakRef)
                mWeakObserver = do_GetWeakReference(aObserver);
        } else {
            mObserver = aObserver;
        }
    }

    ~PrefObserver() {}

    



    bool IsDead() const
    {
        nsCOMPtr<nsIObserver> observer = GetObserver();
        return !!observer;
    }

    



    bool ShouldRemoveFrom(nsIObserver* aObserver,
                          const nsCString& aPrefRoot,
                          const nsCString& aDomain) const
    {
        nsCOMPtr<nsIObserver> observer = GetObserver();
        return (observer == aObserver &&
                mDomain == aDomain && mPrefRoot == aPrefRoot);
    }

    


    bool Observes(const nsCString& aPref) const
    {
        nsCAutoString myPref(mPrefRoot);
        myPref += mDomain;
        return StringBeginsWith(aPref, myPref);
    }

    




    bool Notify() const
    {
        nsCOMPtr<nsIObserver> observer = GetObserver();
        if (!observer) {
            return false;
        }

        nsCOMPtr<nsIPrefBranch> prefBranch;
        nsCOMPtr<nsIPrefService> prefService =
            do_GetService(NS_PREFSERVICE_CONTRACTID);
        if (prefService) {
            prefService->GetBranch(mPrefRoot.get(), 
                                   getter_AddRefs(prefBranch));
            observer->Observe(prefBranch, "nsPref:changed",
                              NS_ConvertASCIItoUTF16(mDomain).get());
        }
        return true;
    }

private:
    already_AddRefed<nsIObserver> GetObserver() const
    {
        nsCOMPtr<nsIObserver> observer =
            mObserver ? mObserver : do_QueryReferent(mWeakObserver);
        return observer.forget();
    }

    
    
    
    nsCOMPtr<nsIObserver> mObserver;
    nsWeakPtr mWeakObserver;
    nsCString mPrefRoot;
    nsCString mDomain;

    
    PrefObserver(const PrefObserver&);
    PrefObserver& operator=(const PrefObserver&);
};


ContentChild* ContentChild::sSingleton;

ContentChild::ContentChild()
    : mDead(false)
{
}

ContentChild::~ContentChild()
{
}

bool
ContentChild::Init(MessageLoop* aIOLoop,
                   base::ProcessHandle aParentHandle,
                   IPC::Channel* aChannel)
{
    NS_ASSERTION(!sSingleton, "only one ContentChild per child");
  
    Open(aChannel, aParentHandle, aIOLoop);
    sSingleton = this;

    return true;
}

PBrowserChild*
ContentChild::AllocPBrowser(const PRUint32& aChromeFlags)
{
  nsRefPtr<TabChild> iframe = new TabChild(aChromeFlags);
  return NS_SUCCEEDED(iframe->Init()) ? iframe.forget().get() : NULL;
}

bool
ContentChild::DeallocPBrowser(PBrowserChild* iframe)
{
    TabChild* child = static_cast<TabChild*>(iframe);
    NS_RELEASE(child);
    return true;
}

PTestShellChild*
ContentChild::AllocPTestShell()
{
    return new TestShellChild();
}

bool
ContentChild::DeallocPTestShell(PTestShellChild* shell)
{
    delete shell;
    return true;
}

bool
ContentChild::RecvPTestShellConstructor(PTestShellChild* actor)
{
    actor->SendPContextWrapperConstructor()->SendPObjectWrapperConstructor(true);
    return true;
}

PNeckoChild* 
ContentChild::AllocPNecko()
{
    return new NeckoChild();
}

bool 
ContentChild::DeallocPNecko(PNeckoChild* necko)
{
    delete necko;
    return true;
}

bool
ContentChild::RecvRegisterChrome(const nsTArray<ChromePackage>& packages,
                                 const nsTArray<ResourceMapping>& resources,
                                 const nsTArray<OverrideMapping>& overrides)
{
    nsCOMPtr<nsIChromeRegistry> registrySvc = nsChromeRegistry::GetService();
    nsChromeRegistryContent* chromeRegistry =
        static_cast<nsChromeRegistryContent*>(registrySvc.get());
    chromeRegistry->RegisterRemoteChrome(packages, resources, overrides);
    return true;
}

bool
ContentChild::RecvSetOffline(const PRBool& offline)
{
  nsCOMPtr<nsIIOService> io (do_GetIOService());
  NS_ASSERTION(io, "IO Service can not be null");

  io->SetOffline(offline);
    
  return true;
}

void
ContentChild::ActorDestroy(ActorDestroyReason why)
{
    if (AbnormalShutdown == why)
        NS_WARNING("shutting down because of crash!");

    
    
    
    
    
    
    mDead = true;
    mPrefObservers.Clear();

    XRE_ShutdownChildProcess();
}

nsresult
ContentChild::AddRemotePrefObserver(const nsCString& aDomain, 
                                    const nsCString& aPrefRoot, 
                                    nsIObserver* aObserver, 
                                    PRBool aHoldWeak)
{
    if (aObserver) {
        mPrefObservers.AppendElement(
            new PrefObserver(aObserver, aHoldWeak, aPrefRoot, aDomain));
    }
    return NS_OK;
}

nsresult
ContentChild::RemoveRemotePrefObserver(const nsCString& aDomain, 
                                       const nsCString& aPrefRoot, 
                                       nsIObserver* aObserver)
{
    if (mDead) {
        
        
        return NS_OK;
    }

    for (PRUint32 i = 0; i < mPrefObservers.Length();
         ) {
        PrefObserver* observer = mPrefObservers[i];
        if (observer->IsDead()) {
            mPrefObservers.RemoveElementAt(i);
            continue;
        } else if (observer->ShouldRemoveFrom(aObserver, aPrefRoot, aDomain)) {
            mPrefObservers.RemoveElementAt(i);
            return NS_OK;
        }
        ++i;
    }

    NS_WARNING("RemoveRemotePrefObserver(): no observer was matched!");
    return NS_ERROR_UNEXPECTED;
}

bool
ContentChild::RecvNotifyRemotePrefObserver(const nsCString& aPref)
{
    for (PRUint32 i = 0; i < mPrefObservers.Length();
         ) {
        PrefObserver* observer = mPrefObservers[i];
        if (observer->Observes(aPref) &&
            !observer->Notify()) {
            
            
            mPrefObservers.RemoveElementAt(i);
            continue;
        }
        ++i;
    }
    return true;
}

bool
ContentChild::RecvNotifyVisited(const IPC::URI& aURI)
{
    nsCOMPtr<nsIURI> newURI = aURI;
    nsRefPtr<History> history = History::GetSingleton();
    history->NotifyVisited(newURI);
    return true;
}

} 
} 
