






































#include "ContentChild.h"
#include "TabChild.h"

#include "mozilla/ipc/TestShellChild.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/ipc/XPCShellEnvironment.h"
#include "mozilla/jsipc/PContextWrapperChild.h"

#include "nsXULAppAPI.h"

#include "base/message_loop.h"
#include "base/task.h"

#include "nsChromeRegistryContent.h"
#include "mozilla/chrome/RegistryMessageUtils.h"

using namespace mozilla::ipc;
using namespace mozilla::net;

namespace mozilla {
namespace dom {

ContentChild* ContentChild::sSingleton;

ContentChild::ContentChild()
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

    ClearPrefObservers();
    XRE_ShutdownChildProcess();
}

nsresult
ContentChild::AddRemotePrefObserver(const nsCString &aDomain, 
                                    const nsCString &aPrefRoot, 
                                    nsIObserver *aObserver, 
                                    PRBool aHoldWeak)
{
    nsPrefObserverStorage* newObserver = 
        new nsPrefObserverStorage(aObserver, aDomain, aPrefRoot, aHoldWeak);

    mPrefObserverArray.AppendElement(newObserver);
    return NS_OK;
}

nsresult
ContentChild::RemoveRemotePrefObserver(const nsCString &aDomain, 
                                       const nsCString &aPrefRoot, 
                                       nsIObserver *aObserver)
{
    if (mPrefObserverArray.IsEmpty())
        return NS_OK;

    nsPrefObserverStorage *entry;
    for (PRUint32 i = 0; i < mPrefObserverArray.Length(); ++i) {
        entry = mPrefObserverArray[i];
        if (entry && entry->GetObserver() == aObserver &&
                     entry->GetDomain().Equals(aDomain)) {
            
            mPrefObserverArray.RemoveElementAt(i);
            return NS_OK;
        }
    }
    NS_WARNING("No preference Observer was matched !");
    return NS_ERROR_UNEXPECTED;
}

bool
ContentChild::RecvNotifyRemotePrefObserver(const nsCString& aDomain)
{
    nsPrefObserverStorage *entry;
    for (PRUint32 i = 0; i < mPrefObserverArray.Length(); ) {
        entry = mPrefObserverArray[i];
        nsCAutoString prefName(entry->GetPrefRoot() + entry->GetDomain());
        
        
        if (StringBeginsWith(aDomain, prefName)) {
            if (!entry->NotifyObserver()) {
                
                mPrefObserverArray.RemoveElementAt(i);
                continue;
            }
        }
        ++i;
    }
    return true;
}

} 
} 
