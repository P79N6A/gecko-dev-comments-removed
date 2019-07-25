





































#include "ContentProcessChild.h"
#include "TabChild.h"

#include "mozilla/ipc/TestShellChild.h"
#include "mozilla/net/NeckoChild.h"
#include "History.h"

#include "nsXULAppAPI.h"

#include "nsDocShellCID.h"
#include "nsNetUtil.h"
#include "base/message_loop.h"
#include "base/task.h"

#include "nsChromeRegistry.h"

using namespace mozilla::ipc;
using namespace mozilla::net;
using namespace mozilla::places;

namespace mozilla {
namespace dom {

ContentProcessChild* ContentProcessChild::sSingleton;

ContentProcessChild::ContentProcessChild()
    : mQuit(PR_FALSE)
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
  NS_ENSURE_TRUE(iframe && NS_SUCCEEDED(iframe->Init()) &&
                 mIFrames.AppendElement(iframe),
                 nsnull);
  return iframe.forget().get();
}

bool
ContentProcessChild::DeallocPIFrameEmbedding(PIFrameEmbeddingChild* iframe)
{
    if (mIFrames.RemoveElement(iframe)) {
      TabChild* child = static_cast<TabChild*>(iframe);
      NS_RELEASE(child);
    }
    return true;
}

PTestShellChild*
ContentProcessChild::AllocPTestShell()
{
    PTestShellChild* testshell = new TestShellChild();
    if (testshell && mTestShells.AppendElement(testshell)) {
        return testshell;
    }
    delete testshell;
    return nsnull;
}

bool
ContentProcessChild::DeallocPTestShell(PTestShellChild* shell)
{
    mTestShells.RemoveElement(shell);
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
ContentProcessChild::RecvregisterChrome(const nsTArray<ChromePackage>& packages,
                                        const nsTArray<ChromeResource>& resources)
{
    nsChromeRegistry* chromeRegistry = nsChromeRegistry::GetService();
    chromeRegistry->RegisterRemoteChrome(packages, resources);
    return true;
}

void
ContentProcessChild::Quit()
{
    NS_ASSERTION(mQuit, "Exiting uncleanly!");
    mIFrames.Clear();
    mTestShells.Clear();
}

void
ContentProcessChild::ActorDestroy(ActorDestroyReason why)
{
    if (AbnormalShutdown == why)
        NS_WARNING("shutting down because of crash!");

    mQuit = PR_TRUE;
    Quit();

    XRE_ShutdownChildProcess();
}

bool
ContentProcessChild::RecvNotifyVisited(const nsCString& aURISpec, 
                                       const bool& mIsVisited)
{
    nsresult rv;

    
    nsCOMPtr<nsIURI> newURI;
    rv = NS_NewURI(getter_AddRefs(newURI), aURISpec);
    
    if (NS_SUCCEEDED(rv)) {
        History *hs = History::GetSingleton();
        if (hs) {
            hs->NotifyVisited(newURI, mIsVisited);
        }
    }
    return true;
}

} 
} 
