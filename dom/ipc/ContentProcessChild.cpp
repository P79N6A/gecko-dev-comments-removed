






































#include "ContentProcessChild.h"
#include "TabChild.h"

#include "mozilla/ipc/TestShellChild.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/ipc/XPCShellEnvironment.h"
#include "mozilla/jsipc/PContextWrapperChild.h"

#include "History.h"
#include "nsXULAppAPI.h"

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

ContentProcessChild* ContentProcessChild::sSingleton;

ContentProcessChild::ContentProcessChild()
{
}

ContentProcessChild::~ContentProcessChild()
{
}

bool
ContentProcessChild::Init(MessageLoop* aIOLoop,
                          base::ProcessHandle aParentHandle,
                          IPC::Channel* aChannel)
{
    NS_ASSERTION(!sSingleton, "only one ContentProcessChild per child");
  
    Open(aChannel, aParentHandle, aIOLoop);
    sSingleton = this;

    return true;
}

PIFrameEmbeddingChild*
ContentProcessChild::AllocPIFrameEmbedding()
{
  nsRefPtr<TabChild> iframe = new TabChild();
  return NS_SUCCEEDED(iframe->Init()) ? iframe.forget().get() : NULL;
}

bool
ContentProcessChild::DeallocPIFrameEmbedding(PIFrameEmbeddingChild* iframe)
{
    TabChild* child = static_cast<TabChild*>(iframe);
    NS_RELEASE(child);
    return true;
}

PTestShellChild*
ContentProcessChild::AllocPTestShell()
{
    return new TestShellChild();
}

bool
ContentProcessChild::DeallocPTestShell(PTestShellChild* shell)
{
    delete shell;
    return true;
}

bool
ContentProcessChild::RecvPTestShellConstructor(PTestShellChild* actor)
{
    actor->SendPContextWrapperConstructor()->SendPObjectWrapperConstructor(true);
    return true;
}

PNeckoChild* 
ContentProcessChild::AllocPNecko()
{
    return new NeckoChild();
}

bool 
ContentProcessChild::DeallocPNecko(PNeckoChild* necko)
{
    delete necko;
    return true;
}

bool
ContentProcessChild::RecvRegisterChrome(const nsTArray<ChromePackage>& packages,
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
ContentProcessChild::RecvSetOffline(const PRBool& offline)
{
  nsCOMPtr<nsIIOService> io (do_GetIOService());
  NS_ASSERTION(io, "IO Service can not be null");

  io->SetOffline(offline);
    
  return true;
}

void
ContentProcessChild::ActorDestroy(ActorDestroyReason why)
{
    if (AbnormalShutdown == why)
        NS_WARNING("shutting down because of crash!");

    ClearPrefObservers();
    XRE_ShutdownChildProcess();
}

nsresult
ContentProcessChild::AddRemotePrefObserver(const nsCString &aDomain, 
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
ContentProcessChild::RemoveRemotePrefObserver(const nsCString &aDomain, 
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
ContentProcessChild::RecvNotifyRemotePrefObserver(const nsCString& aDomain)
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

bool
ContentProcessChild::RecvNotifyVisited(const IPC::URI& aURI)
{
    nsCOMPtr<nsIURI> newURI = aURI;
    History::GetSingleton()->NotifyVisited(newURI);
    return true;
}

} 
} 
